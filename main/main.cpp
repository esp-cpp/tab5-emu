#include <sdkconfig.h>

#include <chrono>
#include <cxxabi.h>
#include <new>
#include "esp_rom_sys.h"
#include <exception>
#include <memory>
#include <thread>
#include <vector>

#include "logger.hpp"
#include "task_monitor.hpp"

#include "carts.hpp"
#include "gui.hpp"
#include "heap_utils.hpp"
#include "rom_info.hpp"
#include "statistics.hpp"
#include "tab5-emu.hpp"

using namespace std::chrono_literals;

// A failed C++ allocation would surface as std::bad_alloc in whoever did not
// catch it, far from the cause. Fail here instead, with the size, the task
// and the caller's backtrace on the console.
static void *checked_new(size_t n) {
  void *p = malloc(n ? n : 1);
  if (!p) {
    // the ROM printf: no locks, no heap, and it still prints when the stdio
    // lock or the heap are what failed
    esp_rom_printf("operator new(%u) failed in task %s: internal free %u (largest %u), psram free %u (largest %u)\n",
                   (unsigned)n, pcTaskGetName(nullptr), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
                   (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                   (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
                   (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    abort(); // the panic handler's backtrace names the caller
  }
  return p;
}
void *operator new(size_t n) { return checked_new(n); }
void *operator new[](size_t n) { return checked_new(n); }
void operator delete(void *p) noexcept { free(p); }
void operator delete[](void *p) noexcept { free(p); }
void operator delete(void *p, size_t) noexcept { free(p); }
void operator delete[](void *p, size_t) noexcept { free(p); }

extern "C" void app_main(void) {
  espp::Logger logger({.tag = "tab5-emu", .level = espp::Logger::Verbosity::INFO});
  logger.info("Bootup");
  // an exception nobody catches ends in abort(): say which one first, so the
  // console names the cause instead of a bare backtrace through terminate()
  std::set_terminate([] {
    esp_rom_printf("terminate: in task %s; internal free %u (largest %u), psram free %u (largest %u)\n",
           pcTaskGetName(nullptr), (unsigned)heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
           (unsigned)heap_caps_get_free_size(MALLOC_CAP_SPIRAM),
           (unsigned)heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
    if (auto *t = __cxxabiv1::__cxa_current_exception_type()) {
      printf("terminate: uncaught exception of type %s", t->name());
      try {
        throw;
      } catch (const std::exception &e) {
        printf(": %s", e.what());
      } catch (...) {
      }
      printf("\n");
    } else {
      printf("terminate: no active exception\n");
    }
    abort();
  });

  // initialize the hardware abstraction layer
  auto internal_free_log = [&](const char *stage) {
    multi_heap_info_t info;
    heap_caps_get_info(&info, MALLOC_CAP_INTERNAL);
    logger.info("internal RAM after {}: free {} (largest {}), dma-capable {}; {} blocks allocated ({} bytes)", stage,
                heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                heap_caps_get_free_size(MALLOC_CAP_DMA | MALLOC_CAP_INTERNAL), info.allocated_blocks,
                info.total_allocated_bytes);
  };
  internal_free_log("boot");
  Tab5Emu::dump_core_dump_to_console();
  Tab5Emu &emu = Tab5Emu::get();

  if (!emu.initialize_tab5()) {
    logger.error("Failed to initialize the Tab5!");
    return;
  }
  internal_free_log("initialize_tab5");

  if (!emu.initialize_sdcard()) {
    logger.warn("Failed to initialize SD card!");
    logger.warn("This may happen if the SD card is not inserted.");
  }
  internal_free_log("initialize_sdcard");

  if (!emu.initialize_audio()) {
    logger.warn("Failed to initialize audio!");
  }
  internal_free_log("initialize_audio");

  if (!emu.initialize_battery()) {
    logger.warn("Failed to initialize battery monitoring!");
  }
  internal_free_log("initialize_battery");

  if (!emu.initialize_input()) {
    logger.warn("Failed to initialize input!");
  }
  internal_free_log("initialize_input");
  if (!emu.initialize_usb_host()) {
    // not fatal: touch controls keep working without a keyboard / mouse
    logger.warn("USB host not available (no keyboard / mouse support)");
  }
  internal_free_log("initialize_usb_host");

  if (!emu.initialize_video()) {
    logger.error("Failed to initialize video!");
    return;
  }
  internal_free_log("initialize_video");

  logger.info("initializing gui...");

  // initialize the gui
  Gui gui({.log_level = espp::Logger::Verbosity::WARN});
  internal_free_log("gui");

  print_heap_state();

#if CONFIG_TAB5_EMU_USB_MSC_AT_BOOT
  logger.warn("USB drive test: enabling USB mass storage at boot");
  emu.initialize_usb_msc();
  while (true) {
    std::this_thread::sleep_for(1s);
  }
#endif

  // set the task priority (for main) to high
  vTaskPrioritySet(nullptr, 20);

  // main loop
  while (true) {
    // reset gui ready to play
    gui.ready_to_play(false);
#if CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS > 0
    static bool autostarted = false;
    if (!autostarted && gui.get_selected_rom().has_value()) {
      autostarted = true;
      logger.warn("Auto-starting the first game in {} ms", CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS);
      std::this_thread::sleep_for(std::chrono::milliseconds(CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS));
      gui.ready_to_play(true);
    }
#endif
    while (!gui.ready_to_play()) {
      std::this_thread::sleep_for(50ms);
    }

    gui.pause();

    auto maybe_selected_rom = gui.get_selected_rom();
    if (maybe_selected_rom.has_value()) {
      auto selected_rom = maybe_selected_rom.value();
      logger.info("Selected rom:\n\t{}", selected_rom);

      print_heap_state();

      // Cart handles platform specific code, state management, etc.
      {
        std::unique_ptr<Cart> cart(make_cart(selected_rom));
        if (cart) {
          while (cart->run())
            ;
        } else {
          logger.error("Failed to create cart!");
        }
      }
    } else {
      logger.error("Invalid rom selected!");
    }

    // print the frame statistics from the previous run
    print_statistics();

    logger.info("Done playing, resuming gui...");

    logger.debug("Task table:\n{}", espp::TaskMonitor::get_latest_info_table());

    emu.clear_screen();
    gui.resume();
  }
}

#include <sdkconfig.h>

#include <chrono>
#include <cxxabi.h>
#include <map>
#include <string>
#if CONFIG_HEAP_TRACING_STANDALONE
#include "esp_heap_trace.h"
#endif
#include <new>
#include "esp_rom_sys.h"
#include <exception>
#include <memory>
#include <thread>
#include <vector>

#include "logger.hpp"
#include "task_monitor.hpp"

#include "carts.hpp"
#include "devlink.hpp"
#include "gui.hpp"
#include "heap_utils.hpp"
#include "remote_control.hpp"
#include "rom_info.hpp"
#include "settings.hpp"
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
#if __cpp_exceptions
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
#endif
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

  // the settings kept on the SD card (volume, brightness, video scaling, the
  // last game and its save slots); the first boot writes the defaults
  {
    Settings defaults;
    defaults.volume = static_cast<uint8_t>(emu.volume());
    defaults.muted = emu.is_muted();
    defaults.brightness = static_cast<uint8_t>(emu.brightness());
    defaults.video_setting = static_cast<uint8_t>(emu.video_setting());
    auto &settings = AppSettings::get();
    settings.init(std::string(Tab5Emu::mount_point) + "/settings.bin", defaults);
    const Settings s = settings.get_settings();
    emu.volume(s.volume);
    emu.mute(s.muted);
    emu.brightness(std::max<int>(5, s.brightness));
    emu.video_setting(static_cast<VideoSetting>(std::min<int>(s.video_setting, 2)));
    logger.info("settings: volume {}{}, brightness {}, video {}, resume {}, last game '{}'", s.volume,
                s.muted ? " (muted)" : "", s.brightness, s.video_setting, s.resume_last_save, s.last_rom);
  }
  internal_free_log("settings");

  logger.info("initializing gui...");

  // initialize the gui
  Gui gui({.log_level = espp::Logger::Verbosity::WARN});
  internal_free_log("gui");

  // the USB link's services (core dump, system, monitor) and the test control
  // (see tools/emulink); its requests for the launcher / the game run here, on
  // the main loop, through main_queue
  // the composite USB device on the USB-C port: the console moves to its CDC
  // port, the link services get its vendor interface, the SD card its drive
  // function (the card stays ours until handed over). Started last, as the
  // launcher's USB drive switch used to.
  if (!emu.initialize_usb_device()) {
    logger.warn("USB composite device not available (console stays on USB-Serial-JTAG)");
  }
  internal_free_log("initialize_usb_device");

  MainQueue main_queue;
  RemoteControl control(gui, main_queue);
  DevLink devlink({.device_name = "Tab5 Emu", .firmware = esp_app_get_description()->version});
  devlink.register_module(control);
  internal_free_log("devlink");
  // up and running: an update that got this far is good (else the bootloader
  // rolls back to the updater on the next reset)
  DevLink::mark_app_valid();

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

#if CONFIG_HEAP_TRACING_STANDALONE
  // DIAGNOSTIC: what ends up inside the ROM arena's hole while a cart that
  // released it runs. Trace every allocation from here; when the arena cannot
  // be reserved again after a cart, list the live allocations in its range.
  static heap_trace_record_t *trace_records =
      static_cast<heap_trace_record_t *>(heap_caps_calloc(12000, sizeof(heap_trace_record_t), MALLOC_CAP_SPIRAM));
  const uint8_t *hole_base = static_cast<const uint8_t *>(emu.rom_arena(1));
  if (trace_records) {
    heap_trace_init_standalone(trace_records, 12000);
    heap_trace_start(HEAP_TRACE_LEAKS);
  }
#endif

  // main loop
  while (true) {
    // reset gui ready to play
    gui.ready_to_play(false);
#if CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS > 0
    // AUTOSTART_MATCH is a comma-separated list; with AUTOQUIT_SECONDS each
    // game is quit after that long and the next one in the list starts,
    // round and round (a launch / quit / launch soak test)
    static size_t autostart_launches = 0;
    if ((autostart_launches == 0 || CONFIG_TAB5_EMU_AUTOQUIT_SECONDS > 0) && gui.get_selected_rom().has_value()) {
      std::vector<std::string> matches;
      std::string_view list = CONFIG_TAB5_EMU_AUTOSTART_MATCH;
      while (!list.empty()) {
        const auto comma = list.find(',');
        auto item = list.substr(0, comma);
        if (!item.empty())
          matches.emplace_back(item);
        list = comma == std::string_view::npos ? std::string_view{} : list.substr(comma + 1);
      }
      if (!matches.empty()) {
        const auto &match = matches[autostart_launches % matches.size()];
        if (!gui.select_rom_matching(match)) {
          logger.warn("Auto-start: no game matches '{}'", match);
        }
      }
      autostart_launches++;
      logger.warn("AUTOSTART launch {} ({}) in {} ms; free: internal {} (largest {}), psram {} (largest {})",
                  autostart_launches, gui.get_selected_rom().value().name, CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS,
                  heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                  heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
      std::this_thread::sleep_for(std::chrono::milliseconds(CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS));
      gui.ready_to_play(true);
    }
#endif
    while (!gui.ready_to_play()) {
      std::this_thread::sleep_for(50ms);
      // settings changed in the GUI are written once the changes stop (not
      // while the PC has the card; the launcher hands it over under this lock)
      {
        std::lock_guard<std::recursive_mutex> lk(Tab5Emu::lvgl_mutex());
        if (!emu.usb_msc_host_has_card()) AppSettings::get().save_if_idle();
      }
      main_queue.poll();
    }

    gui.pause();

    auto maybe_selected_rom = gui.get_selected_rom();
    if (maybe_selected_rom.has_value()) {
      auto selected_rom = maybe_selected_rom.value();
      logger.info("Selected rom:\n\t{}", selected_rom);
      AppSettings::get().modify([&](Settings &s) { s.last_rom = selected_rom.rom_path; });
      AppSettings::get().save();

      print_heap_state();

      // Cart handles platform specific code, state management, etc.
      {
        std::unique_ptr<Cart> cart(make_cart(selected_rom));
        if (cart) {
          control.set_cart(cart.get());
          // start from the save state this game was last saved to / loaded from
          if (AppSettings::get().get_settings().resume_last_save) {
            if (const auto slot = AppSettings::get().last_slot(selected_rom.rom_path)) {
              cart->resume_from_slot(*slot);
            }
          }
#if CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS > 0 && CONFIG_TAB5_EMU_AUTOQUIT_SECONDS > 0
          const auto start = std::chrono::steady_clock::now();
          const auto quit_at = start + std::chrono::seconds(CONFIG_TAB5_EMU_AUTOQUIT_SECONDS);
#if CONFIG_TAB5_EMU_AUTOQUIT_STATES
          // a third of the way in the state is saved, two thirds in loaded
          // back, both through the pause menu
          int state_step = 0;
#endif
          while (cart->run()) {
#if CONFIG_TAB5_EMU_AUTOQUIT_STATES
            const auto now = std::chrono::steady_clock::now();
            if (state_step < 2 && now >= start + (state_step + 1) * (quit_at - start) / 3) {
              const auto action = state_step == 0 ? Menu::Action::SAVE : Menu::Action::LOAD;
              logger.warn("AUTOQUIT: {} state from the menu", state_step == 0 ? "save" : "load");
              cart->menu_action_as_from_menu(action);
              logger.warn("AUTOQUIT: {} done; free: internal {} (largest {}), psram {} (largest {})",
                          state_step == 0 ? "save" : "load", heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                          heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT),
                          heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
              state_step++;
            }
#endif
            main_queue.poll();
            if (std::chrono::steady_clock::now() >= quit_at) {
              logger.warn("AUTOQUIT after {} s", CONFIG_TAB5_EMU_AUTOQUIT_SECONDS);
              cart->quit_as_from_menu();
              break;
            }
          }
#else
          while (cart->run())
            main_queue.poll();
#endif
          control.set_cart(nullptr);
        } else {
          logger.error("Failed to create cart!");
        }
      }
    } else {
      logger.error("Invalid rom selected!");
    }

    // The cart is gone, and with it its pause menu and the screenshots LVGL
    // cached for it: the PSRAM heap is as clean as it gets. Reserve the ROM
    // arena again if it was lost (a no-op normally: the carts that need its
    // room borrow it -- Jedi Knight as its engine heap, GBA for ROM cache
    // blocks, MGS and TPM for their arenas -- instead of releasing it, so it
    // keeps its boot address), before the GUI decodes boxart into the heap.
    if (!Tab5Emu::get().reserve_rom_arena()) {
#if CONFIG_HEAP_TRACING_STANDALONE
      if (trace_records && hole_base) {
        const size_t n = heap_trace_get_count();
        std::map<std::string, std::pair<size_t, size_t>> by_caller;
        size_t in_hole = 0, bytes = 0;
        for (size_t i = 0; i < n; i++) {
          heap_trace_record_t rec;
          if (heap_trace_get(i, &rec) != ESP_OK) continue;
          const auto *p = static_cast<const uint8_t *>(rec.address);
          if (p < hole_base || p >= hole_base + 6324736) continue;
          in_hole++;
          bytes += rec.size;
          const auto key = fmt::format("{} {} {} {}", fmt::ptr(rec.alloced_by[0]), fmt::ptr(rec.alloced_by[1]),
                                       fmt::ptr(rec.alloced_by[2]), fmt::ptr(rec.alloced_by[3]));
          auto &e = by_caller[key];
          e.first++;
          e.second += rec.size;
        }
        logger.warn("hole: {} live allocations, {} bytes, of {} traced", in_hole, bytes, n);
        for (const auto &[k, v] : by_caller) {
          if (v.second >= 2048) logger.warn("  {} x{} {} bytes", k, v.first, v.second);
        }
      }
#endif
    }

    // volume / brightness / video scaling may have changed in the pause menu
    AppSettings::get().modify([&](Settings &s) {
      s.volume = static_cast<uint8_t>(emu.volume());
      s.muted = emu.is_muted();
      s.brightness = static_cast<uint8_t>(emu.brightness());
      s.video_setting = static_cast<uint8_t>(emu.video_setting());
    });
    AppSettings::get().save();

    // print the frame statistics from the previous run
    print_statistics();

    logger.info("Done playing, resuming gui...");

    logger.debug("Task table:\n{}", espp::TaskMonitor::get_latest_info_table());

    emu.clear_screen();
    gui.resume();
  }
}

#include <sdkconfig.h>

#include <chrono>
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

extern "C" void app_main(void) {
  espp::Logger logger({.tag = "tab5-emu", .level = espp::Logger::Verbosity::INFO});
  logger.info("Bootup");

  // initialize the hardware abstraction layer
  Tab5Emu &emu = Tab5Emu::get();

  if (!emu.initialize_tab5()) {
    logger.error("Failed to initialize the Tab5!");
    return;
  }

  if (!emu.initialize_sdcard()) {
    logger.warn("Failed to initialize SD card!");
    logger.warn("This may happen if the SD card is not inserted.");
  }

  if (!emu.initialize_audio()) {
    logger.warn("Failed to initialize audio!");
  }

  if (!emu.initialize_battery()) {
    logger.warn("Failed to initialize battery monitoring!");
  }

  if (!emu.initialize_input()) {
    logger.warn("Failed to initialize input!");
  }

  if (!emu.initialize_video()) {
    logger.error("Failed to initialize video!");
    return;
  }

  logger.info("initializing gui...");

  // initialize the gui
  Gui gui({.log_level = espp::Logger::Verbosity::WARN});

  print_heap_state();

  // set the task priority (for main) to high
  vTaskPrioritySet(nullptr, 20);

  // main loop
  while (true) {
    // reset gui ready to play
    gui.ready_to_play(false);
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

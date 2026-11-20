#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include "lvgl.h"

#include "event_manager.hpp"
#include "high_resolution_timer.hpp"
#include "logger.hpp"

#include "emu-ui/pause_menu.hpp"

#include "statistics.hpp"
#include "tab5-emu.hpp"

/// In-game pause menu: resume / reset / save / load / quit, a save slot
/// selector with the slot's screenshot, and the shared volume / brightness /
/// video scaling settings. The screen is emu-ui's (shared with esp-box-emu).
class Menu {
public:
  static constexpr size_t MAX_SLOT = 5;
  enum class Action { RESUME, RESET, SAVE, LOAD, QUIT };

  typedef std::function<void(Action)> action_fn;
  typedef std::function<std::string()> slot_image_fn;

  struct Config {
    std::string paused_image_path;
    action_fn action_callback;
    slot_image_fn slot_image_callback;
    std::string game_title;  ///< in the menu's title bar
    std::string console_key; ///< rom_info's ConsoleInfo::key: the background
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit Menu(const Config &config);
  ~Menu();

  size_t get_selected_slot() const { return selected_slot_; }
  void select_slot(int slot);

  bool is_paused() const { return paused_; }
  void pause();
  void resume();

  /// Do an action as a tap on its row would (the automated launch tests)
  void click(Action action);

protected:
  void update();
  void update_slot_display();
  void update_pause_image();
  void on_volume(const std::vector<uint8_t> &data);
  void on_action(emu_ui::PauseMenu::Action action);

  std::string paused_image_path_;
  action_fn action_callback_;
  slot_image_fn slot_image_callback_;
  std::unique_ptr<emu_ui::PauseMenu> view_;

  /// screenshots are at most the largest native game frame (JK's 640x480 menu / HUD)
  static constexpr size_t SCREENSHOT_MAX_BYTES = 640 * 480 * 2;
  /// A screenshot's pixels, only while the menu is open: a game loading (JK's
  /// level load) needs every MB of PSRAM it can get
  struct ImageBuffer {
    uint8_t *data{nullptr};
    std::span<uint8_t> span() const { return data ? std::span<uint8_t>(data, SCREENSHOT_MAX_BYTES) : std::span<uint8_t>(); }
    void allocate();
    void release();
  };
  ImageBuffer pause_image_data_;
  lv_image_dsc_t pause_image_desc_{};
  ImageBuffer slot_image_data_;
  lv_image_dsc_t slot_image_desc_{};

  std::atomic<int> selected_slot_{0};
  std::atomic<bool> paused_{true};
  espp::HighResolutionTimer task_{{
      .name = "Menu Task",
      .callback = std::bind(&Menu::update, this),
  }};
  espp::Logger logger_;
  std::recursive_mutex &mutex_{Tab5Emu::lvgl_mutex()}; // shared: LVGL is not thread-safe
};

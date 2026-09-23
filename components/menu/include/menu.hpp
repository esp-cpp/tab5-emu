#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

#include "lvgl.h"

#include "event_manager.hpp"
#include "high_resolution_timer.hpp"
#include "logger.hpp"

#include "statistics.hpp"
#include "tab5-emu.hpp"

/// In-game pause menu: resume / reset / save / load / quit, a save slot
/// selector with the slot's screenshot, and the shared volume / brightness /
/// video scaling settings.
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
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit Menu(const Config &config);
  ~Menu();

  size_t get_selected_slot() const { return selected_slot_; }
  void select_slot(int slot);

  bool is_paused() const { return paused_; }
  void pause();
  void resume();

protected:
  void init_ui();
  void deinit_ui();
  void update();
  void update_shared_state();
  void update_slot_display();
  void update_pause_image();
  void on_volume(const std::vector<uint8_t> &data);
  static void event_callback(lv_event_t *e);
  /// Load a "[w:u16be][h:u16be][rgb565]" screenshot file into an lv image
  bool load_screenshot(const std::string &path, lv_obj_t *image, std::vector<uint8_t> &storage,
                       lv_image_dsc_t &desc);

  std::string paused_image_path_;
  action_fn action_callback_;
  slot_image_fn slot_image_callback_;

  lv_obj_t *screen_{nullptr};
  lv_group_t *group_{nullptr}; ///< gamepad / keyboard focus order
  lv_obj_t *pause_image_{nullptr};
  lv_obj_t *slot_image_{nullptr};
  lv_obj_t *slot_label_{nullptr};
  lv_obj_t *fps_label_{nullptr};
  lv_obj_t *volume_slider_{nullptr};
  lv_obj_t *brightness_slider_{nullptr};
  lv_obj_t *video_dropdown_{nullptr};
  lv_obj_t *btn_resume_{nullptr};
  lv_obj_t *btn_reset_{nullptr};
  lv_obj_t *btn_save_{nullptr};
  lv_obj_t *btn_load_{nullptr};
  lv_obj_t *btn_quit_{nullptr};
  lv_obj_t *btn_slot_prev_{nullptr};
  lv_obj_t *btn_slot_next_{nullptr};
  /// screenshots are at most the largest native game frame (JK's 640x480 menu / HUD)
  static constexpr size_t SCREENSHOT_MAX_BYTES = 640 * 480 * 2;
  std::vector<uint8_t> pause_image_data_;
  lv_image_dsc_t pause_image_desc_{};
  std::vector<uint8_t> slot_image_data_;
  lv_image_dsc_t slot_image_desc_{};

  std::atomic<int> selected_slot_{0};
  std::atomic<bool> paused_{true};
  espp::HighResolutionTimer task_{{
      .name = "Menu Task",
      .callback = std::bind(&Menu::update, this),
  }};
  espp::Logger logger_;
  std::recursive_mutex mutex_;
};

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <vector>

#include "lvgl.h"

#include "event_manager.hpp"
#include "high_resolution_timer.hpp"
#include "logger.hpp"

#include "rom_info.hpp"
#include "tab5-emu.hpp"

/// The launcher GUI: a list of games (from metadata.csv on the SD card) with
/// boxart, plus settings (volume, brightness, video scaling). Hand-written
/// LVGL, laid out for the Tab5's 1280x720 landscape screen.
class Gui {
public:
  struct Config {
    std::string metadata_filename = "metadata.csv";
    espp::Logger::Verbosity log_level{espp::Logger::Verbosity::WARN};
  };

  explicit Gui(const Config &config);
  ~Gui();

  void ready_to_play(bool new_state) { ready_to_play_ = new_state; }
  bool ready_to_play() const { return ready_to_play_; }

  std::optional<RomInfo> get_selected_rom() const {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    if (focused_rom_ < 0 || focused_rom_ >= (int)rom_infos_.size()) {
      return std::nullopt;
    }
    return rom_infos_[focused_rom_];
  }

  /// Stop driving LVGL (while a game is running)
  void pause();
  /// Reload the rom list if the metadata changed, and start driving LVGL again
  void resume();

  void update_rom_list();

protected:
  void init_ui();
  void deinit_ui();
  void update_shared_state();
  void select_rom(int index);
  void update();

  void on_volume(const std::vector<uint8_t> &data);
  void on_battery(const std::vector<uint8_t> &data);

  static void event_callback(lv_event_t *e);

  std::vector<RomInfo> rom_infos_;
  std::atomic<int> focused_rom_{-1};

  lv_obj_t *screen_{nullptr};
  lv_obj_t *rom_list_{nullptr};
  lv_obj_t *boxart_{nullptr};
  lv_obj_t *title_label_{nullptr};
  lv_obj_t *play_button_{nullptr};
  lv_obj_t *volume_slider_{nullptr};
  lv_obj_t *brightness_slider_{nullptr};
  lv_obj_t *mute_switch_{nullptr};
  lv_obj_t *video_dropdown_{nullptr};
  lv_obj_t *battery_label_{nullptr};
  std::vector<lv_obj_t *> rom_buttons_;

  std::string metadata_filename_;
  std::filesystem::file_time_type metadata_last_modified_{};

  std::atomic<bool> paused_{false};
  std::atomic<bool> ready_to_play_{false};
  espp::HighResolutionTimer task_{{
      .name = "Gui Task",
      .callback = std::bind(&Gui::update, this),
  }};
  espp::Logger logger_;
  mutable std::recursive_mutex mutex_;
};

#pragma once

#include <atomic>
#include <filesystem>
#include <memory>
#include <mutex>
#include <optional>
#include <string_view>
#include <vector>

#include "lvgl.h"

#include "event_manager.hpp"
#include "high_resolution_timer.hpp"
#include "logger.hpp"

#include "console_art.hpp"
#include "rom_info.hpp"
#include "settings.hpp"
#include "tab5-emu.hpp"

/// The launcher GUI: a list of games (from metadata.csv on the SD card),
/// grouped under a header per console, with the selected game's boxart and
/// console, plus settings (volume, brightness, video scaling, resume), which
/// are kept in AppSettings. Hand-written LVGL, laid out for the Tab5's
/// 1280x720 landscape screen.
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
  /// Re-read metadata.csv even if its modification time did not change (FAT
  /// times are coarse, and without a set clock they may not move at all).
  void reload_rom_list() {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    metadata_last_modified_ = std::filesystem::file_time_type::min();
    update_rom_list();
  }
  /// The games in list order (grouped by console).
  std::vector<RomInfo> rom_infos() const {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    return rom_infos_;
  }

  /// Select the first game whose name or rom path contains `text` (a game
  /// the console filter hides lifts the filter)
  bool select_rom_matching(std::string_view text);

  /// The consoles that have games, in list order.
  std::vector<Console> consoles() const;
  /// The console whose games the list shows, if it is filtered.
  std::optional<Console> console_filter() const;
  /// Show only `console`'s games (nullopt: every console's); false if it has none.
  bool set_console_filter(std::optional<Console> console);

  /// The settings the launcher shows, for the USB link: {settings: {name:
  /// value}, limits: {name: [min, max]}} (volume, muted, brightness, video,
  /// resume).
  std::string settings_json() const;
  /// Change one of them as its control in the launcher does (shown, applied,
  /// saved once the changes stop); false for an unknown name or a value out
  /// of range.
  bool set_setting(const std::string &name, int value);

  /// The launcher screen (RGB565, little endian, rows packed), for the USB
  /// link's screenshot when no game runs; empty if it cannot be drawn.
  struct Snapshot {
    size_t width{0}, height{0};
    std::vector<uint8_t> pixels;
  };
  Snapshot snapshot();

protected:
  void init_ui();
  void deinit_ui();
  void update_shared_state();
  void select_rom(int index);
  void build_list(int keep_rom);
  void step_console(int dir);
  void update_filter_marker();
  void open_console_picker();
  void close_console_picker();
  static void picker_event(lv_event_t *e);
  void update();

  void on_volume(const std::vector<uint8_t> &data);
  void on_battery(const std::vector<uint8_t> &data);

  static void event_callback(lv_event_t *e);

  std::vector<RomInfo> rom_infos_;
  std::atomic<int> focused_rom_{-1};

  lv_obj_t *screen_{nullptr};
  lv_group_t *group_{nullptr}; ///< gamepad / keyboard focus order
  lv_obj_t *rom_list_{nullptr};
  lv_obj_t *boxart_{nullptr};
  lv_draw_buf_t *boxart_buf_{nullptr}; ///< decoded boxart the image widget shows
  void load_boxart(const std::string &path);
  /// Decode a JPEG on the SD card into a new RGB565 draw buffer (nullptr if it cannot)
  lv_draw_buf_t *decode_image(const std::string &path);

  // the console the selected game runs on: its SD card art, or a badge
  lv_obj_t *console_area_{nullptr};
  lv_draw_buf_t *console_art_buf_{nullptr};
  int shown_console_{-1};
  void show_console(Console console);
  void add_console_header(Console console);
  lv_obj_t *title_label_{nullptr};
  lv_obj_t *play_button_{nullptr};
  lv_obj_t *volume_slider_{nullptr};
  lv_obj_t *brightness_slider_{nullptr};
  lv_obj_t *mute_switch_{nullptr};
  lv_obj_t *usb_switch_{nullptr};
  lv_obj_t *usb_label_{nullptr};
  void set_usb_drive(bool on);
  lv_obj_t *video_dropdown_{nullptr};
  lv_obj_t *resume_switch_{nullptr};
  lv_obj_t *battery_label_{nullptr};
  std::vector<lv_obj_t *> rom_buttons_; ///< per game (rom_infos_ index); nullptr if filtered out

  // the console filter: the Console whose games the list shows, -1 all
  int filter_{-1};
  lv_obj_t *picker_{nullptr};        ///< the console picker, while it is open
  lv_group_t *picker_group_{nullptr};
  std::vector<int> picker_consoles_; ///< the picker's rows: Console, -1 all

  std::string metadata_filename_;
  std::filesystem::file_time_type metadata_last_modified_{};

  std::atomic<bool> paused_{false};
  std::atomic<bool> ready_to_play_{false};
  espp::HighResolutionTimer task_{{
      .name = "Gui Task",
      .callback = std::bind(&Gui::update, this),
  }};
  espp::Logger logger_;
  std::recursive_mutex &mutex_{Tab5Emu::lvgl_mutex()}; // shared: LVGL is not thread-safe
};

#pragma once

#include <atomic>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include "logger.hpp"

#include "menu.hpp"
#include "rom_info.hpp"
#include "tab5-emu.hpp"

/// This class is the base class for all carts (games / emulator cores).
/// It provides the following functionality:
/// - in-game pause menu (resume / reset / save / load / quit)
/// - screenshot capture for the save slots
/// - video setting (original / fit / fill)
class Cart {
public:
  using Pixel = Tab5Emu::Pixel;

  /// Configuration for the Cart class
  struct Config {
    RomInfo info;             ///< rom info
    bool copy_romdata = true; ///< copy the romdata into the shared rom buffer
    espp::Logger::Verbosity verbosity = espp::Logger::Verbosity::WARN; ///< verbosity for the logger
  };

  explicit Cart(const Config &config)
      : info_(config.info)
      , savedir_(FS_PREFIX + "/" + SAVE_DIR)
      , logger_({.tag = "Cart", .level = config.verbosity}) {
    logger_.info("ctor");
    auto &emu = Tab5Emu::get();
    emu.clear_screen();
    // drop a menu request left over from the GUI (e.g. a tap in the hot
    // corner while choosing the game) so the cart doesn't open the pause menu
    // before the game has started
    (void)emu.menu_requested();

    if (config.copy_romdata) {
      logger_.info("Copying romdata...");
      rom_size_bytes_ = emu.copy_file_to_romdata(get_rom_filename());
      romdata_ = emu.romdata();
    }

    menu_ = std::make_unique<Menu>(Menu::Config{
        .paused_image_path = get_paused_image_path(),
        .action_callback = std::bind(&Cart::on_menu_action, this, std::placeholders::_1),
        .slot_image_callback = [this]() -> std::string { return get_screenshot_path(false); },
        .log_level = espp::Logger::Verbosity::WARN});

    std::error_code ec;
    if (!std::filesystem::exists(savedir_, ec)) {
      std::filesystem::create_directories(savedir_, ec);
    }
  }

  virtual ~Cart() {
    logger_.info("Base dtor");
    Tab5Emu::get().release_video_buffers();
  }

  std::string get_rom_filename() const { return info_.rom_path; }

  virtual void reset() { logger_.info("Base reset"); }

  virtual void load() {
    logger_.info("Base loading...");
    copy_file(get_screenshot_path(true), get_paused_image_path());
  }

  virtual void save() {
    logger_.info("Base saving...");
    copy_file(get_paused_image_path(), get_screenshot_path(true));
  }

  /// Save the current (native resolution) frame to a file as
  /// [w:u16be][h:u16be][RGB565 pixels]
  virtual bool screenshot(std::string_view filename) {
    logger_.info("Base screenshot: {}", filename);
    auto [width, height] = get_video_size();
    std::span<uint8_t> frame = get_video_buffer();
    if (frame.empty()) {
      return false;
    }
    uint8_t header[4] = {(uint8_t)(width >> 8), (uint8_t)(width & 0xFF), (uint8_t)(height >> 8),
                         (uint8_t)(height & 0xFF)};
    std::ofstream file(filename.data(), std::ios::binary);
    if (!file.is_open()) {
      logger_.error("Failed to open file: {}", filename);
      return false;
    }
    file.write((char *)header, sizeof(header));
    file.write((char *)frame.data(), frame.size());
    file.close();
    return true;
  }

  virtual bool is_running() const { return running_; }

  /// Run one iteration of the game loop; returns false once the user quits.
  virtual bool run() {
    running_ = true;
    auto &emu = Tab5Emu::get();
    // the menu is requested by the boot button, by the touch "menu" hot corner
    // or by start+select on a gamepad
    auto state = emu.gamepad_state();
    bool show_menu = emu.menu_requested() || (state.start && state.select);
    if (show_menu) {
      logger_.info("Menu pressed!");
      pre_menu();
      screenshot(get_paused_image_path());
      menu_->resume();
      while (!menu_->is_paused()) {
        using namespace std::chrono_literals;
        std::this_thread::sleep_for(100ms);
      }
      // taps on the menu also reach the in-game touch handler (and its
      // "menu" hot corner); drop any request they generated
      (void)emu.menu_requested();
      emu.clear_screen();
      if (running_)
        post_menu();
    }
    return running_;
  }

protected:
  static constexpr size_t SCREEN_WIDTH = Tab5Emu::lcd_width();
  static constexpr size_t SCREEN_HEIGHT = Tab5Emu::lcd_height();
  static inline const std::string FS_PREFIX = Tab5Emu::mount_point;
  static inline const std::string SAVE_DIR = "saves";

  static void copy_file(const std::string &from, const std::string &to) {
    std::error_code ec;
    if (!std::filesystem::exists(from, ec)) {
      return;
    }
    if (std::filesystem::exists(to, ec)) {
      std::filesystem::remove(to, ec);
    }
    std::ifstream src(from, std::ios::binary);
    std::ofstream dst(to, std::ios::binary);
    dst << src.rdbuf();
  }

  virtual void on_menu_action(Menu::Action action) {
    switch (action) {
    case Menu::Action::RESUME:
      menu_->pause();
      break;
    case Menu::Action::RESET:
      reset();
      menu_->pause();
      break;
    case Menu::Action::QUIT:
      running_ = false;
      menu_->pause();
      break;
    case Menu::Action::SAVE:
      save();
      break;
    case Menu::Action::LOAD:
      load();
      break;
    default:
      break;
    }
  }

  virtual std::string get_save_extension() const { return ".sav"; }
  std::string get_screenshot_extension() const { return ".bin"; }

  virtual void pre_menu() {}
  virtual void post_menu() { handle_video_setting(); }

  virtual std::pair<size_t, size_t> get_video_size() const { return {SCREEN_WIDTH, SCREEN_HEIGHT}; }
  virtual std::span<uint8_t> get_video_buffer() const { return {}; }

  virtual void set_original_video_setting() = 0;
  virtual void set_fit_video_setting() = 0;
  virtual void set_fill_video_setting() = 0;

  /// Helpers for the video settings of a fixed native size (emulator cores):
  /// original = largest integer scale that fits, fit = largest scale that
  /// keeps the aspect ratio (in the PPA's 1/16 steps), fill = whole screen.
  void display_original(size_t w, size_t h) {
    const size_t s = std::max<size_t>(1, std::min(SCREEN_WIDTH / w, SCREEN_HEIGHT / h));
    Tab5Emu::get().display_size(w * s, h * s);
  }
  void display_fit(size_t w, size_t h) {
    float s = std::min(static_cast<float>(SCREEN_WIDTH) / w, static_cast<float>(SCREEN_HEIGHT) / h);
    s = std::floor(s * 16.0f) / 16.0f;
    Tab5Emu::get().display_size(static_cast<size_t>(w * s), static_cast<size_t>(h * s));
  }
  void display_fill() { Tab5Emu::get().display_size(SCREEN_WIDTH, SCREEN_HEIGHT); }

  virtual void handle_video_setting() {
    switch (Tab5Emu::get().video_setting()) {
    case VideoSetting::ORIGINAL:
      set_original_video_setting();
      break;
    case VideoSetting::FIT:
      set_fit_video_setting();
      break;
    case VideoSetting::FILL:
      set_fill_video_setting();
      break;
    default:
      break;
    }
  }

  int get_selected_save_slot() const { return menu_->get_selected_slot(); }

  std::string get_save_path(bool bypass_exist_check = false) const {
    namespace fs = std::filesystem;
    auto save_path = savedir_ + "/" + fs::path(get_rom_filename()).stem().string() +
                     fmt::format("_{}", get_selected_save_slot()) + get_save_extension();
    std::error_code ec;
    if (bypass_exist_check || fs::exists(save_path, ec)) {
      return save_path;
    }
    return "";
  }

  std::string get_paused_image_path() const { return savedir_ + "/paused" + get_screenshot_extension(); }

  std::string get_screenshot_path(bool bypass_exist_check = false) const {
    auto save_path = get_save_path(bypass_exist_check);
    if (!save_path.empty()) {
      return save_path + get_screenshot_extension();
    }
    return "";
  }

  std::atomic<bool> running_{false};
  size_t rom_size_bytes_{0};
  uint8_t *romdata_{nullptr};
  RomInfo info_;
  std::string savedir_;
  std::unique_ptr<Menu> menu_;
  espp::Logger logger_;
};

#pragma once

#include <filesystem>

#include "cart.hpp"
#if defined(ENABLE_SCUMMVM)
#include "scummvm_port.hpp"
#endif

/// A game ScummVM runs (Star Trek: 25th Anniversary, ...): the ROM is a marker
/// file (*.scummvm) in the game's folder, and ScummVM detects the game there.
/// The engine runs in its own task; the pause menu parks it.
class ScummvmCart : public Cart {
public:
  explicit ScummvmCart(const Cart::Config &config)
      : Cart(config) {
    handle_video_setting();
#if defined(ENABLE_SCUMMVM)
    const auto dir = std::filesystem::path(get_rom_filename()).parent_path().string();
    if (!scummvm_start(dir)) {
      logger_.error("ScummVM could not start {}", dir);
      running_ = false;
    }
#endif
  }

  ~ScummvmCart() override {
#if defined(ENABLE_SCUMMVM)
    scummvm_stop();
#endif
  }

  void load() override {
#if defined(ENABLE_SCUMMVM)
    const auto path = get_save_path();
    if (path.empty() || !scummvm_load(path, get_selected_save_slot())) {
      logger_.warn("ScummVM: no state loaded from slot {}", get_selected_save_slot());
      return;
    }
#endif
    Cart::load();
  }

  void save() override {
#if defined(ENABLE_SCUMMVM)
    if (!scummvm_save(get_save_path(true), get_selected_save_slot())) {
      logger_.warn("ScummVM: the game can't save now");
      return;
    }
#endif
    Cart::save();
  }

  bool run() override {
#if defined(ENABLE_SCUMMVM)
    // the game quit (from its own menu) or failed: back to the launcher
    if (!scummvm_running()) {
      const auto status = scummvm_status();
      if (!status.empty())
        logger_.error("ScummVM: {}", status);
      running_ = false;
      return false;
    }
#endif
    // the screen's size is only known once the engine has set it up (and a
    // game can change it): the video setting follows it
    if (get_video_size() != video_size_)
      handle_video_setting();
    std::this_thread::sleep_for(std::chrono::milliseconds(16));
    return Cart::run();
  }

protected:
  bool uses_rom_folder() const override { return true; }

  void capture_hold(bool hold) override {
#if defined(ENABLE_SCUMMVM)
    if (hold)
      scummvm_pause();
    else
      scummvm_resume();
#endif
  }

  void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_SCUMMVM)
    scummvm_pause();
#endif
  }

  void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_SCUMMVM)
    scummvm_resume();
#endif
  }

  std::pair<size_t, size_t> get_video_size() const override {
#if defined(ENABLE_SCUMMVM)
    const auto [w, h] = scummvm_video_size();
    return {static_cast<size_t>(w), static_cast<size_t>(h)};
#else
    return {320, 200};
#endif
  }

  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_SCUMMVM)
    return scummvm_video_buffer();
#else
    return {};
#endif
  }

  void handle_video_setting() override {
    video_size_ = get_video_size();
    // (not set up yet: 0x0)
    if (video_size_.first == 0 || video_size_.second == 0)
      return;
    Cart::handle_video_setting();
  }

  // the DOS games' 320x200 was drawn for a 4:3 screen
  void set_original_video_setting() override {
    const auto [w, h] = get_video_size();
    display_original(w, h * 6 / 5);
  }
  void set_fit_video_setting() override {
    const auto [w, h] = get_video_size();
    display_fit(w, h * 6 / 5);
  }
  void set_fill_video_setting() override { display_fill(); }

  std::string get_save_extension() const override { return "_scummvm.sav"; }

  std::pair<size_t, size_t> video_size_{0, 0};
};

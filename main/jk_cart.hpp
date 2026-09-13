#pragma once

#include "cart.hpp"
#if defined(ENABLE_JK)
#include "jk.hpp"
#endif

/// Cart for Star Wars: Jedi Knight - Dark Forces II, running on OpenJKDF2.
///
/// The "rom" for this cart is the game's install directory on the SD card
/// (see README): the metadata entry points at `<install>/jk.cd` (or any
/// marker file inside the install dir); the directory containing it is used
/// as the engine's working directory.
class JkCart : public Cart {
public:
  explicit JkCart(const Cart::Config &config)
      : Cart(config) {
    handle_video_setting();
    init();
  }

  ~JkCart() override { deinit(); }

  void reset() override {
    Cart::reset();
#if defined(ENABLE_JK)
    jk::reset();
#endif
  }

  void load() override {
    Cart::load();
#if defined(ENABLE_JK)
    jk::load(get_save_path(), get_selected_save_slot());
#endif
  }

  void save() override {
    Cart::save();
#if defined(ENABLE_JK)
    jk::save(get_save_path(true), get_selected_save_slot());
#endif
  }

  void init() {
#if defined(ENABLE_JK)
    auto game_dir = std::filesystem::path(get_rom_filename()).parent_path().string();
    jk::init({.game_dir = game_dir, .save_dir = savedir_});
#endif
  }

  void deinit() {
#if defined(ENABLE_JK)
    jk::deinit();
#endif
  }

  bool run() override {
#if defined(ENABLE_JK)
    if (!jk::run_frame()) {
      // the engine asked to exit (e.g. the user quit from the in-game menu)
      running_ = false;
      return false;
    }
#endif
    return Cart::run();
  }

protected:
  void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_JK)
    jk::pause();
#endif
  }

  void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_JK)
    jk::resume();
#endif
  }

  std::pair<size_t, size_t> get_video_size() const override {
#if defined(ENABLE_JK)
    return jk::video_size();
#else
    return {0, 0};
#endif
  }

  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_JK)
    return jk::video_buffer_rgb565();
#else
    return {};
#endif
  }

  void set_original_video_setting() override {
#if defined(ENABLE_JK)
    auto [w, h] = jk::video_size();
    // "original" is a 2x integer scale of the internal resolution, centered
    Tab5Emu::get().display_size(w * 2, h * 2);
#endif
  }

  void set_fit_video_setting() override {
#if defined(ENABLE_JK)
    auto [w, h] = jk::video_size();
    float scale = static_cast<float>(SCREEN_HEIGHT) / static_cast<float>(h);
    Tab5Emu::get().display_size(static_cast<size_t>(w * scale), SCREEN_HEIGHT);
#endif
  }

  void set_fill_video_setting() override {
#if defined(ENABLE_JK)
    Tab5Emu::get().display_size(SCREEN_WIDTH, SCREEN_HEIGHT);
#endif
  }

  std::string get_save_extension() const override { return "_jk.sav"; }
};

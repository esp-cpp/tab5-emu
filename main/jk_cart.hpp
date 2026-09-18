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
#if defined(ENABLE_JK)
    // only touch the pause image when the engine took the save
    if (jk::load(get_save_path(), get_selected_save_slot())) {
      Cart::load();
    }
#else
    Cart::load();
#endif
  }

  void save() override {
#if defined(ENABLE_JK)
    if (jk::save(get_save_path(true), get_selected_save_slot())) {
      Cart::save();
    }
#else
    Cart::save();
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

  // The engine presents frames of varying size (640x480 menus, the internal
  // resolution in-game); the jk component applies the setting to whatever
  // it is currently presenting.
  void set_original_video_setting() override {
#if defined(ENABLE_JK)
    jk::apply_video_setting();
#endif
  }
  void set_fit_video_setting() override {
#if defined(ENABLE_JK)
    jk::apply_video_setting();
#endif
  }
  void set_fill_video_setting() override {
#if defined(ENABLE_JK)
    jk::apply_video_setting();
#endif
  }

  std::string get_save_extension() const override { return "_jk.sav"; }
};

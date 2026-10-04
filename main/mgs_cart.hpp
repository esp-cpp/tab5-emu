#pragma once

#include "cart.hpp"
#if defined(ENABLE_MGS)
#include "mgs.hpp"
#endif

/// Cart for Metal Gear Solid, running natively (the decompilation on psyz).
///
/// The "rom" is the directory of disc files that port/extract_disc.py
/// produces (STAGE.DIR, RADIO.DAT, FACE.DAT, ...): the metadata entry points
/// at `<dir>/STAGE.DIR`, and the directory must be called `MGS` (any case),
/// because the game addresses its files as "cdrom:\MGS\NAME".
class MgsCart : public Cart {
public:
  explicit MgsCart(const Cart::Config &config)
      : Cart(config) {
    handle_video_setting();
#if defined(ENABLE_MGS)
    const auto data_dir = std::filesystem::path(get_rom_filename()).parent_path().string();
    if (!mgs::init({.data_dir = data_dir})) {
      logger_.error("could not start Metal Gear Solid");
      running_ = false;
    }
#endif
  }

  ~MgsCart() override {
#if defined(ENABLE_MGS)
    mgs::deinit();
#endif
  }

  // Memory-card saves are not implemented by the port; the menu's save / load
  // do nothing beyond the base class bookkeeping.
  void reset() override {
    Cart::reset();
#if defined(ENABLE_MGS)
    if (!mgs::reset()) {
      logger_.error("could not restart Metal Gear Solid");
      running_ = false;
    }
#endif
  }
  void load() override { logger_.warn("load: the port has no memory card"); }
  void save() override { logger_.warn("save: the port has no memory card"); }

  bool run() override {
#if defined(ENABLE_MGS)
    if (!mgs::running()) {
      running_ = false;
      return false;
    }
#endif
    // the game runs on its own tasks; this loop only polls for the menu, and
    // it shares core 0 with them
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(10ms);
    return Cart::run();
  }

protected:
  void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_MGS)
    mgs::pause();
#endif
  }

  void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_MGS)
    mgs::resume();
#endif
  }

  static constexpr size_t MGS_WIDTH = 320;
  static constexpr size_t MGS_HEIGHT = 240;

  void set_original_video_setting() override { display_original(MGS_WIDTH, MGS_HEIGHT); }
  void set_fit_video_setting() override { display_fit(MGS_WIDTH, MGS_HEIGHT); }
  void set_fill_video_setting() override { display_fill(); }
  std::pair<size_t, size_t> get_video_size() const override { return {MGS_WIDTH, MGS_HEIGHT}; }
  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_MGS)
    return mgs::video_buffer_rgb565();
#else
    return {};
#endif
  }

  std::string get_save_extension() const override { return "_mgs.sav"; }
};

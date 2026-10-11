#pragma once

#include "cart.hpp"
#if defined(ENABLE_TPM)
#include "tpm.hpp"
#endif

/// Cart for Star Wars Episode I: The Phantom Menace (PSX), native.
///
/// The metadata entry points at `<dir>/SLUS_008.84`; `<dir>` holds the disc
/// image (.bin, raw sectors). The game itself is tpm-decomp's static
/// recompilation of the executable (components/tpm). Without it, the core
/// shows the game's load screens from extracted disc files.
class TpmCart : public Cart {
public:
  explicit TpmCart(const Cart::Config &config)
      : Cart(config) {
    handle_video_setting();
#if defined(ENABLE_TPM)
    const auto data_dir = std::filesystem::path(get_rom_filename()).parent_path().string();
    if (!tpm::init({.data_dir = data_dir})) {
      logger_.error("could not start The Phantom Menace");
      running_ = false;
    }
#endif
  }

  ~TpmCart() override {
#if defined(ENABLE_TPM)
    tpm::deinit();
#endif
  }

  void reset() override {
    Cart::reset();
#if defined(ENABLE_TPM)
    if (!tpm::reset()) {
      running_ = false;
    }
#endif
  }

  bool run() override {
#if defined(ENABLE_TPM)
    if (!tpm::update()) {
      running_ = false;
      return false;
    }
#endif
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(16ms);
    return Cart::run();
  }

protected:
  void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_TPM)
    tpm::pause();
#endif
  }

  void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_TPM)
    tpm::resume();
#endif
  }

  static constexpr size_t TPM_WIDTH = 320;
  static constexpr size_t TPM_HEIGHT = 240;

  void set_original_video_setting() override { display_original(TPM_WIDTH, TPM_HEIGHT); }
  void set_fit_video_setting() override { display_fit(TPM_WIDTH, TPM_HEIGHT); }
  void set_fill_video_setting() override { display_fill(); }
  std::pair<size_t, size_t> get_video_size() const override { return {TPM_WIDTH, TPM_HEIGHT}; }
  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_TPM)
    return tpm::video_buffer_rgb565();
#else
    return {};
#endif
  }

  std::string get_save_extension() const override { return "_tpm.sav"; }
};

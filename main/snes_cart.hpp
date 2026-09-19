#pragma once

#include "cart.hpp"

#if defined(ENABLE_SNES)
#include "snes.hpp"
#endif

class SnesCart : public Cart {
public:
  explicit SnesCart(const Cart::Config &config) : Cart(config) {
    handle_video_setting();
    init();
  }
  ~SnesCart() override {
    logger_.info("~SnesCart()");
    deinit();
  }
  void reset() override {
    Cart::reset();
#if defined(ENABLE_SNES)
    reset_snes();
#endif
  }
  void load() override {
    Cart::load();
#if defined(ENABLE_SNES)
    load_snes(get_save_path());
#endif
  }
  void save() override {
    Cart::save();
#if defined(ENABLE_SNES)
    save_snes(get_save_path(true));
#endif
  }
  void init() {
#if defined(ENABLE_SNES)
    init_snes(romdata_, rom_size_bytes_);
#endif
  }
  void deinit() {
#if defined(ENABLE_SNES)
    deinit_snes();
#endif
  }
  bool run() override {
#if defined(ENABLE_SNES)
    run_snes_rom();
#endif
    return Cart::run();
  }

protected:
  static constexpr size_t SNES_WIDTH = 256;
  static constexpr size_t SNES_HEIGHT = 224;
  void set_original_video_setting() override { display_original(SNES_WIDTH, SNES_HEIGHT); }
  void set_fit_video_setting() override { display_fit(SNES_WIDTH, SNES_HEIGHT); }
  void set_fill_video_setting() override { display_fill(); }
  std::pair<size_t, size_t> get_video_size() const override {
#if defined(ENABLE_SNES)
    return get_snes_video_size();
#else
    return {SNES_WIDTH, SNES_HEIGHT};
#endif
  }
  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_SNES)
    return get_snes_video_buffer();
#else
    return {};
#endif
  }
  std::string get_save_extension() const override { return "_snes.sav"; }
};

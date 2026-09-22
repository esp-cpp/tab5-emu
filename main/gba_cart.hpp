#pragma once

#include "cart.hpp"

#if defined(ENABLE_GBA)
#include "gba.hpp"
#endif

class GbaCart : public Cart {
public:
  explicit GbaCart(const Cart::Config &config) : Cart(config) {
    // the 16 MB ROM cache does not fit next to the 6 MB SNES ROM arena
    Tab5Emu::get().release_rom_arena();
    handle_video_setting();
    init();
  }
  ~GbaCart() override {
    logger_.info("~GbaCart()");
    deinit();
    Tab5Emu::get().reserve_rom_arena(); // the heap is clean again
  }
  void reset() override {
    Cart::reset();
#if defined(ENABLE_GBA)
    reset_gba_core();
#endif
  }
  void load() override {
    Cart::load();
#if defined(ENABLE_GBA)
    load_gba(get_save_path());
#endif
  }
  void save() override {
    Cart::save();
#if defined(ENABLE_GBA)
    save_gba(get_save_path(true));
#endif
  }
  void init() {
#if defined(ENABLE_GBA)
    init_gba(get_rom_filename()); // the core reads (and pages) the ROM file itself
#endif
  }
  void deinit() {
#if defined(ENABLE_GBA)
    deinit_gba();
#endif
  }
  bool run() override {
#if defined(ENABLE_GBA)
    run_gba_rom();
#endif
    return Cart::run();
  }

protected:
  static constexpr size_t GBA_WIDTH = 240;
  static constexpr size_t GBA_HEIGHT = 160;
  void set_original_video_setting() override { display_original(GBA_WIDTH, GBA_HEIGHT); }
  void set_fit_video_setting() override { display_fit(GBA_WIDTH, GBA_HEIGHT); }
  void set_fill_video_setting() override { display_fill(); }
  std::pair<size_t, size_t> get_video_size() const override { return {GBA_WIDTH, GBA_HEIGHT}; }
  std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_GBA)
    return get_gba_video_buffer();
#else
    return {};
#endif
  }
  std::string get_save_extension() const override { return "_gba.sav"; }
};

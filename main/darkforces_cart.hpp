#pragma once

#include "cart.hpp"
#if defined(ENABLE_DARKFORCES)
#include "darkforces.hpp"
#endif

class DarkForcesCart : public Cart {
public:
  explicit DarkForcesCart(const Cart::Config& config)
    : Cart(config) {
    handle_video_setting();
    // the core runs its allocator in the box's 4 MB "ROM" block
    Tab5Emu::get().reserve_romdata(4 * 1024 * 1024);
    init();
  }

  virtual ~DarkForcesCart() override {
    deinit();
  }

  // cppcheck-suppress uselessOverride
  virtual void reset() override {
    Cart::reset();
#if defined(ENABLE_DARKFORCES)
    reset_darkforces();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void load() override {
    Cart::load();
#if defined(ENABLE_DARKFORCES)
    load_darkforces(get_save_path(), get_selected_save_slot());
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void save() override {
    Cart::save();
#if defined(ENABLE_DARKFORCES)
    save_darkforces(get_save_path(true), get_selected_save_slot());
#endif
  }

  void init() {
#if defined(ENABLE_DARKFORCES)
    init_darkforces(get_rom_filename(), romdata_, rom_size_bytes_);
#endif
  }

  void deinit() {
#if defined(ENABLE_DARKFORCES)
    deinit_darkforces();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual bool run() override {
#if defined(ENABLE_DARKFORCES)
    run_darkforces_rom();
    // quitting from the game's own menu returns to the emulator menu
    if (darkforces_quit_requested()) {
      running_ = false;
      return false;
    }
#endif
    return Cart::run();
  }

protected:
  // Dark Forces renders at the DOS resolution of 320x200.
  static constexpr size_t DF_WIDTH = 320;
  static constexpr size_t DF_HEIGHT = 200;

  // (the GOBs and LFDs are read as the game goes)
  bool uses_rom_folder() const override { return true; }

  void capture_hold(bool hold) override {
#if defined(ENABLE_DARKFORCES)
    if (hold) {
      pause_darkforces_tasks();
    } else {
      resume_darkforces_tasks();
    }
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_DARKFORCES)
    logger_.info("darkforces::pre_menu()");
    pause_darkforces_tasks();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_DARKFORCES)
    logger_.info("darkforces::post_menu()");
    resume_darkforces_tasks();
#endif
  }

  virtual void set_original_video_setting() override {
#if defined(ENABLE_DARKFORCES)
    display_original(DF_WIDTH, DF_HEIGHT);
#endif
  }

  virtual std::pair<size_t, size_t> get_video_size() const override {
    return std::make_pair(DF_WIDTH, DF_HEIGHT);
  }

  // cppcheck-suppress uselessOverride
  virtual std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_DARKFORCES)
    return get_darkforces_video_buffer();
#else
    return std::span<uint8_t>();
#endif
  }

  virtual void set_fit_video_setting() override {
#if defined(ENABLE_DARKFORCES)
    display_fit(DF_WIDTH, DF_WIDTH * 3 / 4);
#endif
  }

  virtual void set_fill_video_setting() override {
#if defined(ENABLE_DARKFORCES)
    display_fill();
#endif
  }

  virtual std::string get_save_extension() const override {
    return "_darkforces.sav";
  }
};

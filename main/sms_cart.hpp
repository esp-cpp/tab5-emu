#pragma once

#include "cart.hpp"
#if defined(ENABLE_SMS)
#include "sms.hpp"
#endif

class SmsCart : public Cart {
public:
  // an emulator core: a state loads right after the game starts
  virtual bool supports_resume() const override { return true; }

  explicit SmsCart(const Cart::Config& config)
    : Cart(config) {
    handle_video_setting();
    init();
  }

  virtual ~SmsCart() override {
    logger_.info("~SmsCart()");
    deinit();
  }

  // cppcheck-suppress uselessOverride
  virtual void reset() override {
    Cart::reset();
#if defined(ENABLE_SMS)
    reset_sms();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void load() override {
#if defined(ENABLE_SMS)
    // (only a state the core took counts as loaded)
    if (load_sms(get_save_path())) Cart::load();
#else
    Cart::load();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void save() override {
#if defined(ENABLE_SMS)
    // (only a state that reached the card counts as saved)
    if (save_sms(get_save_path(true))) Cart::save();
#else
    Cart::save();
#endif
  }

  void init() {
#if defined(ENABLE_SMS)
    switch (info_.platform) {
    case Emulator::SEGA_MASTER_SYSTEM:
      logger_.info("sms::init()");
      init_sms(romdata_, rom_size_bytes_);
      break;
    case Emulator::SEGA_GAME_GEAR:
      logger_.info("gg::init()");
      init_gg(romdata_, rom_size_bytes_);
      break;
    default:
      logger_.error("unknown platform");
      return;
    }
#endif
  }

  void deinit() {
#if defined(ENABLE_SMS)
    deinit_sms();
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual bool run() override {
#if defined(ENABLE_SMS)
    run_sms_rom();
#endif
    return Cart::run();
  }

protected:
  // SMS
  static constexpr size_t SMS_WIDTH = 256;
  static constexpr size_t SMS_HEIGHT = 192;
  static constexpr size_t GG_WIDTH = 160;
  static constexpr size_t GG_HEIGHT = 144;

  // cppcheck-suppress uselessOverride
  virtual void pre_menu() override {
    Cart::pre_menu();
#if defined(ENABLE_SMS)
    logger_.info("sms::pre_menu()");
#endif
  }

  // cppcheck-suppress uselessOverride
  virtual void post_menu() override {
    Cart::post_menu();
#if defined(ENABLE_SMS)
    logger_.info("sms::post_menu()");
#endif
  }

  virtual void set_original_video_setting() override {
#if defined(ENABLE_SMS)
    const bool sms = info_.platform == Emulator::SEGA_MASTER_SYSTEM;
    const size_t width = sms ? SMS_WIDTH : GG_WIDTH;
    const size_t height = sms ? SMS_HEIGHT : GG_HEIGHT;
    display_original(width, height);
#endif
  }

  virtual std::pair<size_t, size_t> get_video_size() const override {
    auto height = info_.platform == Emulator::SEGA_MASTER_SYSTEM ? SMS_HEIGHT : GG_HEIGHT;
    auto width = info_.platform == Emulator::SEGA_MASTER_SYSTEM ? SMS_WIDTH : GG_WIDTH;
    return std::make_pair(width, height);
  }

  // cppcheck-suppress uselessOverride
  virtual std::span<uint8_t> get_video_buffer() const override {
#if defined(ENABLE_SMS)
    return get_sms_video_buffer();
#else
    return std::span<uint8_t>();
#endif
  }

  virtual void set_fit_video_setting() override {
#if defined(ENABLE_SMS)
    const bool sms = info_.platform == Emulator::SEGA_MASTER_SYSTEM;
    const size_t width = sms ? SMS_WIDTH : GG_WIDTH;
    const size_t height = sms ? SMS_HEIGHT : GG_HEIGHT;
    display_fit(width, height);
#endif
  }

  virtual void set_fill_video_setting() override {
#if defined(ENABLE_SMS)
    display_fill();
#endif
  }

  virtual std::string get_save_extension() const override {
    switch (info_.platform) {
    case Emulator::SEGA_MASTER_SYSTEM:
      return "_sms.sav";
    case Emulator::SEGA_GAME_GEAR:
      return "_gg.sav";
    default:
      return Cart::get_save_extension();
    }
  }
};

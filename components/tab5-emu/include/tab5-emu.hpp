#pragma once

#include <sdkconfig.h>

#include <atomic>
#include <memory>
#include <mutex>
#include <span>
#include <string>
#include <vector>

#include <esp_err.h>
#include <sdmmc_cmd.h>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>

#include "driver/ppa.h"

#include "m5stack-tab5.hpp"

#include "base_component.hpp"
#include "event_manager.hpp"
#include "task.hpp"

#include "gamepad_state.hpp"
#include "video_setting.hpp"

/// Event topics published by the HAL (payloads are empty / informational)
static inline const std::string volume_changed_topic = "tab5-emu/volume";
static inline const std::string battery_topic = "tab5-emu/battery";
static inline const std::string menu_button_topic = "tab5-emu/menu";

/// Hardware abstraction layer for the tab5-emu project, wrapping the espp
/// M5Stack Tab5 BSP. The API intentionally mirrors esp-box-emu's BoxEmu so
/// that game / emulator glue code can be shared between the two projects.
class Tab5Emu : public espp::BaseComponent {
public:
  using Bsp = espp::M5StackTab5;
  using Pixel = Bsp::Pixel;
  using TouchpadData = Bsp::TouchpadData;
  using touch_callback_t = Bsp::touch_callback_t;

  /// Logical (landscape) screen size the games see
  static constexpr size_t lcd_width() { return Bsp::display_height(); }  // 1280
  static constexpr size_t lcd_height() { return Bsp::display_width(); }  // 720

  static constexpr char mount_point[] = "/sdcard";

  /// Access the singleton
  static Tab5Emu &get() {
    static Tab5Emu instance;
    return instance;
  }

  Tab5Emu(const Tab5Emu &) = delete;
  Tab5Emu &operator=(const Tab5Emu &) = delete;
  Tab5Emu(Tab5Emu &&) = delete;
  Tab5Emu &operator=(Tab5Emu &&) = delete;

  /// Initialize the Tab5 hardware: IO expanders, LCD, LVGL display, touch and
  /// the BOOT button.
  bool initialize_tab5();

  /////////////////////////////////////////////////////////////////////////////
  // uSD Card
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_sdcard();
  sdmmc_card_t *sdcard() const;

  /////////////////////////////////////////////////////////////////////////////
  // Memory (shared "cartridge" buffer, for emulator cores that want the whole
  // rom in RAM)
  /////////////////////////////////////////////////////////////////////////////

  size_t copy_file_to_romdata(const std::string &filename);
  uint8_t *romdata() const { return romdata_; }
  void free_romdata();

  /////////////////////////////////////////////////////////////////////////////
  // Audio
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_audio();
  bool is_muted() const { return Bsp::get().is_muted(); }
  void mute(bool v);
  void volume(float volume);
  float volume() const { return Bsp::get().volume(); }
  void audio_sample_rate(uint32_t rate) { Bsp::get().audio_sample_rate(rate); }
  uint32_t audio_sample_rate() const { return Bsp::get().audio_sample_rate(); }
  size_t audio_buffer_size() const { return Bsp::get().audio_buffer_size(); }
  void play_audio(const uint8_t *data, size_t size) { Bsp::get().play_audio(data, size); }
  void play_audio(std::span<const uint8_t> data) { Bsp::get().play_audio(data); }

  /////////////////////////////////////////////////////////////////////////////
  // Display / brightness
  /////////////////////////////////////////////////////////////////////////////

  void brightness(float v) { Bsp::get().brightness(v); }
  float brightness() const { return Bsp::get().brightness(); }
  /// The LVGL display (for the GUI / menu components)
  std::shared_ptr<espp::Display<Pixel>> display() const { return display_; }

  /////////////////////////////////////////////////////////////////////////////
  // Input
  /////////////////////////////////////////////////////////////////////////////

  /// Initialize the input sources (touch virtual gamepad; later USB HID)
  bool initialize_input();
  /// Current gamepad state (merged from all input sources)
  GamepadState gamepad_state();
  /// Latest touch data (in logical / landscape coordinates)
  TouchpadData touchpad_data() const { return Bsp::get().touchpad_data(); }
  /// True once if the user asked for the in-game menu (BOOT button, or the
  /// touch hot-corner). Reading it clears the request.
  bool menu_requested() { return menu_requested_.exchange(false); }
  bool button_state() const { return Bsp::get().button_state(); }

  /////////////////////////////////////////////////////////////////////////////
  // Battery
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_battery();
  Bsp::BatteryStatus battery_status() const { return Bsp::get().get_battery_status(); }

  /////////////////////////////////////////////////////////////////////////////
  // Video
  //
  // Games render into their own (native resolution) frame buffer, either 8bpp
  // paletted or RGB565, and hand it to push_frame(). The video task converts
  // it to RGB565, then uses the PPA to scale + rotate it directly into the
  // MIPI-DSI panel's frame buffer at the requested display size (centered).
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_video();
  /// Fill the whole panel with black
  void clear_screen();
  /// Set the on-screen size (in logical landscape pixels) the native frame is
  /// scaled to. It is centered on the screen.
  void display_size(size_t width, size_t height);
  /// Set the native (game) frame size; pitch is in pixels (defaults to width)
  void native_size(size_t width, size_t height, int pitch = -1);
  const uint16_t *palette() const { return palette_; }
  /// Set the palette (RGB565) used to convert 8bpp frames; nullptr for RGB565
  /// native frames
  void palette(const uint16_t *palette, size_t size = 256);
  /// Queue a frame for display. The frame must stay valid until the next
  /// push_frame() call. Non-blocking; drops the frame if the previous one is
  /// still being processed.
  void push_frame(const void *frame);
  /// Block until the previously pushed frame has been presented
  void wait_frame();
  VideoSetting video_setting() const { return video_setting_; }
  void video_setting(VideoSetting setting) { video_setting_ = setting; }
  /// Native frame size currently configured
  std::pair<size_t, size_t> native_size() const { return {native_width_, native_height_}; }

protected:
  Tab5Emu();

  bool has_palette() const { return palette_ != nullptr; }
  bool video_task_callback(std::mutex &m, std::condition_variable &cv, bool &task_notified);
  bool ensure_rgb_frame();
  bool blit_rgb_frame();
  void *dpi_frame_buffer();
  void on_touch(const TouchpadData &data);

  // touch virtual gamepad
  struct TouchGamepad {
    std::mutex mutex;
    TouchpadData last{};
    GamepadState state{};
  } touch_gamepad_;

  // sdcard
  sdmmc_card_t *sdcard_{nullptr};

  // memory
  uint8_t *romdata_{nullptr};
  size_t romdata_size_{0};

  // display
  std::shared_ptr<espp::Display<Pixel>> display_;
  std::atomic<bool> menu_requested_{false};

  // video
  std::atomic<VideoSetting> video_setting_{VideoSetting::FIT};
  std::unique_ptr<espp::Task> video_task_;
  QueueHandle_t video_queue_{nullptr};
  SemaphoreHandle_t frame_done_{nullptr};
  ppa_client_handle_t ppa_client_{nullptr};
  void *dpi_fb_{nullptr};
  size_t dpi_fb_bytes_{0};
  uint16_t *rgb_frame_{nullptr};
  size_t rgb_frame_bytes_{0};
  size_t native_width_{0};
  size_t native_height_{0};
  size_t native_pitch_{0};
  size_t display_width_{lcd_width()};
  size_t display_height_{lcd_height()};
  const uint16_t *palette_{nullptr};
  size_t palette_size_{256};
};

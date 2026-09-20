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
#include "usb_device.hpp"
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
  /// A permanent PSRAM block reserved at boot for cores that need one big
  /// contiguous ROM buffer (the SNES core wants 4 MB); the general heap gets
  /// too fragmented by GUI allocations to guarantee that later. Returns
  /// nullptr if `bytes` exceeds the reservation.
  uint8_t *rom_arena(size_t bytes) const { return (rom_arena_ && bytes <= ROM_ARENA_BYTES) ? rom_arena_ : nullptr; }
  static constexpr size_t rom_arena_size() { return ROM_ARENA_BYTES; }
  /// Carts that need the PSRAM more than a ROM buffer (JK's level load takes
  /// ~10 MB) release the arena while they run and reserve it again as soon
  /// as they are done, before the GUI can fragment the heap.
  void release_rom_arena();
  bool reserve_rom_arena();

  /////////////////////////////////////////////////////////////////////////////
  // Audio
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_audio();
  bool is_muted() const { return Bsp::get().is_muted(); }
  void mute(bool v);
  void volume(float volume);
  float volume() const { return Bsp::get().volume(); }
  /// Sample rate of the audio the game produces. The DAC stays at its fixed
  /// rate (48 kHz); other rates are resampled in play_audio(). Changing the
  /// I2S clock at runtime (the esp-box-emu approach) is avoided: the clock is
  /// shared with the microphone channel and any mismatch between the game's
  /// production rate and the DAC drifts the queue until it drops audio.
  void audio_sample_rate(uint32_t rate);
  uint32_t audio_sample_rate() const { return audio_source_rate_; }
  uint32_t audio_hardware_rate() const { return Bsp::get().audio_sample_rate(); }
  size_t audio_buffer_size() const { return Bsp::get().audio_buffer_size(); }
  /// Queue 16-bit interleaved stereo PCM (non-blocking); returns the bytes
  /// actually queued, which is less than size when the queue is full
  size_t play_audio(const uint8_t *data, size_t size);
  size_t play_audio(std::span<const uint8_t> data) { return play_audio(data.data(), data.size()); }
  /// How long play_audio() may wait for queue space before giving up (0 =
  /// never wait, drop instead). Waiting paces a game that produces audio
  /// slightly faster than the DAC consumes it.
  void audio_max_wait_ms(uint32_t ms) { audio_max_wait_ms_ = ms; }

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
  /// Latest touch data (in logical / landscape coordinates); a finger is
  /// down when num_touch_points > 0 (btn_state is the GT911's home button)
  TouchpadData touchpad_data() const {
    std::lock_guard<std::mutex> lk(touch_gamepad_.mutex);
    return touch_gamepad_.last;
  }
  /// True once if the user asked for the in-game menu (BOOT button, or the
  /// touch hot-corner). Reading it clears the request.
  bool menu_requested() { return menu_requested_.exchange(false); }

  /////////////////////////////////////////////////////////////////////////////
  // USB host: HID keyboard / mouse on the USB-A port (hubs supported)
  /////////////////////////////////////////////////////////////////////////////

  /// Start the USB host library and the HID class driver. Not fatal when it
  /// fails (the touch controls keep working).
  bool initialize_usb_host();
  /// Keyboard state: bitmap over HID usage IDs (0..255, incl. the modifiers
  /// at 0xE0..0xE7). Bit (usage & 7) of keys[usage >> 3].
  struct KeyboardState {
    uint8_t keys[32];
  };
  KeyboardState keyboard_state() const;
  struct MouseState {
    int dx{0};          ///< motion accumulated since the last take_mouse_motion()
    int dy{0};
    int wheel{0};
    uint8_t buttons{0}; ///< bit0 left, bit1 right, bit2 middle
  };
  /// Current buttons and the pending (not yet taken) motion
  MouseState mouse_state() const;
  /// Return and clear the accumulated mouse motion
  MouseState take_mouse_motion();
  bool usb_keyboard_present() const { return usb_keyboards_ > 0; }
  bool usb_mouse_present() const { return usb_mice_ > 0; }
  /// Stop the USB host library (keyboard / mouse stop working). The USB
  /// controller is shared with device mode (see initialize_usb_msc()).
  void deinitialize_usb_host();
  bool is_usb_host_enabled() const { return usb_hid_ != nullptr; }

  /////////////////////////////////////////////////////////////////////////////
  // USB mass storage: expose the SD card to a PC as a USB drive
  //
  // The card's FAT volume is unmounted here while a PC has it, so nothing on
  // the device may use /sdcard in that time (the GUI shows a "USB drive" mode
  // and reloads the ROM list afterwards). The P4 has one USB OTG controller,
  // so the USB host (keyboard / mouse) is stopped while the drive is on and
  // restarted afterwards.
  /////////////////////////////////////////////////////////////////////////////

  bool initialize_usb_msc();
  void deinitialize_usb_msc();
  /// Give the USB-C pins back to the serial console if a previous run left
  /// them on the OTG controller (called once at construction).
  void usb_msc_restore_console();
  bool is_usb_msc_enabled() const { return usb_device_ != nullptr; }
  /// True while a PC holds the card (the volume is not mounted here)
  bool usb_msc_host_has_card() const;
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
  /// Free the per-cart video buffers (conversion tile, frame_buffer0/1);
  /// called when a cart ends. They are re-created on the next frame.
  void release_video_buffers();
  /// Set the on-screen size (in logical landscape pixels) the native frame is
  /// scaled to. It is centered on the screen.
  void display_size(size_t width, size_t height);
  /// Set the native (game) frame size; pitch is in pixels (defaults to width)
  void native_size(size_t width, size_t height, int pitch = -1);
  const uint16_t *palette() const { return palette_; }
  /// Set the palette (RGB565) used to convert 8bpp frames; nullptr for RGB565
  /// native frames
  void palette(const uint16_t *palette, size_t size = 256);
  /// Set the size of the optional 8bpp overlay (HUD) that can accompany a
  /// paletted frame; pitch is in pixels (defaults to width). Width/height 0
  /// disables the overlay. The overlay is drawn 1:1 (palette index 0 =
  /// transparent), centered, on top of the native frame after the native frame
  /// has been upscaled by the smallest integer factor that makes it at least as
  /// tall as the overlay; the result is what the PPA then scales to the screen.
  /// This keeps a HUD that is drawn at a higher resolution than the game's
  /// world sharp instead of downsampling it into the world frame.
  void overlay_size(size_t width, size_t height, int pitch = -1);
  /// Queue a frame for display. The frame (and overlay, if any) must stay valid
  /// until the next push_frame() call. Non-blocking; drops the frame if the
  /// previous one is still being processed.
  /// overlay_row_begin/end limit the overlay rows that may contain visible
  /// pixels (rows outside are skipped without being read).
  void push_frame(const void *frame, const void *overlay = nullptr, size_t overlay_row_begin = 0,
                  size_t overlay_row_end = SIZE_MAX);
  /// Video task timing since the last call (accumulated over frames).
  struct VideoStats {
    uint64_t convert_us{0}; ///< palette conversion / upscale / overlay
    uint64_t blit_us{0};    ///< PPA scale + rotate into the panel buffer
    uint32_t frames{0};
    uint32_t tiles{0};
  };
  VideoStats video_stats(bool reset = true);
  /// Block until the previously pushed frame has been presented
  void wait_frame();
  VideoSetting video_setting() const { return video_setting_; }
  void video_setting(VideoSetting setting) { video_setting_ = setting; }
  /// Native frame size currently configured
  std::pair<size_t, size_t> native_size() const { return {native_width_, native_height_}; }
  /// Two RGB565 frame buffers emulator cores can render into and hand to
  /// push_frame() alternately (esp-box-emu convention). Sized for the largest
  /// native emulator frame (FRAME_BUFFER_PIXELS), allocated on first use.
  static constexpr size_t FRAME_BUFFER_PIXELS = 320 * 240;
  uint8_t *frame_buffer0();
  uint8_t *frame_buffer1();

protected:
  Tab5Emu();

  bool has_palette() const { return palette_ != nullptr; }
  bool video_task_callback(std::mutex &m, std::condition_variable &cv, bool &task_notified);
  bool ensure_tile_buffer();
  struct Layout {
    ppa_srm_rotation_angle_t angle;
    bool swap; ///< 90/270: landscape x maps to panel y and vice versa
    float scale_x, scale_y;
    uint32_t out_w, out_h; ///< scaled size in landscape (logical) pixels
    uint32_t off_x, off_y; ///< panel (native) offset of the centered picture
  };
  Layout layout() const;
  /// Convert staging rect [x0,x0+tw)x[y0,y0+th) into the tile buffer
  void convert_tile(const void *frame, const void *overlay, size_t ov_r0, size_t ov_r1, size_t x0, size_t y0,
                    size_t tw, size_t th);
  /// PPA the tile buffer (tw x th) to its place on the panel
  bool blit_tile(const Layout &lo, size_t x0, size_t y0, size_t tw, size_t th);
  /// integer factor the native frame is upscaled by in the staging buffer
  size_t staging_scale() const;
  size_t staging_width() const { return native_width_ * staging_scale(); }
  size_t staging_height() const { return native_height_ * staging_scale(); }

  struct VideoFrame {
    const void *frame;
    const void *overlay;
    size_t overlay_row_begin;
    size_t overlay_row_end;
  };
  void *dpi_frame_buffer();
  void on_touch(const TouchpadData &data);

  // touch virtual gamepad
  struct TouchGamepad {
    mutable std::mutex mutex;
    TouchpadData last{};
    GamepadState state{};
  } touch_gamepad_;

  // sdcard
  sdmmc_card_t *sdcard_{nullptr};

  // memory
  uint8_t *romdata_{nullptr};
  size_t romdata_size_{0};
  static constexpr size_t ROM_ARENA_BYTES = 0x400000 + 0x200 + 0x8000; // snes9x MAX_ROM_SIZE + slack
  uint8_t *rom_arena_{nullptr};

  // display
  std::shared_ptr<espp::Display<Pixel>> display_;
  std::atomic<bool> menu_requested_{false};

  // audio
  uint32_t audio_source_rate_{48000};
  uint32_t audio_max_wait_ms_{30};
  uint32_t audio_phase_{0}; ///< resampler phase (16.16)
  int16_t audio_last_[2]{0, 0}; ///< last source frame (resampler continuity)
  std::vector<uint8_t> audio_resample_buf_;
  uint64_t audio_bytes_sent_{0}, audio_bytes_dropped_{0}, audio_waits_{0};
  int64_t audio_last_report_us_{0};

  // usb hid (see usb_hid.cpp)
  struct UsbHid;
  UsbHid *usb_hid_{nullptr};
  // usb msc (see usb_msc.cpp)
  std::unique_ptr<espp::UsbDevice> usb_device_;
  bool usb_host_was_enabled_{false};
  mutable std::mutex hid_mutex_;
  KeyboardState keyboard_{};
  MouseState mouse_{};
  std::atomic<int> usb_keyboards_{0};
  std::atomic<int> usb_mice_{0};
  void on_hid_keyboard_report(const uint8_t *data, size_t len);
  void on_hid_mouse_report(const uint8_t *data, size_t len);

  // video
  std::atomic<VideoSetting> video_setting_{VideoSetting::FIT};
  std::unique_ptr<espp::Task> video_task_;
  QueueHandle_t video_queue_{nullptr};
  SemaphoreHandle_t frame_done_{nullptr};
  ppa_client_handle_t ppa_client_{nullptr};
  void *dpi_fb_{nullptr};
  size_t dpi_fb_bytes_{0};
  // conversion tile: the staging frame is converted and blitted in tiles
  // small enough to live in internal RAM
  static constexpr size_t TILE_BYTES = 64 * 1024;
  uint16_t *tile_buf_{nullptr};
  size_t tile_buf_bytes_{0};
  uint8_t *frame_buffers_[2]{nullptr, nullptr};
  VideoStats video_stats_{};
  size_t native_width_{0};
  size_t native_height_{0};
  size_t native_pitch_{0};
  size_t overlay_width_{0};
  size_t overlay_height_{0};
  size_t overlay_pitch_{0};
  size_t display_width_{lcd_width()};
  size_t display_height_{lcd_height()};
  const uint16_t *palette_{nullptr};
  size_t palette_size_{256};
};

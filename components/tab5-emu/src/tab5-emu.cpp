#include "tab5-emu.hpp"

#include <cstring>
#include <fstream>

#include <esp_cache.h>
#include <esp_heap_caps.h>
#include <esp_lcd_mipi_dsi.h>

#include "statistics.hpp"

using namespace std::chrono_literals;

// The LVGL rotation that makes the (portrait-native, 720x1280) panel a
// 1280x720 landscape screen. Flip to 270 if the image is upside down for the
// way you hold the Tab5.
static constexpr lv_display_rotation_t LANDSCAPE_ROTATION = LV_DISPLAY_ROTATION_90;

// ---------------------------------------------------------------------------
// Access to protected BSP state.
//
// The espp Tab5 BSP keeps the esp_lcd panel handle protected. We need it to
// get the MIPI-DSI panel's frame buffer so the PPA can scale + rotate game
// frames straight into it (no intermediate copy through
// esp_lcd_panel_draw_bitmap). Forming a pointer-to-member from a derived
// class is the standard-conforming way to reach a protected member of a base
// object; replace with a BSP accessor once espp exposes one.
// ---------------------------------------------------------------------------
namespace {
struct BspAccess : espp::M5StackTab5 {
  static esp_lcd_panel_handle_t panel(espp::M5StackTab5 &bsp) {
    auto pm = &BspAccess::lcd_handles_;
    return (bsp.*pm).panel;
  }
};
} // namespace

Tab5Emu::Tab5Emu()
    : espp::BaseComponent("Tab5Emu", espp::Logger::Verbosity::INFO) {}

/////////////////////////////////////////////////////////////////////////////
// Board
/////////////////////////////////////////////////////////////////////////////

bool Tab5Emu::initialize_tab5() {
  auto &bsp = Bsp::get();
  if (!bsp.initialize_io_expanders()) {
    logger_.error("Failed to initialize IO expanders");
    return false;
  }
  if (!bsp.initialize_lcd()) {
    logger_.error("Failed to initialize LCD");
    return false;
  }
  // one tenth of the screen per LVGL draw buffer (double buffered)
  static constexpr size_t pixel_buffer_size = Bsp::display_width() * Bsp::display_height() / 10;
  if (!bsp.initialize_display(pixel_buffer_size)) {
    logger_.error("Failed to initialize display");
    return false;
  }
  // make the LVGL screen landscape; the BSP's flush rotates via the PPA
  lv_display_set_rotation(lv_display_get_default(), LANDSCAPE_ROTATION);

  if (!bsp.initialize_touch(std::bind_front(&Tab5Emu::on_touch, this))) {
    logger_.error("Failed to initialize touch");
    return false;
  }
  // the BOOT button requests the in-game menu
  if (!bsp.initialize_button([this](const espp::Interrupt::Event &event) {
        if (event.active) {
          menu_requested_ = true;
          espp::EventManager::get().publish(menu_button_topic, {});
        }
      })) {
    logger_.warn("Failed to initialize the BOOT button");
  }
  bsp.brightness(75.0f);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// SD card
/////////////////////////////////////////////////////////////////////////////

bool Tab5Emu::initialize_sdcard() {
  auto &bsp = Bsp::get();
  if (!bsp.initialize_sdcard(Bsp::SdCardConfig{.format_if_mount_failed = false,
                                               .max_files = 16,
                                               .allocation_unit_size = 16 * 1024})) {
    return false;
  }
  sdcard_ = bsp.sdcard();
  uint32_t size_mb = 0, free_mb = 0;
  if (bsp.get_sd_card_info(&size_mb, &free_mb)) {
    logger_.info("SD card: {} MB total, {} MB free", size_mb, free_mb);
  }
  return sdcard_ != nullptr;
}

sdmmc_card_t *Tab5Emu::sdcard() const { return sdcard_; }

/////////////////////////////////////////////////////////////////////////////
// Memory
/////////////////////////////////////////////////////////////////////////////

size_t Tab5Emu::copy_file_to_romdata(const std::string &filename) {
  std::ifstream file(filename, std::ios::binary | std::ios::ate);
  if (!file.is_open()) {
    logger_.error("Could not open {}", filename);
    return 0;
  }
  size_t size = file.tellg();
  file.seekg(0);
  free_romdata();
  romdata_ = static_cast<uint8_t *>(heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  if (!romdata_) {
    logger_.error("Could not allocate {} bytes for {}", size, filename);
    return 0;
  }
  file.read(reinterpret_cast<char *>(romdata_), size);
  romdata_size_ = size;
  return size;
}

void Tab5Emu::free_romdata() {
  if (romdata_) {
    heap_caps_free(romdata_);
    romdata_ = nullptr;
    romdata_size_ = 0;
  }
}

/////////////////////////////////////////////////////////////////////////////
// Audio
/////////////////////////////////////////////////////////////////////////////

bool Tab5Emu::initialize_audio() {
  auto &bsp = Bsp::get();
  if (!bsp.initialize_audio(48000)) {
    return false;
  }
  bsp.mute(false);
  bsp.volume(60.0f);
  return true;
}

void Tab5Emu::mute(bool v) {
  Bsp::get().mute(v);
  espp::EventManager::get().publish(volume_changed_topic, {});
}

void Tab5Emu::volume(float volume) {
  Bsp::get().volume(volume);
  espp::EventManager::get().publish(volume_changed_topic, {});
}

/////////////////////////////////////////////////////////////////////////////
// Battery
/////////////////////////////////////////////////////////////////////////////

bool Tab5Emu::initialize_battery() {
  auto &bsp = Bsp::get();
  if (!bsp.initialize_battery_monitoring()) {
    return false;
  }
  bsp.set_charging_enabled(true);
  return true;
}

/////////////////////////////////////////////////////////////////////////////
// Input
/////////////////////////////////////////////////////////////////////////////

bool Tab5Emu::initialize_input() {
  // Touch is initialized with the board; nothing else yet (USB HID host is
  // planned, see docs/PLAN.md).
  return true;
}

void Tab5Emu::on_touch(const TouchpadData &raw) {
  // convert from the raw (portrait, native) coordinates into the rotated
  // (landscape) coordinates used by LVGL and the games
  auto data = Bsp::get().touchpad_convert(raw);
  std::lock_guard<std::mutex> lk(touch_gamepad_.mutex);
  touch_gamepad_.last = data;

  // Virtual gamepad layout (landscape, 1280x720):
  //   - top-right hot corner (100x100): menu
  //   - left third of the screen: d-pad, by direction from the zone center
  //   - right third: A (lower-right), B (lower-left), X (upper-right),
  //     Y (upper-left); a touch in the bottom strip of the middle third is
  //     START (right half) / SELECT (left half)
  GamepadState state{};
  if (data.num_touch_points > 0) {
    const int w = lcd_width(), h = lcd_height();
    const int x = data.x, y = data.y;
    if (x >= w - 100 && y < 100) {
      menu_requested_ = true;
    } else if (x < w / 3) {
      const int cx = w / 6, cy = h / 2;
      const int dx = x - cx, dy = y - cy;
      constexpr int dead = 40;
      if (dx > dead && std::abs(dx) > std::abs(dy) / 2)
        state.right = 1;
      if (dx < -dead && std::abs(dx) > std::abs(dy) / 2)
        state.left = 1;
      if (dy > dead && std::abs(dy) > std::abs(dx) / 2)
        state.down = 1;
      if (dy < -dead && std::abs(dy) > std::abs(dx) / 2)
        state.up = 1;
    } else if (x >= 2 * w / 3) {
      const bool right_half = x >= (2 * w / 3 + w / 6);
      const bool lower_half = y >= h / 2;
      if (lower_half && right_half)
        state.a = 1;
      else if (lower_half)
        state.b = 1;
      else if (right_half)
        state.x = 1;
      else
        state.y = 1;
    } else if (y >= h - 120) {
      if (x >= w / 2)
        state.start = 1;
      else
        state.select = 1;
    }
  }
  touch_gamepad_.state = state;
}

GamepadState Tab5Emu::gamepad_state() {
  std::lock_guard<std::mutex> lk(touch_gamepad_.mutex);
  return touch_gamepad_.state;
}

/////////////////////////////////////////////////////////////////////////////
// Video
/////////////////////////////////////////////////////////////////////////////

void *Tab5Emu::dpi_frame_buffer() {
  if (dpi_fb_) {
    return dpi_fb_;
  }
  auto panel = BspAccess::panel(Bsp::get());
  if (!panel) {
    logger_.error("No LCD panel handle; was initialize_tab5() called?");
    return nullptr;
  }
  void *fb0 = nullptr;
  if (esp_lcd_dpi_panel_get_frame_buffer(panel, 1, &fb0) != ESP_OK || !fb0) {
    logger_.error("Could not get the DPI panel frame buffer");
    return nullptr;
  }
  dpi_fb_ = fb0;
  dpi_fb_bytes_ = Bsp::display_width() * Bsp::display_height() * sizeof(Pixel);
  logger_.info("DPI frame buffer at {} ({} bytes)", fb0, dpi_fb_bytes_);
  return dpi_fb_;
}

bool Tab5Emu::initialize_video() {
  if (video_task_) {
    return true;
  }
  if (!dpi_frame_buffer()) {
    return false;
  }
  ppa_client_config_t ppa_cfg = {};
  ppa_cfg.oper_type = PPA_OPERATION_SRM;
  ppa_cfg.max_pending_trans_num = 1;
  if (auto err = ppa_register_client(&ppa_cfg, &ppa_client_); err != ESP_OK) {
    logger_.error("Could not register a PPA client: {}", esp_err_to_name(err));
    return false;
  }
  video_queue_ = xQueueCreate(1, sizeof(VideoFrame));
  frame_done_ = xSemaphoreCreateBinary();
  using namespace std::placeholders;
  video_task_ = espp::Task::make_unique({
      .callback = std::bind(&Tab5Emu::video_task_callback, this, _1, _2, _3),
      .task_config = {.name = "video", .stack_size_bytes = 6 * 1024, .priority = 20, .core_id = 1},
  });
  video_task_->start();
  return true;
}

void Tab5Emu::display_size(size_t width, size_t height) {
  display_width_ = std::min(width, lcd_width());
  display_height_ = std::min(height, lcd_height());
  logger_.info("display size: {}x{}", display_width_, display_height_);
  clear_screen();
}

void Tab5Emu::native_size(size_t width, size_t height, int pitch) {
  native_width_ = width;
  native_height_ = height;
  native_pitch_ = pitch > 0 ? pitch : width;
  logger_.info("native size: {}x{} (pitch {})", native_width_, native_height_, native_pitch_);
}

void Tab5Emu::overlay_size(size_t width, size_t height, int pitch) {
  overlay_width_ = width;
  overlay_height_ = height;
  overlay_pitch_ = pitch > 0 ? static_cast<size_t>(pitch) : width;
  logger_.info("overlay size: {}x{} (pitch {})", overlay_width_, overlay_height_, overlay_pitch_);
}

size_t Tab5Emu::staging_scale() const {
  // overlays are only supported on paletted frames
  if (!has_palette() || overlay_width_ == 0 || overlay_height_ == 0 || native_height_ == 0) {
    return 1;
  }
  return std::max<size_t>(1, (overlay_height_ + native_height_ - 1) / native_height_);
}

void Tab5Emu::palette(const uint16_t *palette, size_t size) {
  palette_ = palette;
  palette_size_ = size;
}

void Tab5Emu::clear_screen() {
  auto fb = dpi_frame_buffer();
  if (!fb) {
    return;
  }
  memset(fb, 0, dpi_fb_bytes_);
  esp_cache_msync(fb, dpi_fb_bytes_, ESP_CACHE_MSYNC_FLAG_DIR_C2M);
}

void Tab5Emu::push_frame(const void *frame, const void *overlay) {
  if (!video_queue_) {
    return;
  }
  // overwrite: if the video task hasn't consumed the previous frame yet, the
  // game is running faster than the display and we drop the older one
  VideoFrame vf{frame, overlay};
  xQueueOverwrite(video_queue_, &vf);
}

void Tab5Emu::wait_frame() {
  if (frame_done_) {
    xSemaphoreTake(frame_done_, pdMS_TO_TICKS(100));
  }
}

bool Tab5Emu::ensure_rgb_frame() {
  const size_t needed = staging_width() * staging_height() * sizeof(uint16_t);
  if (needed == 0) {
    return false;
  }
  if (rgb_frame_ && rgb_frame_bytes_ >= needed) {
    return true;
  }
  logger_.info("staging frame: {}x{} ({} bytes)", staging_width(), staging_height(), needed);
  if (rgb_frame_) {
    heap_caps_free(rgb_frame_);
    rgb_frame_ = nullptr;
  }
  // 128-byte aligned so the PPA's cache sync covers exactly this buffer;
  // prefer internal memory for the palette-conversion writes
  rgb_frame_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(128, needed, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!rgb_frame_) {
    rgb_frame_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(128, needed, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  if (!rgb_frame_) {
    logger_.error("Could not allocate {} bytes for the RGB frame", needed);
    return false;
  }
  rgb_frame_bytes_ = needed;
  return true;
}

bool Tab5Emu::blit_rgb_frame() {
  auto fb = dpi_frame_buffer();
  if (!fb || !ppa_client_) {
    return false;
  }
  // the LVGL rotation the GUI configured; the BSP's flush uses the identical
  // LVGL->PPA mapping, so the game frames come out the same way up as the GUI
  auto rotation = lv_display_get_rotation(lv_display_get_default());
  ppa_srm_rotation_angle_t angle = PPA_SRM_ROTATION_ANGLE_0;
  bool swap = false;
  switch (rotation) {
  case LV_DISPLAY_ROTATION_90:
    angle = PPA_SRM_ROTATION_ANGLE_90;
    swap = true;
    break;
  case LV_DISPLAY_ROTATION_180:
    angle = PPA_SRM_ROTATION_ANGLE_180;
    break;
  case LV_DISPLAY_ROTATION_270:
    angle = PPA_SRM_ROTATION_ANGLE_270;
    swap = true;
    break;
  default:
    break;
  }
  // panel (native) picture size
  const uint32_t pic_w = Bsp::display_width();  // 720
  const uint32_t pic_h = Bsp::display_height(); // 1280
  // scaled block size in landscape (logical) space
  const uint32_t in_w = staging_width();
  const uint32_t in_h = staging_height();
  const float scale_x = static_cast<float>(display_width_) / static_cast<float>(in_w);
  const float scale_y = static_cast<float>(display_height_) / static_cast<float>(in_h);
  const uint32_t out_w = static_cast<uint32_t>(in_w * scale_x);
  const uint32_t out_h = static_cast<uint32_t>(in_h * scale_y);
  // centered; a centered box has the same offsets whichever way it is rotated
  const uint32_t off_x = swap ? (pic_w - out_h) / 2 : (pic_w - out_w) / 2;
  const uint32_t off_y = swap ? (pic_h - out_w) / 2 : (pic_h - out_h) / 2;

  ppa_srm_oper_config_t srm = {};
  srm.in.buffer = rgb_frame_;
  srm.in.pic_w = in_w;
  srm.in.pic_h = in_h;
  srm.in.block_w = in_w;
  srm.in.block_h = in_h;
  srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  srm.out.buffer = fb;
  srm.out.buffer_size = dpi_fb_bytes_;
  srm.out.pic_w = pic_w;
  srm.out.pic_h = pic_h;
  srm.out.block_offset_x = off_x;
  srm.out.block_offset_y = off_y;
  srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  srm.rotation_angle = angle;
  srm.scale_x = scale_x;
  srm.scale_y = scale_y;
  srm.mode = PPA_TRANS_MODE_BLOCKING;
  auto err = ppa_do_scale_rotate_mirror(ppa_client_, &srm);
  if (err != ESP_OK) {
    logger_.error_rate_limited("PPA scale/rotate failed: {}", esp_err_to_name(err));
    return false;
  }
  return true;
}

bool Tab5Emu::video_task_callback(std::mutex &m, std::condition_variable &cv, bool &task_notified) {
  VideoFrame vf{nullptr, nullptr};
  if (xQueueReceive(video_queue_, &vf, portMAX_DELAY) != pdTRUE) {
    return false;
  }
  if (vf.frame == nullptr) {
    clear_screen();
    xSemaphoreGive(frame_done_);
    return false;
  }
  if (!ensure_rgb_frame()) {
    xSemaphoreGive(frame_done_);
    return false;
  }
  const auto t0 = esp_timer_get_time();
  convert_frame(vf.frame, vf.overlay);
  blit_rgb_frame();
  update_frame_time(esp_timer_get_time() - t0);
  xSemaphoreGive(frame_done_);
  return false;
}

// Convert the native frame into the RGB565 staging buffer. With an overlay the
// staging buffer is the native frame upscaled by staging_scale() (nearest
// neighbour) with the overlay drawn 1:1 on top, centered, palette index 0
// transparent.
void Tab5Emu::convert_frame(const void *frame, const void *overlay) {
  const size_t k = staging_scale();
  const size_t sw = staging_width();
  const size_t sh = staging_height();
  if (!has_palette()) {
    // RGB565 native frames: no overlay support, plain copy
    const uint16_t *src = static_cast<const uint16_t *>(frame);
    if (native_pitch_ == native_width_) {
      memcpy(rgb_frame_, src, native_width_ * native_height_ * sizeof(uint16_t));
    } else {
      for (size_t y = 0; y < native_height_; y++) {
        memcpy(rgb_frame_ + y * native_width_, src + y * native_pitch_, native_width_ * sizeof(uint16_t));
      }
    }
    return;
  }
  const uint8_t *src = static_cast<const uint8_t *>(frame);
  const uint16_t *pal = palette_;
  if (k == 1) {
    for (size_t y = 0; y < native_height_; y++) {
      const uint8_t *row = src + y * native_pitch_;
      uint16_t *dst = rgb_frame_ + y * native_width_;
      size_t x = 0;
      for (; x + 4 <= native_width_; x += 4) {
        dst[x + 0] = pal[row[x + 0]];
        dst[x + 1] = pal[row[x + 1]];
        dst[x + 2] = pal[row[x + 2]];
        dst[x + 3] = pal[row[x + 3]];
      }
      for (; x < native_width_; x++) {
        dst[x] = pal[row[x]];
      }
    }
  } else if (k == 2) {
    // the common case (e.g. 426x240 world under a 640x480 HUD): write pixel
    // pairs as 32-bit words, then duplicate the row
    for (size_t y = 0; y < native_height_; y++) {
      const uint8_t *row = src + y * native_pitch_;
      uint32_t *dst = reinterpret_cast<uint32_t *>(rgb_frame_ + (2 * y) * sw);
      for (size_t x = 0; x < native_width_; x++) {
        const uint32_t p = pal[row[x]];
        dst[x] = p | (p << 16);
      }
      memcpy(dst + sw / 2, dst, sw * sizeof(uint16_t));
    }
  } else {
    for (size_t y = 0; y < native_height_; y++) {
      const uint8_t *row = src + y * native_pitch_;
      uint16_t *dst = rgb_frame_ + (k * y) * sw;
      for (size_t x = 0; x < native_width_; x++) {
        const uint16_t p = pal[row[x]];
        for (size_t i = 0; i < k; i++) {
          dst[k * x + i] = p;
        }
      }
      for (size_t i = 1; i < k; i++) {
        memcpy(dst + i * sw, dst, sw * sizeof(uint16_t));
      }
    }
  }
  if (!overlay || overlay_width_ == 0 || overlay_height_ == 0) {
    return;
  }
  // keyed overlay, 1:1, centered in the staging frame. Rows / 4-pixel groups
  // that are entirely transparent (index 0) are skipped cheaply, so a mostly
  // empty HUD layer costs little.
  const size_t ow = std::min(overlay_width_, sw);
  const size_t oh = std::min(overlay_height_, sh);
  const size_t ox = (sw - ow) / 2;
  const size_t oy = (sh - oh) / 2;
  const uint8_t *ov = static_cast<const uint8_t *>(overlay);
  for (size_t y = 0; y < oh; y++) {
    const uint8_t *row = ov + y * overlay_pitch_;
    uint16_t *dst = rgb_frame_ + (oy + y) * sw + ox;
    size_t x = 0;
    for (; x + 4 <= ow; x += 4) {
      uint32_t word;
      memcpy(&word, row + x, sizeof(word));
      if (word == 0) {
        continue;
      }
      if (row[x + 0]) dst[x + 0] = pal[row[x + 0]];
      if (row[x + 1]) dst[x + 1] = pal[row[x + 1]];
      if (row[x + 2]) dst[x + 2] = pal[row[x + 2]];
      if (row[x + 3]) dst[x + 3] = pal[row[x + 3]];
    }
    for (; x < ow; x++) {
      if (row[x]) dst[x] = pal[row[x]];
    }
  }
}

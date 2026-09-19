#include "tab5-emu.hpp"

#include <algorithm>
#include <cmath>
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
  GamepadState state;
  {
    std::lock_guard<std::mutex> lk(touch_gamepad_.mutex);
    state = touch_gamepad_.state;
  }
  // USB keyboard as a gamepad: arrows = d-pad, Z/X/A/S = A/B/X/Y (also
  // Ctrl/Alt), Enter = start, Shift / Backspace = select
  if (usb_keyboard_present()) {
    const auto kb = keyboard_state();
    auto key = [&](int usage) { return (kb.keys[usage >> 3] >> (usage & 7)) & 1; };
    state.up |= key(82);
    state.down |= key(81);
    state.left |= key(80);
    state.right |= key(79);
    state.a |= key(29) | key(0xE0);    // z, left ctrl
    state.b |= key(27) | key(0xE2);    // x, left alt
    state.x |= key(4);                 // a
    state.y |= key(22);                // s
    state.start |= key(40) | key(88);  // enter, keypad enter
    state.select |= key(0xE1) | key(42) | key(0xE5); // shift, backspace
  }
  return state;
}

uint8_t *Tab5Emu::frame_buffer0() {
  if (!frame_buffers_[0]) {
    frame_buffers_[0] = static_cast<uint8_t *>(
        heap_caps_aligned_calloc(64, 1, FRAME_BUFFER_PIXELS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  return frame_buffers_[0];
}

uint8_t *Tab5Emu::frame_buffer1() {
  if (!frame_buffers_[1]) {
    frame_buffers_[1] = static_cast<uint8_t *>(
        heap_caps_aligned_calloc(64, 1, FRAME_BUFFER_PIXELS * sizeof(uint16_t), MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  return frame_buffers_[1];
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
  ensure_tile_buffer();
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

void Tab5Emu::push_frame(const void *frame, const void *overlay, size_t overlay_row_begin,
                         size_t overlay_row_end) {
  if (!video_queue_) {
    return;
  }
  // overwrite: if the video task hasn't consumed the previous frame yet, the
  // game is running faster than the display and we drop the older one
  VideoFrame vf{frame, overlay, overlay_row_begin, overlay_row_end};
  xQueueOverwrite(video_queue_, &vf);
}

Tab5Emu::VideoStats Tab5Emu::video_stats(bool reset) {
  auto stats = video_stats_;
  if (reset) {
    video_stats_ = {};
  }
  return stats;
}

void Tab5Emu::wait_frame() {
  if (frame_done_) {
    xSemaphoreTake(frame_done_, pdMS_TO_TICKS(100));
  }
}

bool Tab5Emu::ensure_tile_buffer() {
  if (tile_buf_) {
    return true;
  }
  // 128-byte aligned so the PPA's cache sync covers exactly this buffer;
  // internal memory so the conversion writes and the PPA reads are fast
  tile_buf_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(128, TILE_BYTES, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT));
  if (!tile_buf_) {
    logger_.warn("No internal RAM for the {} byte video tile; using PSRAM", TILE_BYTES);
    tile_buf_ = static_cast<uint16_t *>(heap_caps_aligned_alloc(128, TILE_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
  }
  if (!tile_buf_) {
    logger_.error("Could not allocate {} bytes for the video tile", TILE_BYTES);
    return false;
  }
  tile_buf_bytes_ = TILE_BYTES;
  return true;
}

Tab5Emu::Layout Tab5Emu::layout() const {
  Layout lo{};
  // the LVGL rotation the GUI configured; the BSP's flush uses the identical
  // LVGL->PPA mapping, so the game frames come out the same way up as the GUI
  auto rotation = lv_display_get_rotation(lv_display_get_default());
  lo.angle = PPA_SRM_ROTATION_ANGLE_0;
  lo.swap = false;
  switch (rotation) {
  case LV_DISPLAY_ROTATION_90:
    lo.angle = PPA_SRM_ROTATION_ANGLE_90;
    lo.swap = true;
    break;
  case LV_DISPLAY_ROTATION_180:
    lo.angle = PPA_SRM_ROTATION_ANGLE_180;
    break;
  case LV_DISPLAY_ROTATION_270:
    lo.angle = PPA_SRM_ROTATION_ANGLE_270;
    lo.swap = true;
    break;
  default:
    break;
  }
  // panel (native) picture size
  const uint32_t pic_w = Bsp::display_width();  // 720
  const uint32_t pic_h = Bsp::display_height(); // 1280
  const uint32_t in_w = staging_width();
  const uint32_t in_h = staging_height();
  // the PPA scales in 1/16 steps; use the ratio it will actually apply so the
  // tiles line up exactly
  auto quantize = [](float s) { return std::floor(s * 16.0f) / 16.0f; };
  lo.scale_x = quantize(static_cast<float>(display_width_) / static_cast<float>(in_w));
  lo.scale_y = quantize(static_cast<float>(display_height_) / static_cast<float>(in_h));
  lo.out_w = static_cast<uint32_t>(in_w * lo.scale_x);
  lo.out_h = static_cast<uint32_t>(in_h * lo.scale_y);
  // centered; a centered box has the same offsets whichever way it is rotated
  lo.off_x = lo.swap ? (pic_w - lo.out_h) / 2 : (pic_w - lo.out_w) / 2;
  lo.off_y = lo.swap ? (pic_h - lo.out_w) / 2 : (pic_h - lo.out_h) / 2;
  return lo;
}

bool Tab5Emu::blit_tile(const Layout &lo, size_t x0, size_t y0, size_t tw, size_t th) {
  auto fb = dpi_frame_buffer();
  if (!fb || !ppa_client_) {
    return false;
  }
  // the tile's scaled rect in landscape (logical) space; tile origins are
  // multiples of 16 so these are exact for any 1/16-step scale
  const uint32_t X0 = static_cast<uint32_t>(std::lround(x0 * lo.scale_x));
  const uint32_t Y0 = static_cast<uint32_t>(std::lround(y0 * lo.scale_y));
  const uint32_t X1 = static_cast<uint32_t>(std::lround((x0 + tw) * lo.scale_x));
  const uint32_t Y1 = static_cast<uint32_t>(std::lround((y0 + th) * lo.scale_y));
  // where that rect lands on the panel (same mapping as lv_display_rotate_area)
  uint32_t bx = 0, by = 0;
  switch (lo.angle) {
  case PPA_SRM_ROTATION_ANGLE_90:
    bx = Y0;
    by = lo.out_w - X1;
    break;
  case PPA_SRM_ROTATION_ANGLE_180:
    bx = lo.out_w - X1;
    by = lo.out_h - Y1;
    break;
  case PPA_SRM_ROTATION_ANGLE_270:
    bx = lo.out_h - Y1;
    by = X0;
    break;
  default:
    bx = X0;
    by = Y0;
    break;
  }
  ppa_srm_oper_config_t srm = {};
  srm.in.buffer = tile_buf_;
  srm.in.pic_w = tw;
  srm.in.pic_h = th;
  srm.in.block_w = tw;
  srm.in.block_h = th;
  srm.in.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  srm.out.buffer = fb;
  srm.out.buffer_size = dpi_fb_bytes_;
  srm.out.pic_w = Bsp::display_width();
  srm.out.pic_h = Bsp::display_height();
  srm.out.block_offset_x = lo.off_x + bx;
  srm.out.block_offset_y = lo.off_y + by;
  srm.out.srm_cm = PPA_SRM_COLOR_MODE_RGB565;
  srm.rotation_angle = lo.angle;
  srm.scale_x = lo.scale_x;
  srm.scale_y = lo.scale_y;
  srm.mode = PPA_TRANS_MODE_BLOCKING;
  auto err = ppa_do_scale_rotate_mirror(ppa_client_, &srm);
  if (err != ESP_OK) {
    logger_.error_rate_limited("PPA scale/rotate failed: {} (tile {},{} {}x{} -> {},{})", esp_err_to_name(err), x0,
                               y0, tw, th, lo.off_x + bx, lo.off_y + by);
    return false;
  }
  return true;
}

bool Tab5Emu::video_task_callback(std::mutex &m, std::condition_variable &cv, bool &task_notified) {
  VideoFrame vf{nullptr, nullptr, 0, 0};
  if (xQueueReceive(video_queue_, &vf, portMAX_DELAY) != pdTRUE) {
    return false;
  }
  if (vf.frame == nullptr) {
    clear_screen();
    xSemaphoreGive(frame_done_);
    return false;
  }
  if (!ensure_tile_buffer() || native_width_ == 0 || native_height_ == 0) {
    xSemaphoreGive(frame_done_);
    return false;
  }
  const auto t0 = esp_timer_get_time();
  const auto lo = layout();
  const size_t sw = staging_width();
  const size_t sh = staging_height();
  const size_t tile_pixels = tile_buf_bytes_ / sizeof(uint16_t);
  // Tile along the axis that maps to panel rows: the PPA driver cache-syncs
  // every panel row a block touches, so a tile should cover few panel rows.
  // Tile origins are multiples of 16 pixels so they scale to exact positions.
  size_t tw = sw, th = sh;
  if (sw * sh > tile_pixels) {
    if (lo.swap) {
      tw = std::max<size_t>(16, (tile_pixels / sh) & ~size_t(15));
    } else {
      th = std::max<size_t>(16, (tile_pixels / sw) & ~size_t(15));
    }
  }
  uint64_t convert_us = 0, blit_us = 0;
  uint32_t tiles = 0;
  for (size_t y0 = 0; y0 < sh; y0 += th) {
    const size_t h = std::min(th, sh - y0);
    for (size_t x0 = 0; x0 < sw; x0 += tw) {
      const size_t w = std::min(tw, sw - x0);
      const auto t1 = esp_timer_get_time();
      convert_tile(vf.frame, vf.overlay, vf.overlay_row_begin, vf.overlay_row_end, x0, y0, w, h);
      const auto t2 = esp_timer_get_time();
      blit_tile(lo, x0, y0, w, h);
      const auto t3 = esp_timer_get_time();
      convert_us += t2 - t1;
      blit_us += t3 - t2;
      tiles++;
    }
  }
  video_stats_.convert_us += convert_us;
  video_stats_.blit_us += blit_us;
  video_stats_.frames++;
  video_stats_.tiles += tiles;
  update_frame_time(esp_timer_get_time() - t0);
  xSemaphoreGive(frame_done_);
  return false;
}

// Convert the staging rect [x0,x0+tw)x[y0,y0+th) into the tile buffer (pitch
// tw). The staging frame is the native frame upscaled by staging_scale()
// (nearest neighbour) with the overlay drawn 1:1 on top, centered, palette
// index 0 transparent.
void Tab5Emu::convert_tile(const void *frame, const void *overlay, size_t ov_r0, size_t ov_r1, size_t x0, size_t y0,
                           size_t tw, size_t th) {
  const size_t k = staging_scale();
  const size_t sw = staging_width();
  const size_t sh = staging_height();
  uint16_t *tile = tile_buf_;
  if (!has_palette()) {
    // RGB565 native frames: no overlay support (k == 1), plain copy
    const uint16_t *src = static_cast<const uint16_t *>(frame);
    for (size_t r = 0; r < th; r++) {
      memcpy(tile + r * tw, src + (y0 + r) * native_pitch_ + x0, tw * sizeof(uint16_t));
    }
    return;
  }
  const uint8_t *src = static_cast<const uint8_t *>(frame);
  const uint16_t *pal = palette_;
  for (size_t r = 0; r < th; r++) {
    const size_t Y = y0 + r;
    uint16_t *dst = tile + r * tw;
    if (k > 1 && (Y % k) != 0 && r > 0) {
      // same native row as the previous staging row
      memcpy(dst, dst - tw, tw * sizeof(uint16_t));
      continue;
    }
    const uint8_t *row = src + (Y / k) * native_pitch_;
    if (k == 1) {
      const uint8_t *s = row + x0;
      size_t x = 0;
      for (; x + 4 <= tw; x += 4) {
        dst[x + 0] = pal[s[x + 0]];
        dst[x + 1] = pal[s[x + 1]];
        dst[x + 2] = pal[s[x + 2]];
        dst[x + 3] = pal[s[x + 3]];
      }
      for (; x < tw; x++) {
        dst[x] = pal[s[x]];
      }
    } else if (k == 2 && (x0 % 2) == 0 && (tw % 2) == 0) {
      // write pixel pairs as 32-bit words
      const uint8_t *s = row + x0 / 2;
      uint32_t *d32 = reinterpret_cast<uint32_t *>(dst);
      const size_t n = tw / 2;
      for (size_t i = 0; i < n; i++) {
        const uint32_t p = pal[s[i]];
        d32[i] = p | (p << 16);
      }
    } else {
      for (size_t x = 0; x < tw; x++) {
        dst[x] = pal[row[(x0 + x) / k]];
      }
    }
  }
  if (!overlay || overlay_width_ == 0 || overlay_height_ == 0) {
    return;
  }
  // keyed overlay, 1:1, centered in the staging frame; only the overlay rows
  // in [ov_r0, ov_r1) can hold visible pixels. 4-pixel groups that are all
  // transparent are skipped cheaply.
  const size_t ow = std::min(overlay_width_, sw);
  const size_t oh = std::min(overlay_height_, sh);
  const size_t ox = (sw - ow) / 2;
  const size_t oy = (sh - oh) / 2;
  // overlay row / column ranges (in overlay pixels) intersecting this tile
  const size_t sy0 = std::max(oy, y0), sy1 = std::min(oy + oh, y0 + th);
  const size_t sx0 = std::max(ox, x0), sx1 = std::min(ox + ow, x0 + tw);
  if (sy1 <= sy0 || sx1 <= sx0) {
    return;
  }
  const size_t r0 = std::max(ov_r0, sy0 - oy);
  const size_t r1 = std::min(ov_r1, sy1 - oy);
  const size_t c0 = sx0 - ox;
  const size_t c1 = sx1 - ox;
  if (r1 <= r0) {
    return;
  }
  const uint8_t *ov = static_cast<const uint8_t *>(overlay);
  for (size_t r = r0; r < r1; r++) {
    const uint8_t *row = ov + r * overlay_pitch_;
    uint16_t *dst = tile + (oy + r - y0) * tw + (ox - x0);
    size_t c = c0;
    for (; c + 4 <= c1; c += 4) {
      uint32_t word;
      memcpy(&word, row + c, sizeof(word));
      if (word == 0) {
        continue;
      }
      if (row[c + 0]) dst[c + 0] = pal[row[c + 0]];
      if (row[c + 1]) dst[c + 1] = pal[row[c + 1]];
      if (row[c + 2]) dst[c + 2] = pal[row[c + 2]];
      if (row[c + 3]) dst[c + 3] = pal[row[c + 3]];
    }
    for (; c < c1; c++) {
      if (row[c]) dst[c] = pal[row[c]];
    }
  }
}

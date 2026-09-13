#include "jk.hpp"
#include "jk_esp.h"

#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <thread>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_timer.h>

#include "logger.hpp"
#include "tab5-emu.hpp"

using namespace std::chrono_literals;

static espp::Logger logger({.tag = "jk", .level = espp::Logger::Verbosity::INFO});

// ---------------------------------------------------------------------------
// state shared with the C platform layer
// ---------------------------------------------------------------------------
volatile int jk_esp_quit_requested = 0;

namespace {
jk::Config g_config;
bool g_paused = false;
int g_native_w = CONFIG_JK_INTERNAL_WIDTH;
int g_native_h = CONFIG_JK_INTERNAL_HEIGHT;
// the frame most recently handed to the display, converted to RGB565 (for
// screenshots / pause image). Allocated on first present.
std::vector<uint8_t> g_last_rgb565;
uint16_t g_palette565[256];
int g_last_w = 0, g_last_h = 0;
// 8-bit frame the presented palette applies to; kept so the HAL video task
// (which converts asynchronously) reads a stable buffer
std::vector<uint8_t> g_frame8[2];
int g_frame8_index = 0;
} // namespace

// ---------------------------------------------------------------------------
// C interface used by the platform layer
// ---------------------------------------------------------------------------
extern "C" {

void jk_esp_present_8bpp(const uint8_t *pixels, int width, int height, int pitch, const uint8_t *pal24) {
  auto &emu = Tab5Emu::get();
  if (width != g_last_w || height != g_last_h) {
    emu.native_size(width, height, width);
    g_last_w = width;
    g_last_h = height;
    g_last_rgb565.assign((size_t)width * height * 2, 0);
    g_frame8[0].assign((size_t)width * height, 0);
    g_frame8[1].assign((size_t)width * height, 0);
  }
  for (int i = 0; i < 256; i++) {
    uint8_t r = pal24[i * 3 + 0], g = pal24[i * 3 + 1], b = pal24[i * 3 + 2];
    g_palette565[i] = (uint16_t)(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
  }
  // the previous frame is done (wait_frame), so its buffer can be reused
  emu.wait_frame();
  auto &dst = g_frame8[g_frame8_index];
  g_frame8_index ^= 1;
  if (pitch == width) {
    memcpy(dst.data(), pixels, (size_t)width * height);
  } else {
    for (int y = 0; y < height; y++) {
      memcpy(dst.data() + (size_t)y * width, pixels + (size_t)y * pitch, width);
    }
  }
  // keep an RGB565 copy for screenshots
  uint16_t *shot = reinterpret_cast<uint16_t *>(g_last_rgb565.data());
  for (size_t i = 0; i < (size_t)width * height; i++) {
    shot[i] = g_palette565[dst[i]];
  }
  emu.palette(g_palette565, 256);
  emu.push_frame(dst.data());
}

void jk_esp_present_rgb565(const uint16_t *pixels, int width, int height, int pitch) {
  auto &emu = Tab5Emu::get();
  if (width != g_last_w || height != g_last_h) {
    emu.native_size(width, height, pitch);
    g_last_w = width;
    g_last_h = height;
    g_last_rgb565.assign((size_t)width * height * 2, 0);
  }
  emu.wait_frame();
  for (int y = 0; y < height; y++) {
    memcpy(g_last_rgb565.data() + (size_t)y * width * 2, pixels + (size_t)y * pitch, (size_t)width * 2);
  }
  emu.palette(nullptr, 0);
  emu.push_frame(g_last_rgb565.data());
}

void jk_esp_read_input(jk_esp_input_t *out) {
  auto &emu = Tab5Emu::get();
  memset(out, 0, sizeof(*out));
  out->buttons = emu.gamepad_state().buttons;
  auto touch = emu.touchpad_data();
  out->touch_down = touch.btn_state && touch.num_touch_points > 0;
  out->touch_x = out->touch_down ? touch.x : -1;
  out->touch_y = out->touch_down ? touch.y : -1;
}

size_t jk_esp_audio_write(const int16_t *stereo_pcm, size_t num_frames) {
  auto &emu = Tab5Emu::get();
  size_t bytes = num_frames * 2 * sizeof(int16_t);
  emu.play_audio(reinterpret_cast<const uint8_t *>(stereo_pcm), bytes);
  return bytes;
}

void jk_esp_audio_set_rate(uint32_t sample_rate) { Tab5Emu::get().audio_sample_rate(sample_rate); }

uint32_t jk_esp_time_ms(void) { return (uint32_t)(esp_timer_get_time() / 1000); }
uint64_t jk_esp_time_us(void) { return (uint64_t)esp_timer_get_time(); }
void jk_esp_sleep_ms(uint32_t ms) { std::this_thread::sleep_for(std::chrono::milliseconds(ms)); }

void *jk_esp_malloc(size_t size) {
  void *p = heap_caps_malloc(size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) {
    p = heap_caps_malloc(size, MALLOC_CAP_8BIT);
  }
  return p;
}
void *jk_esp_realloc(void *ptr, size_t size) {
  void *p = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) {
    p = heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT);
  }
  return p;
}
void jk_esp_free(void *ptr) { heap_caps_free(ptr); }

void jk_esp_log(const char *fmt, ...) {
  char buf[256];
  va_list args;
  va_start(args, fmt);
  vsnprintf(buf, sizeof(buf), fmt, args);
  va_end(args);
  // strip the trailing newline the engine usually adds
  size_t n = strlen(buf);
  while (n && (buf[n - 1] == '\n' || buf[n - 1] == '\r')) {
    buf[--n] = 0;
  }
  logger.info("{}", buf);
}

int jk_esp_internal_width(void) { return g_native_w; }
int jk_esp_internal_height(void) { return g_native_h; }
const char *jk_esp_game_dir(void) { return g_config.game_dir.c_str(); }

} // extern "C"

// ---------------------------------------------------------------------------
// engine entry points (provided by the platform layer when the engine is
// built)
// ---------------------------------------------------------------------------
#if CONFIG_JK_ENGINE
extern "C" {
int jk_esp_engine_startup(const char *game_dir);
int jk_esp_engine_frame(void);
void jk_esp_engine_shutdown(void);
}
#endif

// ---------------------------------------------------------------------------
// test pattern (CONFIG_JK_ENGINE=n)
// ---------------------------------------------------------------------------
namespace {
std::vector<uint8_t> g_pattern;
uint8_t g_pattern_pal[256 * 3];
uint32_t g_pattern_frame = 0;

void pattern_init() {
  g_pattern.assign((size_t)g_native_w * g_native_h, 0);
  // palette: a 216-color cube + greys, so the pattern exercises the LUT path
  for (int i = 0; i < 216; i++) {
    g_pattern_pal[i * 3 + 0] = (i / 36) * 51;
    g_pattern_pal[i * 3 + 1] = ((i / 6) % 6) * 51;
    g_pattern_pal[i * 3 + 2] = (i % 6) * 51;
  }
  for (int i = 216; i < 256; i++) {
    uint8_t g = (uint8_t)((i - 216) * 255 / 39);
    g_pattern_pal[i * 3 + 0] = g;
    g_pattern_pal[i * 3 + 1] = g;
    g_pattern_pal[i * 3 + 2] = g;
  }
}

void pattern_frame() {
  const int w = g_native_w, h = g_native_h;
  const uint32_t t = g_pattern_frame++;
  auto input = jk_esp_input_t{};
  jk_esp_read_input(&input);
  for (int y = 0; y < h; y++) {
    uint8_t *row = g_pattern.data() + (size_t)y * w;
    for (int x = 0; x < w; x++) {
      // scrolling color bands + a moving diagonal
      int band = ((x + t) / 16) % 6;
      int r = band, g = (y / 40) % 6, b = ((x + y + t) / 24) % 6;
      row[x] = (uint8_t)(r * 36 + g * 6 + b);
    }
  }
  // corner markers (grey ramp) so orientation / centering are obvious
  for (int y = 0; y < 16; y++) {
    for (int x = 0; x < 16; x++) {
      g_pattern[(size_t)y * w + x] = 255;                       // top-left: white
      g_pattern[(size_t)y * w + (w - 1 - x)] = 236;             // top-right: grey
      g_pattern[(size_t)(h - 1 - y) * w + x] = 216;             // bottom-left: black
    }
  }
  // show the pressed gamepad buttons as filled boxes along the bottom
  for (int b = 0; b < 10; b++) {
    bool pressed = input.buttons & (1 << b);
    for (int y = h - 12; y < h - 4; y++) {
      for (int x = 4 + b * 12; x < 12 + b * 12; x++) {
        g_pattern[(size_t)y * w + x] = pressed ? 255 : 226;
      }
    }
  }
  jk_esp_present_8bpp(g_pattern.data(), w, h, w, g_pattern_pal);
}
} // namespace

// ---------------------------------------------------------------------------
// public API
// ---------------------------------------------------------------------------
namespace jk {

bool init(const Config &config) {
  g_config = config;
  g_paused = false;
  jk_esp_quit_requested = 0;
  g_last_w = g_last_h = 0;
  g_native_w = CONFIG_JK_INTERNAL_WIDTH;
  g_native_h = CONFIG_JK_INTERNAL_HEIGHT;
  logger.info("init: game_dir='{}' internal {}x{}", config.game_dir, g_native_w, g_native_h);
#if CONFIG_JK_ENGINE
  return jk_esp_engine_startup(config.game_dir.c_str()) != 0;
#else
  pattern_init();
  return true;
#endif
}

void deinit() {
#if CONFIG_JK_ENGINE
  jk_esp_engine_shutdown();
#endif
  Tab5Emu::get().wait_frame();
  g_pattern.clear();
  g_pattern.shrink_to_fit();
}

bool run_frame() {
  if (g_paused) {
    std::this_thread::sleep_for(10ms);
    return true;
  }
#if CONFIG_JK_ENGINE
  if (jk_esp_quit_requested) {
    return false;
  }
  return jk_esp_engine_frame() != 0;
#else
  auto t0 = esp_timer_get_time();
  pattern_frame();
  // pace the test pattern at ~60 fps
  auto elapsed = esp_timer_get_time() - t0;
  if (elapsed < 16000) {
    std::this_thread::sleep_for(std::chrono::microseconds(16000 - elapsed));
  }
  return !jk_esp_quit_requested;
#endif
}

void pause() { g_paused = true; }
void resume() { g_paused = false; }

void reset() {
  logger.info("reset");
#if CONFIG_JK_ENGINE
  jk_esp_engine_shutdown();
  jk_esp_engine_startup(g_config.game_dir.c_str());
#else
  g_pattern_frame = 0;
#endif
}

void save(const std::string &path, int slot) {
  // TODO(phase 5): route to the engine's jkSaveLoad with a slot-derived name
  logger.info("save slot {} -> {}", slot, path);
}

void load(const std::string &path, int slot) {
  // TODO(phase 5): route to the engine's jkSaveLoad
  logger.info("load slot {} <- {}", slot, path);
}

std::pair<size_t, size_t> video_size() { return {(size_t)g_native_w, (size_t)g_native_h}; }

std::span<uint8_t> video_buffer_rgb565() { return {g_last_rgb565.data(), g_last_rgb565.size()}; }

} // namespace jk

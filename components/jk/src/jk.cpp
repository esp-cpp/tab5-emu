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
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

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
std::atomic<bool> g_paused{false};
// The engine runs on its own task: it keeps large buffers on the stack, far
// more than the main task has, so it gets a big stack in PSRAM.
TaskHandle_t g_engine_task = nullptr;
std::atomic<bool> g_engine_running{false};
std::atomic<bool> g_engine_stop{false};
std::atomic<bool> g_engine_started_ok{false};
constexpr size_t ENGINE_STACK_BYTES = 1024 * 1024;
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
const uint8_t *g_last_frame8 = nullptr;
} // namespace

// ---------------------------------------------------------------------------
// C interface used by the platform layer
// ---------------------------------------------------------------------------
extern "C" {

uint64_t jk_esp_us_input = 0, jk_esp_us_wait = 0, jk_esp_us_free = 0, jk_esp_us_realloc = 0, jk_esp_us_log = 0;
uint32_t jk_esp_n_input = 0, jk_esp_n_wait = 0, jk_esp_n_free = 0, jk_esp_n_realloc = 0, jk_esp_n_log = 0;

static uint32_t g_presents = 0;

void jk_esp_present_8bpp(const uint8_t *pixels, int width, int height, int pitch, const uint8_t *pal24) {
  auto &emu = Tab5Emu::get();
  g_presents++;
  if (g_presents <= 10 || (g_presents % 100) == 0) {
    logger.info("present #{} {}x{} pitch {}", g_presents, width, height, pitch);
  }
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
  {
    auto t0 = esp_timer_get_time();
    emu.wait_frame();
    jk_esp_us_wait += esp_timer_get_time() - t0; jk_esp_n_wait++;
  }
  auto &dst = g_frame8[g_frame8_index];
  g_frame8_index ^= 1;
  if (pitch == width) {
    memcpy(dst.data(), pixels, (size_t)width * height);
  } else {
    for (int y = 0; y < height; y++) {
      memcpy(dst.data() + (size_t)y * width, pixels + (size_t)y * pitch, width);
    }
  }
  g_last_frame8 = dst.data();
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
  auto t0 = esp_timer_get_time();
  struct Acc { int64_t t0; ~Acc() { jk_esp_us_input += esp_timer_get_time() - t0; jk_esp_n_input++; } } acc{t0};
  auto &emu = Tab5Emu::get();
  memset(out, 0, sizeof(*out));
  out->buttons = emu.gamepad_state().buttons;
  auto touch = emu.touchpad_data();
  out->touch_down = touch.num_touch_points > 0;
  out->touch_x = out->touch_down ? touch.x : -1;
  out->touch_y = out->touch_down ? touch.y : -1;
  static bool last_down = false;
  if (out->touch_down != last_down) {
    logger.info("touch {} at {},{}", out->touch_down ? "down" : "up", touch.x, touch.y);
    last_down = out->touch_down;
  }
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
  auto t0 = esp_timer_get_time();
  struct Acc { int64_t t0; ~Acc() { jk_esp_us_realloc += esp_timer_get_time() - t0; jk_esp_n_realloc++; } } acc{t0};
  void *p = heap_caps_realloc(ptr, size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (!p) {
    p = heap_caps_realloc(ptr, size, MALLOC_CAP_8BIT);
  }
  return p;
}
void jk_esp_free(void *ptr) {
  auto t0 = esp_timer_get_time();
  heap_caps_free(ptr);
  jk_esp_us_free += esp_timer_get_time() - t0; jk_esp_n_free++;
}

void jk_esp_log(const char *fmt, ...) {
  auto t0 = esp_timer_get_time();
  struct Acc { int64_t t0; ~Acc() { jk_esp_us_log += esp_timer_get_time() - t0; jk_esp_n_log++; } } acc{t0};
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

void jk_esp_print_heap(void) {
  logger.info("heap: internal free {} (largest {}), psram free {} (largest {})",
              heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
}

int jk_esp_internal_width(void) { return g_native_w; }
int jk_esp_internal_height(void) { return g_native_h; }
const char *jk_esp_game_dir(void) { return g_config.game_dir.c_str(); }
const char *jk_esp_engine_args(void) { return CONFIG_JK_ENGINE_ARGS; }

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
#if defined(JK_ESP_FS_DEBUG)
  // bring-up benchmark: SD read throughput and float parsing speed
  {
    auto path = config.game_dir + "/resource/Res2.gob";
    FILE *f = fopen(path.c_str(), "rb");
    if (f) {
      std::vector<uint8_t> buf(16 * 1024);
      size_t total = 0;
      auto t0 = esp_timer_get_time();
      while (total < 8 * 1024 * 1024) {
        size_t n = fread(buf.data(), 1, buf.size(), f);
        if (!n) break;
        total += n;
      }
      auto dt = esp_timer_get_time() - t0;
      fclose(f);
      logger.info("bench: read {} KB in {} ms = {:.1f} MB/s", total / 1024, dt / 1000, total / 1.048576 / (dt ? dt : 1));
      // line reads
      f = fopen(path.c_str(), "rb");
      char line[256];
      size_t lines = 0;
      t0 = esp_timer_get_time();
      while (lines < 50000 && fgets(line, sizeof(line), f)) lines++;
      dt = esp_timer_get_time() - t0;
      fclose(f);
      logger.info("bench: {} fgets in {} ms", lines, dt / 1000);
    }
    auto t0 = esp_timer_get_time();
    float acc = 0;
    for (int i = 0; i < 20000; i++) {
      float a, b, c;
      sscanf("0.123456 -1.234567 12.345678", "%f %f %f", &a, &b, &c);
      acc += a + b + c;
    }
    auto dt = esp_timer_get_time() - t0;
    logger.info("bench: 20000 sscanf(3 floats) in {} ms (acc {})", dt / 1000, acc);
  }
#endif
  g_engine_stop = false;
  g_engine_started_ok = false;
  g_engine_running = true;
  auto ok = xTaskCreatePinnedToCoreWithCaps(
      [](void *) {
        if (jk_esp_engine_startup(g_config.game_dir.c_str())) {
          g_engine_started_ok = true;
          while (!g_engine_stop && !jk_esp_quit_requested) {
            if (g_paused) {
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }
            if (!jk_esp_engine_frame()) {
              logger.warn("engine frame returned 0");
              break;
            }
          }
          logger.info("engine loop done (stop={} quit={})", g_engine_stop.load(), jk_esp_quit_requested);
          jk_esp_engine_shutdown();
        } else {
          logger.error("engine startup failed");
        }
        g_engine_running = false;
        vTaskDelete(nullptr);
      },
      "jk_engine", ENGINE_STACK_BYTES, nullptr, 10, &g_engine_task, 0, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  if (ok != pdPASS) {
    logger.error("could not create the engine task");
    g_engine_running = false;
    return false;
  }
  return true;
#else
  pattern_init();
  return true;
#endif
}

void deinit() {
#if CONFIG_JK_ENGINE
  g_engine_stop = true;
  jk_esp_quit_requested = 1;
  while (g_engine_running) {
    std::this_thread::sleep_for(10ms);
  }
  g_engine_task = nullptr;
#endif
  Tab5Emu::get().wait_frame();
  g_pattern.clear();
  g_pattern.shrink_to_fit();
}

bool run_frame() {
#if CONFIG_JK_ENGINE
  // the engine task does the work; this just paces the cart loop
  std::this_thread::sleep_for(20ms);
  return g_engine_running;
#else
  if (g_paused) {
    std::this_thread::sleep_for(10ms);
    return true;
  }
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
  // TODO: restart the engine task
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

std::span<uint8_t> video_buffer_rgb565() {
  // convert the last presented 8-bit frame on demand (screenshots only)
  if (g_last_frame8 && g_last_rgb565.size() == (size_t)g_last_w * g_last_h * 2) {
    uint16_t *shot = reinterpret_cast<uint16_t *>(g_last_rgb565.data());
    for (size_t i = 0; i < (size_t)g_last_w * g_last_h; i++) {
      shot[i] = g_palette565[g_last_frame8[i]];
    }
  }
  return {g_last_rgb565.data(), g_last_rgb565.size()};
}

} // namespace jk

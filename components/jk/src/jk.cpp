#include "jk.hpp"
#include "statistics.hpp"
#include "jk_esp.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <mutex>
#include <thread>
#include <vector>

#include <esp_heap_caps.h>

#include "task.hpp"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hpp"
#include "tab5-emu.hpp"

using namespace std::chrono_literals;

// engine-thread entry points (src/Platform/ESP32/jk_esp_save.c)
extern "C" {
int jk_esp_engine_save(const char *path, const char *label);
int jk_esp_engine_load(const char *path);
int jk_esp_engine_reset(void);
}

static espp::Logger logger({.tag = "jk", .level = espp::Logger::Verbosity::INFO});

// ---------------------------------------------------------------------------
// state shared with the C platform layer
// ---------------------------------------------------------------------------
volatile int jk_esp_quit_requested = 0;

namespace {
jk::Config g_config;
std::atomic<bool> g_paused{false};
// true while the engine loop sits in its paused branch (no frame in flight),
// so the cart may read the frame buffers (screenshots) and post requests
std::atomic<bool> g_parked{false};
// The engine runs on its own task: it keeps large buffers on the stack, far
// more than the main task has, so it gets a big stack in PSRAM.
TaskHandle_t g_engine_task = nullptr;
std::atomic<bool> g_engine_running{false};
std::atomic<bool> g_engine_stop{false};
std::atomic<bool> g_engine_started_ok{false};
// software mixer: runs on its own task so audio keeps flowing through engine
// frame-time jitter (level loads, long frames)
std::mutex g_audio_mutex;
// pause-menu requests (save / load / reset) run on the engine thread between
// frames; the caller blocks until the engine has handled them
enum class Request { NONE, SAVE, LOAD, RESET };
std::atomic<Request> g_request{Request::NONE};
std::string g_request_path;
std::string g_request_label;
std::atomic<int> g_request_result{0};

void run_pending_request() {
  const auto req = g_request.load();
  if (req == Request::NONE) {
    return;
  }
  int result = 0;
  switch (req) {
  case Request::SAVE:
    result = jk_esp_engine_save(g_request_path.c_str(), g_request_label.c_str());
    break;
  case Request::LOAD:
    result = jk_esp_engine_load(g_request_path.c_str());
    break;
  case Request::RESET:
    result = jk_esp_engine_reset();
    break;
  default:
    break;
  }
  g_request_result = result;
  g_request = Request::NONE;
}

// post a request and wait for the engine thread to handle it
bool request(Request req, const std::string &path = {}, const std::string &label = {}) {
  if (!g_engine_running || !g_engine_started_ok) {
    return false;
  }
  g_request_path = path;
  g_request_label = label;
  g_request_result = 0;
  g_request = req;
  for (int i = 0; i < 1000 && g_request != Request::NONE; i++) {
    std::this_thread::sleep_for(10ms);
  }
  if (g_request != Request::NONE) {
    logger.error("engine did not handle the request");
    g_request = Request::NONE;
    return false;
  }
  return g_request_result != 0;
}
std::unique_ptr<espp::Task> g_audio_task;
extern "C" void stdSound_ESP32_Pump(void);
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
// same for the (optional) HUD overlay presented on top of the world frame
std::vector<uint8_t> g_overlay8[2];
int g_overlay_w = 0, g_overlay_h = 0;
} // namespace

// ---------------------------------------------------------------------------
// C interface used by the platform layer
// ---------------------------------------------------------------------------
extern "C" {

uint64_t jk_esp_us_copy = 0;
uint64_t jk_esp_us_input = 0, jk_esp_us_wait = 0, jk_esp_us_free = 0, jk_esp_us_realloc = 0, jk_esp_us_log = 0;
uint32_t jk_esp_n_input = 0, jk_esp_n_wait = 0, jk_esp_n_free = 0, jk_esp_n_realloc = 0, jk_esp_n_log = 0;

static uint32_t g_presents = 0;

static void apply_video_setting_for(int w, int h) {
  auto &emu = Tab5Emu::get();
  const float sw = static_cast<float>(Tab5Emu::lcd_width()), sh = static_cast<float>(Tab5Emu::lcd_height());
  switch (emu.video_setting()) {
  case VideoSetting::ORIGINAL: {
    // largest integer scale that fits
    int s = std::max(1, (int)std::min(sw / w, sh / h));
    emu.display_size(w * s, h * s);
    break;
  }
  case VideoSetting::FILL:
    emu.display_size(Tab5Emu::lcd_width(), Tab5Emu::lcd_height());
    break;
  case VideoSetting::FIT:
  default: {
    // the PPA scales in 1/16 steps; round down so the picture stays on screen
    float s = std::min(sw / w, sh / h);
    s = std::floor(s * 16.0f) / 16.0f;
    emu.display_size(static_cast<size_t>(w * s), static_cast<size_t>(h * s));
    break;
  }
  }
}

void jk_esp_present_8bpp(const uint8_t *pixels, int width, int height, int pitch, const uint8_t *pal24) {
  jk_esp_present_8bpp_overlay(pixels, width, height, pitch, nullptr, 0, 0, 0, pal24);
}

void jk_esp_present_8bpp_overlay(const uint8_t *pixels, int width, int height, int pitch,
                                 const uint8_t *overlay, int overlay_width, int overlay_height,
                                 int overlay_pitch, const uint8_t *pal24) {
  auto &emu = Tab5Emu::get();
  if (g_paused) {
    // the tab5-emu pause menu owns the screen; the engine only gets here
    // while paused when a level load (one long "frame") is presenting its
    // loading screen, which must not draw over the menu
    return;
  }
  if (!overlay || overlay_width <= 0 || overlay_height <= 0) {
    overlay = nullptr;
    overlay_width = overlay_height = 0;
  }
  g_presents++;
  if (g_presents <= 10 || (g_presents % 100) == 0) {
    logger.info("present #{} {}x{} pitch {}", g_presents, width, height, pitch);
  }
  if (width != g_last_w || height != g_last_h || overlay_width != g_overlay_w || overlay_height != g_overlay_h) {
    emu.wait_frame();
    emu.native_size(width, height, width);
    emu.overlay_size(overlay_width, overlay_height, overlay_width);
    apply_video_setting_for(width, height);
    g_last_w = width;
    g_last_h = height;
    g_overlay_w = overlay_width;
    g_overlay_h = overlay_height;
    g_last_rgb565.assign((size_t)width * height * 2, 0);
    g_frame8[0].assign((size_t)width * height, 0);
    g_frame8[1].assign((size_t)width * height, 0);
    g_overlay8[0].assign((size_t)overlay_width * overlay_height, 0);
    g_overlay8[1].assign((size_t)overlay_width * overlay_height, 0);
    logger.info("present {}x{} overlay {}x{}", width, height, overlay_width, overlay_height);
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
  const auto t_copy = esp_timer_get_time();
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
  const uint8_t *ov = nullptr;
  size_t ov_r0 = 0, ov_r1 = 0;
  if (overlay) {
    // the overlay index follows the frame index (both were flipped above).
    // Only rows with any visible (non-zero) pixel are copied; the HUD leaves
    // most of the 640x480 layer transparent.
    auto &odst = g_overlay8[g_frame8_index ^ 1];
    ov_r0 = SIZE_MAX;
    for (int y = 0; y < overlay_height; y++) {
      const uint8_t *row = overlay + (size_t)y * overlay_pitch;
      bool visible = false;
      int x = 0;
      for (; x + 4 <= overlay_width; x += 4) {
        uint32_t word;
        memcpy(&word, row + x, sizeof(word));
        if (word) {
          visible = true;
          break;
        }
      }
      for (; !visible && x < overlay_width; x++) {
        visible = row[x] != 0;
      }
      if (visible) {
        memcpy(odst.data() + (size_t)y * overlay_width, row, overlay_width);
        ov_r0 = std::min<size_t>(ov_r0, y);
        ov_r1 = y + 1;
      }
    }
    if (ov_r1 > ov_r0) {
      ov = odst.data();
    } else {
      ov_r0 = ov_r1 = 0;
    }
  }
  jk_esp_us_copy += esp_timer_get_time() - t_copy;
  emu.palette(g_palette565, 256);
  emu.push_frame(dst.data(), ov, ov_r0, ov_r1);
}

void jk_esp_present_report(void) {
  static uint32_t last_presents = 0;
  static uint64_t last_wait = 0, last_copy = 0, last_us = 0;
  const uint64_t now = esp_timer_get_time();
  const uint32_t n = g_presents - last_presents;
  const float secs = (now - last_us) / 1e6f;
  const auto vs = Tab5Emu::get().video_stats();
  const float vf = vs.frames ? static_cast<float>(vs.frames) : 1.0f;
  logger.info("present: {} frames in {:.1f}s = {:.1f} fps; engine wait {:.1f} ms/frame, copy {:.1f} ms/frame; "
              "video task avg {:.1f} ms max {:.1f} ms (convert {:.1f} ms, blit {:.1f} ms, {} tiles/frame)",
              n, secs, secs > 0 ? n / secs : 0.0f, n ? (jk_esp_us_wait - last_wait) / 1000.0f / n : 0.0f,
              n ? (jk_esp_us_copy - last_copy) / 1000.0f / n : 0.0f, get_frame_time_avg() / 1000.0f,
              get_frame_time_max() / 1000.0f, vs.convert_us / 1000.0f / vf, vs.blit_us / 1000.0f / vf,
              vs.frames ? vs.tiles / vs.frames : 0);
  reset_frame_time();
  last_presents = g_presents;
  last_wait = jk_esp_us_wait;
  last_copy = jk_esp_us_copy;
  last_us = now;
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
  const size_t bytes = num_frames * 2 * sizeof(int16_t);
  const size_t queued = emu.play_audio(reinterpret_cast<const uint8_t *>(stereo_pcm), bytes);
  return queued / (2 * sizeof(int16_t));
}

void jk_esp_audio_set_rate(uint32_t sample_rate) { Tab5Emu::get().audio_sample_rate(sample_rate); }

uint32_t jk_esp_audio_rate(void) { return Tab5Emu::get().audio_sample_rate(); }

void jk_esp_audio_lock(void) { g_audio_mutex.lock(); }
void jk_esp_audio_unlock(void) { g_audio_mutex.unlock(); }

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
        logger.info("heap before engine startup: internal {} (largest {}), psram {} (largest {})",
                    heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                    heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
        if (jk_esp_engine_startup(g_config.game_dir.c_str())) {
          g_engine_started_ok = true;
          while (!g_engine_stop && !jk_esp_quit_requested) {
            run_pending_request();
            if (g_paused) {
              g_parked = true;
              vTaskDelay(pdMS_TO_TICKS(10));
              continue;
            }
            g_parked = false;
            if (!jk_esp_engine_frame()) {
              logger.warn("engine frame returned 0");
              break;
            }
          }
          logger.info("engine loop done (stop={} quit={})", g_engine_stop.load(), jk_esp_quit_requested);
          jk_esp_engine_shutdown();
          logger.info("heap after engine shutdown: internal {} (largest {}), psram {} (largest {})",
                      heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_largest_free_block(MALLOC_CAP_INTERNAL),
                      heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
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
  // audio: top the HAL's output queue up every 10 ms (the mixer produces
  // as much as the queue accepts, so this paces itself by back-pressure)
  g_audio_task = espp::Task::make_unique({
      .callback =
          [](std::mutex &m, std::condition_variable &cv, bool &task_notified) {
            if (!g_paused) {
              stdSound_ESP32_Pump();
            }
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, 10ms, [&] { return task_notified; });
            return false;
          },
      .task_config = {.name = "jk_audio", .stack_size_bytes = 6 * 1024, .priority = 15, .core_id = 1},
  });
  g_audio_task->start();
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
  g_audio_task.reset();
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

void pause() {
  g_paused = true;
#if CONFIG_JK_ENGINE
  // wait for the frame in flight to finish so the engine's buffers are stable
  for (int i = 0; i < 200 && g_engine_running && g_engine_started_ok && !g_parked; i++) {
    std::this_thread::sleep_for(10ms);
  }
  if (g_engine_running && !g_parked) {
    logger.warn("pause: engine did not park");
  }
#endif
}
void resume() { g_paused = false; }

void reset() {
  logger.info("reset (restart level)");
#if CONFIG_JK_ENGINE
  if (!request(Request::RESET)) {
    logger.warn("reset: nothing to restart");
  }
#else
  g_pattern_frame = 0;
#endif
}

bool save(const std::string &path, int slot) {
  logger.info("save slot {} -> {}", slot, path);
#if CONFIG_JK_ENGINE
  if (path.empty()) {
    return false;
  }
  return request(Request::SAVE, path, fmt::format("Slot {}", slot));
#else
  return false;
#endif
}

bool load(const std::string &path, int slot) {
  logger.info("load slot {} <- {}", slot, path);
#if CONFIG_JK_ENGINE
  if (path.empty()) {
    return false;
  }
  return request(Request::LOAD, path);
#else
  return false;
#endif
}

std::pair<size_t, size_t> video_size() {
  // the size of the frame most recently presented (menus 640x480, the
  // internal resolution in-game)
  if (g_last_w > 0 && g_last_h > 0) {
    return {(size_t)g_last_w, (size_t)g_last_h};
  }
  return {(size_t)g_native_w, (size_t)g_native_h};
}

void apply_video_setting() {
  if (g_last_w > 0 && g_last_h > 0) {
    apply_video_setting_for(g_last_w, g_last_h);
  }
}

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

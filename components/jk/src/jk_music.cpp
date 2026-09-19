// Music streaming for the JK core: Ogg Vorbis (the GOG / Steam soundtrack,
// MUSIC/Track*.ogg) decoded with stb_vorbis on a low-priority task into a
// PCM ring buffer, which the engine's software mixer drains with linear
// resampling and the music volume. Single producer (decode task), single
// consumer (audio task, under the engine's audio lock).
#include "jk_esp.h"

#include <algorithm>
#include <atomic>
#include <cmath>
#include <cstring>
#include <mutex>
#include <string>

#include <esp_heap_caps.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

#include "logger.hpp"

#include "stb_vorbis.h"

namespace {
espp::Logger logger({.tag = "jk_music", .level = espp::Logger::Verbosity::INFO});

constexpr size_t RING_FRAMES = 1 << 16;      // 65536 stereo frames = ~1.5 s at 44.1 kHz
constexpr size_t DECODE_CHUNK_FRAMES = 2048; // per decode call
constexpr size_t VORBIS_ALLOC_BYTES = 640 * 1024;
constexpr size_t DECODE_STACK_BYTES = 48 * 1024;

int16_t *g_ring = nullptr; // interleaved stereo
std::atomic<size_t> g_ring_w{0}, g_ring_r{0};
std::atomic<uint32_t> g_rate{44100};
std::atomic<bool> g_eof{true};      // decoder reached the end of the file (or no file)
std::atomic<bool> g_active{false};  // a file is open
std::atomic<int> g_volume256{256};
std::atomic<uint32_t> g_generation{0};
uint32_t g_pos_fixed = 0; // resampler phase (16.16), consumer side

std::mutex g_open_mutex; // serializes open/stop against the decode task
stb_vorbis *g_vorbis = nullptr;
char *g_vorbis_alloc = nullptr;
std::string g_pending_path; // set by open(), consumed by the decode task
std::atomic<bool> g_open_request{false};
std::atomic<bool> g_stop_request{false};
TaskHandle_t g_task = nullptr;

// decode statistics
uint64_t g_decode_us = 0;
uint32_t g_decode_frames = 0;
int64_t g_last_report_us = 0;

size_t ring_available() {
  return (g_ring_w.load(std::memory_order_acquire) - g_ring_r.load(std::memory_order_relaxed)) & (RING_FRAMES - 1);
}
size_t ring_space() { return RING_FRAMES - 1 - ring_available(); }

void close_file() {
  if (g_vorbis) {
    stb_vorbis_close(g_vorbis);
    g_vorbis = nullptr;
  }
  g_active = false;
  g_eof = true;
}

bool open_file(const std::string &path) {
  close_file();
  int error = 0;
  stb_vorbis_alloc alloc = {g_vorbis_alloc, static_cast<int>(VORBIS_ALLOC_BYTES)};
  g_vorbis = stb_vorbis_open_filename(path.c_str(), &error, &alloc);
  if (!g_vorbis) {
    logger.warn("could not open '{}' (stb_vorbis error {})", path, error);
    return false;
  }
  const auto info = stb_vorbis_get_info(g_vorbis);
  logger.info("playing '{}': {} Hz, {} ch, setup {} KB", path, info.sample_rate, info.channels,
              info.setup_memory_required / 1024);
  g_rate = info.sample_rate;
  g_ring_w = g_ring_r = 0;
  g_pos_fixed = 0;
  g_eof = false;
  g_active = true;
  g_generation++;
  return true;
}

void decode_task(void *) {
  int16_t chunk[DECODE_CHUNK_FRAMES * 2];
  while (true) {
    if (g_stop_request.exchange(false)) {
      std::lock_guard<std::mutex> lk(g_open_mutex);
      close_file();
    }
    if (g_open_request.exchange(false)) {
      std::lock_guard<std::mutex> lk(g_open_mutex);
      open_file(g_pending_path);
    }
    if (!g_active || g_eof) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }
    if (ring_space() < DECODE_CHUNK_FRAMES) {
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    int got = 0;
    {
      std::lock_guard<std::mutex> lk(g_open_mutex);
      if (!g_vorbis) {
        continue;
      }
      const auto t0 = esp_timer_get_time();
      got = stb_vorbis_get_samples_short_interleaved(g_vorbis, 2, chunk, DECODE_CHUNK_FRAMES * 2);
      g_decode_us += esp_timer_get_time() - t0;
    }
    if (got <= 0) {
      g_eof = true;
      logger.info("end of track");
      continue;
    }
    g_decode_frames += got;
    // copy into the ring (may wrap)
    size_t w = g_ring_w.load(std::memory_order_relaxed);
    for (int i = 0; i < got; i++) {
      g_ring[2 * w] = chunk[2 * i];
      g_ring[2 * w + 1] = chunk[2 * i + 1];
      w = (w + 1) & (RING_FRAMES - 1);
    }
    g_ring_w.store(w, std::memory_order_release);
    const auto now = esp_timer_get_time();
    if (now - g_last_report_us > 30'000'000) {
      const float secs = g_decode_frames / static_cast<float>(g_rate.load());
      logger.info("decode: {:.1f} s of audio in {:.1f} ms ({:.1f}% of a core)", secs, g_decode_us / 1000.0f,
                  secs > 0 ? (g_decode_us / 1e6f) / secs * 100.0f : 0.0f);
      g_decode_us = 0;
      g_decode_frames = 0;
      g_last_report_us = now;
    }
  }
}

bool ensure_started() {
  if (g_task) {
    return true;
  }
  g_ring = static_cast<int16_t *>(heap_caps_malloc(RING_FRAMES * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM));
  g_vorbis_alloc = static_cast<char *>(heap_caps_malloc(VORBIS_ALLOC_BYTES, MALLOC_CAP_SPIRAM));
  if (!g_ring || !g_vorbis_alloc) {
    logger.error("no memory for the music streamer");
    return false;
  }
  if (xTaskCreatePinnedToCoreWithCaps(decode_task, "jk_music", DECODE_STACK_BYTES, nullptr, 4, &g_task, 1,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
    logger.error("could not create the music decode task");
    g_task = nullptr;
    return false;
  }
  return true;
}
} // namespace

extern "C" {

int jk_esp_music_open(const char *path) {
  if (!path || !ensure_started()) {
    return 0;
  }
  std::string full = path;
  if (full[0] != '/') {
    full = std::string(jk_esp_game_dir()) + "/" + full;
  }
  for (auto &c : full) {
    if (c == '\\') c = '/';
  }
  // cheap existence check so the engine can fall through its candidate names
  FILE *f = fopen(full.c_str(), "rb");
  if (!f) {
    return 0;
  }
  fclose(f);
  {
    std::lock_guard<std::mutex> lk(g_open_mutex);
    g_pending_path = full;
  }
  g_eof = false; // report "playing" until the decoder says otherwise
  g_active = true;
  g_open_request = true;
  return 1;
}

void jk_esp_music_stop(void) {
  if (!g_task) {
    return;
  }
  g_stop_request = true;
  g_active = false;
  g_eof = true;
}

int jk_esp_music_playing(void) { return g_active && (!g_eof || ring_available() > 0); }

void jk_esp_music_volume(float volume) {
  g_volume256 = static_cast<int>(std::clamp(volume, 0.0f, 1.0f) * 256.0f);
}

void jk_esp_music_mix(int16_t *stereo, size_t frames, uint32_t rate) {
  if (!g_ring || !g_active || rate == 0) {
    return;
  }
  const int vol = g_volume256.load();
  if (vol <= 0) {
    return;
  }
  const uint32_t step = static_cast<uint32_t>((static_cast<uint64_t>(g_rate.load()) << 16) / rate);
  size_t r = g_ring_r.load(std::memory_order_relaxed);
  const size_t avail = ring_available();
  size_t consumed = 0; // whole source frames consumed
  uint32_t pos = g_pos_fixed;
  for (size_t i = 0; i < frames; i++) {
    const size_t idx = pos >> 16;
    if (idx + 1 >= avail) {
      break; // starved (or end of data): leave the rest untouched
    }
    const uint32_t frac = pos & 0xFFFF;
    const size_t a = (r + idx) & (RING_FRAMES - 1);
    const size_t b = (r + idx + 1) & (RING_FRAMES - 1);
    const int32_t l = (g_ring[2 * a] * static_cast<int32_t>(0x10000 - frac) + g_ring[2 * b] * static_cast<int32_t>(frac)) >> 16;
    const int32_t rr = (g_ring[2 * a + 1] * static_cast<int32_t>(0x10000 - frac) + g_ring[2 * b + 1] * static_cast<int32_t>(frac)) >> 16;
    int32_t ml = stereo[2 * i] + ((l * vol) >> 8);
    int32_t mr = stereo[2 * i + 1] + ((rr * vol) >> 8);
    stereo[2 * i] = static_cast<int16_t>(std::clamp<int32_t>(ml, -32768, 32767));
    stereo[2 * i + 1] = static_cast<int16_t>(std::clamp<int32_t>(mr, -32768, 32767));
    pos += step;
  }
  consumed = pos >> 16;
  g_pos_fixed = pos & 0xFFFF;
  g_ring_r.store((r + consumed) & (RING_FRAMES - 1), std::memory_order_release);
}

} // extern "C"

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
constexpr size_t DECODE_CHUNK_FRAMES = 4096; // per decode call (>= one IMA block)
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

// IMA ADPCM WAV (the converted soundtrack, see tools/convert_music.py):
// 4 bits per sample, blocks of `block_align` bytes holding `samples_per_block`
// frames, decoded here with the standard IMA tables. Costs almost nothing.
struct ImaWav {
  FILE *f{nullptr};
  uint16_t channels{0};
  uint32_t rate{0};
  uint16_t block_align{0};
  uint16_t samples_per_block{0};
  long data_end{0};
  uint8_t *block{nullptr};
};
ImaWav g_ima{};
const int16_t IMA_STEP_TABLE[89] = {
    7,     8,     9,     10,    11,    12,    13,    14,    16,    17,    19,    21,    23,    25,    28,
    31,    34,    37,    41,    45,    50,    55,    60,    66,    73,    80,    88,    97,    107,   118,
    130,   143,   157,   173,   190,   209,   230,   253,   279,   307,   337,   371,   408,   449,   494,
    544,   598,   658,   724,   796,   876,   963,   1060,  1166,  1282,  1411,  1552,  1707,  1878,  2066,
    2272,  2499,  2749,  3024,  3327,  3660,  4026,  4428,  4871,  5358,  5894,  6484,  7132,  7845,  8630,
    9493,  10442, 11487, 12635, 13899, 15289, 16818, 18500, 20350, 22385, 24623, 27086, 29794, 32767};
const int8_t IMA_INDEX_TABLE[16] = {-1, -1, -1, -1, 2, 4, 6, 8, -1, -1, -1, -1, 2, 4, 6, 8};

inline int16_t ima_step(int nibble, int32_t &predictor, int &index) {
  const int step = IMA_STEP_TABLE[index];
  int diff = step >> 3;
  if (nibble & 1) diff += step >> 2;
  if (nibble & 2) diff += step >> 1;
  if (nibble & 4) diff += step;
  if (nibble & 8) diff = -diff;
  predictor = std::clamp<int32_t>(predictor + diff, -32768, 32767);
  index = std::clamp(index + IMA_INDEX_TABLE[nibble], 0, 88);
  return static_cast<int16_t>(predictor);
}

void ima_close() {
  if (g_ima.f) {
    fclose(g_ima.f);
  }
  free(g_ima.block);
  g_ima = {};
}

bool ima_open(const std::string &path) {
  ima_close();
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return false;
  uint8_t hdr[12];
  if (fread(hdr, 1, 12, f) != 12 || memcmp(hdr, "RIFF", 4) || memcmp(hdr + 8, "WAVE", 4)) {
    fclose(f);
    return false;
  }
  bool have_fmt = false;
  while (true) {
    uint8_t ch[8];
    if (fread(ch, 1, 8, f) != 8) break;
    const uint32_t size = ch[4] | (ch[5] << 8) | (ch[6] << 16) | (static_cast<uint32_t>(ch[7]) << 24);
    const long next = ftell(f) + static_cast<long>(size + (size & 1));
    if (!memcmp(ch, "fmt ", 4)) {
      uint8_t fmt[20] = {};
      if (fread(fmt, 1, std::min<size_t>(size, sizeof(fmt)), f) < 16) break;
      const uint16_t tag = fmt[0] | (fmt[1] << 8);
      g_ima.channels = fmt[2] | (fmt[3] << 8);
      g_ima.rate = fmt[4] | (fmt[5] << 8) | (fmt[6] << 16) | (static_cast<uint32_t>(fmt[7]) << 24);
      g_ima.block_align = fmt[12] | (fmt[13] << 8);
      const uint16_t bits = fmt[14] | (fmt[15] << 8);
      g_ima.samples_per_block = size >= 20 ? (fmt[18] | (fmt[19] << 8)) : 0;
      if (tag != 0x11 || bits != 4 || (g_ima.channels != 1 && g_ima.channels != 2) || g_ima.block_align == 0) {
        logger.warn("'{}': not IMA ADPCM (tag 0x{:x}, {} bits, {} ch)", path, tag, bits, g_ima.channels);
        fclose(f);
        return false;
      }
      if (!g_ima.samples_per_block) {
        g_ima.samples_per_block = (g_ima.block_align - 4 * g_ima.channels) * 8 / (4 * g_ima.channels) + 1;
      }
      have_fmt = true;
    } else if (!memcmp(ch, "data", 4)) {
      if (!have_fmt) break;
      g_ima.f = f;
      g_ima.data_end = ftell(f) + static_cast<long>(size);
      g_ima.block = static_cast<uint8_t *>(malloc(g_ima.block_align));
      if (!g_ima.block) break;
      return true;
    }
    fseek(f, next, SEEK_SET);
  }
  fclose(f);
  g_ima = {};
  return false;
}

// decode one block into `out` (interleaved stereo); returns frames decoded
int ima_decode_block(int16_t *out) {
  if (!g_ima.f || ftell(g_ima.f) >= g_ima.data_end) return 0;
  const size_t n = fread(g_ima.block, 1, g_ima.block_align, g_ima.f);
  if (n < static_cast<size_t>(4 * g_ima.channels)) return 0;
  const int ch = g_ima.channels;
  int32_t pred[2] = {0, 0};
  int idx[2] = {0, 0};
  for (int c = 0; c < ch; c++) {
    const uint8_t *h = g_ima.block + 4 * c;
    pred[c] = static_cast<int16_t>(h[0] | (h[1] << 8));
    idx[c] = std::clamp<int>(h[2], 0, 88);
    out[c] = static_cast<int16_t>(pred[c]);
  }
  if (ch == 1) out[1] = out[0];
  int frames = 1;
  const uint8_t *p = g_ima.block + 4 * ch;
  const uint8_t *end = g_ima.block + n;
  // data: per channel 4 bytes (8 nibbles) at a time, channels interleaved
  while (p + 4 * ch <= end && frames + 8 <= g_ima.samples_per_block) {
    for (int c = 0; c < ch; c++) {
      const uint8_t *q = p + 4 * c;
      for (int i = 0; i < 8; i++) {
        const int nib = (i & 1) ? (q[i >> 1] >> 4) : (q[i >> 1] & 0x0F);
        const int16_t v = ima_step(nib, pred[c], idx[c]);
        out[2 * (frames + i) + c] = v;
        if (ch == 1) out[2 * (frames + i) + 1] = v;
      }
    }
    frames += 8;
    p += 4 * ch;
  }
  return frames;
}
char *g_vorbis_alloc = nullptr;
std::string g_pending_path; // set by open(), consumed by the decode task
std::atomic<bool> g_open_request{false};
std::atomic<bool> g_stop_request{false};
std::atomic<bool> g_quit_request{false}; // the decode task parks itself (deinit deletes it)
std::atomic<bool> g_parked{false};
TaskHandle_t g_task = nullptr;

// decode statistics
uint64_t g_decode_us = 0, g_decode_max_us = 0;
uint32_t g_decode_frames = 0, g_decode_calls = 0;
std::atomic<uint32_t> g_starved_frames{0}; // output frames the mixer had no music for
int64_t g_last_report_us = 0;


size_t ring_available() {
  return (g_ring_w.load(std::memory_order_acquire) - g_ring_r.load(std::memory_order_relaxed)) & (RING_FRAMES - 1);
}
size_t ring_space() { return RING_FRAMES - 1 - ring_available(); }

void report(const char *state) {
  const float secs = g_decode_frames / static_cast<float>(g_rate.load());
  logger.info("{}: decoded {:.1f} s in {} calls, {:.1f} ms total, max {:.1f} ms ({:.1f}% of a core); buffered {} frames; "
              "starved {} output frames",
              state, secs, g_decode_calls, g_decode_us / 1000.0f, g_decode_max_us / 1000.0f,
              secs > 0 ? (g_decode_us / 1e6f) / secs * 100.0f : 0.0f, ring_available(), g_starved_frames.exchange(0));
  g_decode_us = g_decode_max_us = 0;
  g_decode_frames = g_decode_calls = 0;
}

void close_file() {
  ima_close();
  if (g_vorbis) {
    stb_vorbis_close(g_vorbis);
    g_vorbis = nullptr;
  }
  g_active = false;
  g_eof = true;
}

bool open_file(const std::string &path) {
  close_file();
  // converted soundtrack (tools/convert_music.py): same name, .wav
  {
    std::string wav = path;
    const auto dot = wav.rfind('.');
    if (dot != std::string::npos) wav.resize(dot);
    wav += ".wav";
    if (ima_open(wav)) {
      logger.info("playing '{}': IMA ADPCM {} Hz, {} ch, {} frames/block", wav, g_ima.rate, g_ima.channels,
                  g_ima.samples_per_block);
      g_rate = g_ima.rate;
      g_ring_w = g_ring_r = 0;
      g_pos_fixed = 0;
      g_eof = false;
      g_active = true;
      g_generation++;
      return true;
    }
  }
  int error = 0;
  if (!g_vorbis_alloc) { // only .ogg needs it (640 KB); the ADPCM path never allocates it
    g_vorbis_alloc = static_cast<char *>(heap_caps_malloc(VORBIS_ALLOC_BYTES, MALLOC_CAP_SPIRAM));
    if (!g_vorbis_alloc) {
      logger.warn("no memory for the vorbis decoder");
      return false;
    }
  }
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
    if (g_quit_request) {
      {
        std::lock_guard<std::mutex> lk(g_open_mutex);
        close_file();
      }
      g_parked = true;
      vTaskSuspend(nullptr); // a WithCaps task cannot free its own stack
    }
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
      const auto now = esp_timer_get_time();
      if (now - g_last_report_us > 15'000'000) {
        report("ring full");
        g_last_report_us = now;
      }
      vTaskDelay(pdMS_TO_TICKS(10));
      continue;
    }
    int got = 0;
    {
      std::lock_guard<std::mutex> lk(g_open_mutex);
      if (!g_vorbis && !g_ima.f) {
        continue;
      }
      const auto t0 = esp_timer_get_time();
      if (g_ima.f) {
        // as many whole blocks as fit the chunk
        while (got + g_ima.samples_per_block <= static_cast<int>(DECODE_CHUNK_FRAMES)) {
          const int n = ima_decode_block(chunk + 2 * got);
          if (n <= 0) break;
          got += n;
        }
      } else {
        got = stb_vorbis_get_samples_short_interleaved(g_vorbis, 2, chunk, DECODE_CHUNK_FRAMES * 2);
      }
      const uint64_t dt = esp_timer_get_time() - t0;
      g_decode_us += dt;
      g_decode_max_us = std::max(g_decode_max_us, dt);
      g_decode_calls++;
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
    if (now - g_last_report_us > 15'000'000) {
      report("decoding");
      g_last_report_us = now;
    }
  }
}

bool ensure_started() {
  if (g_task) {
    return true;
  }
  g_ring = static_cast<int16_t *>(heap_caps_malloc(RING_FRAMES * 2 * sizeof(int16_t), MALLOC_CAP_SPIRAM));
  if (!g_ring) {
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

// Release the streamer (task, ring, decoder buffer): the engine session is
// over, and these would otherwise stay in the middle of the PSRAM heap.
void jk_esp_music_deinit(void) {
  if (g_task) {
    g_active = false;
    g_eof = true;
    g_quit_request = true;
    for (int i = 0; i < 200 && !g_parked; i++) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (g_parked) {
      vTaskDeleteWithCaps(g_task);
    } else {
      logger.error("music decode task did not park; leaking it");
    }
    g_task = nullptr;
    g_parked = false;
    g_quit_request = false;
  }
  heap_caps_free(g_ring);
  g_ring = nullptr;
  heap_caps_free(g_vorbis_alloc);
  g_vorbis_alloc = nullptr;
  g_ring_w = g_ring_r = 0;
}

void jk_esp_music_volume(float volume) {
  const int v = static_cast<int>(std::clamp(volume, 0.0f, 1.0f) * 256.0f);
  if (v != g_volume256.load()) {
    logger.info("volume {:.2f}", volume);
  }
  g_volume256 = v;
}

void jk_esp_music_mix(int16_t *stereo, size_t frames, uint32_t rate) {
  if (!g_ring || !g_active || rate == 0) {
    return;
  }
  // always consume (keeps the stream's timing) even when the volume is 0
  const int vol = g_volume256.load();
  const uint32_t step = static_cast<uint32_t>((static_cast<uint64_t>(g_rate.load()) << 16) / rate);
  size_t r = g_ring_r.load(std::memory_order_relaxed);
  const size_t avail = ring_available();
  size_t consumed = 0; // whole source frames consumed
  uint32_t pos = g_pos_fixed;
  for (size_t i = 0; i < frames; i++) {
    const size_t idx = pos >> 16;
    if (idx + 1 >= avail) {
      // starved (or end of data): leave the rest untouched
      g_starved_frames += frames - i;
      break;
    }
    const uint32_t frac = pos & 0xFFFF;
    const size_t a = (r + idx) & (RING_FRAMES - 1);
    const size_t b = (r + idx + 1) & (RING_FRAMES - 1);
    const int32_t l = (g_ring[2 * a] * static_cast<int32_t>(0x10000 - frac) + g_ring[2 * b] * static_cast<int32_t>(frac)) >> 16;
    const int32_t rr = (g_ring[2 * a + 1] * static_cast<int32_t>(0x10000 - frac) + g_ring[2 * b + 1] * static_cast<int32_t>(frac)) >> 16;
    if (vol > 0) {
      int32_t ml = stereo[2 * i] + ((l * vol) >> 8);
      int32_t mr = stereo[2 * i + 1] + ((rr * vol) >> 8);
      stereo[2 * i] = static_cast<int16_t>(std::clamp<int32_t>(ml, -32768, 32767));
      stereo[2 * i + 1] = static_cast<int16_t>(std::clamp<int32_t>(mr, -32768, 32767));
    }
    pos += step;
  }
  consumed = pos >> 16;
  g_pos_fixed = pos & 0xFFFF;
  g_ring_r.store((r + consumed) & (RING_FRAMES - 1), std::memory_order_release);
}

} // extern "C"

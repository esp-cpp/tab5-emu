// The game itself: tpm-decomp's static recompilation of SLUS_008.84 (built
// from the user's own executable at configure time, see CMakeLists.txt) and
// its runtime, on the Tab5.
//
// Threads:
//   tpm_game  (core 0) the recompiled code; the runtime's hardware model runs
//             inline at its loop back-edges (vblank, CD, SPU, interrupts)
//   tpm_video (core 1) hands the frame the GPU worker converted at the
//             vblank to the HAL
//   tpm_audio (core 1) moves the SPU's samples to the HAL's audio queue
//
// The disc is a raw 2352-byte-sector image on the card (the .bin of a
// bin/cue rip); the executable comes from the card if it is there as a file,
// otherwise out of the image's ISO9660 directory.

#include "tpm_engine.hpp"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <esp_heap_caps.h>
#include <esp_rom_sys.h>
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/semphr.h>
#include <freertos/task.h>
#include <freertos/idf_additions.h>

#include "sdkconfig.h"

#include "logger.hpp"
#include "statistics.hpp"
#include "tab5-emu.hpp"

extern "C" {
#include "rt_platform.h"
#include "shared_memory.h"
// tpm_rt.ld: the runtime's statics, for the reset between launches
extern uint8_t _tpm_bss_start[], _tpm_bss_end[], _tpm_data_start[], _tpm_data_end[];
#if TPM_GPU_BENCH
extern const uint8_t _binary_gpubench_bin_start[], _binary_gpubench_bin_end[];
#endif
}

namespace tpm::engine {

namespace {

espp::Logger logger({.tag = "tpm", .level = espp::Logger::Verbosity::INFO});

constexpr size_t FRAME_W = 320;
constexpr size_t FRAME_H = 240;
constexpr uint32_t AUDIO_RATE = 44100;

FILE *g_disc{nullptr};
uint32_t g_disc_sectors{0};
std::vector<uint8_t> g_exe;

TaskHandle_t g_game_task{nullptr};
TaskHandle_t g_video_task{nullptr};
TaskHandle_t g_gpu_task{nullptr};  // core 1: the GPU's command stream, top band
TaskHandle_t g_gpu0_task{nullptr}; // core 0: the bottom band, in the game's idle time
volatile int g_gpu_stop{0};
volatile bool g_gpu_done[2]{};
TaskHandle_t g_audio_task{nullptr};
std::atomic<bool> g_game_done{false};
std::atomic<bool> g_stop{false};
std::atomic<bool> g_paused{false};
SemaphoreHandle_t g_resume{nullptr};

uint8_t *g_frames[2]{};
int g_frame_index{0};
std::atomic<uint8_t *> g_last_frame{nullptr};
std::atomic<bool> g_video_busy{false};
RtDisplay g_display{};
std::atomic<uint32_t> g_flips{0}; // display-area moves: the game's frames

// SPU output: the game's thread writes, the audio task reads
constexpr size_t RING_FRAMES = 8192; // ~186 ms
int16_t *g_ring{nullptr};
std::atomic<uint32_t> g_ring_w{0}, g_ring_r{0};

// ---- a fresh machine for every launch ----
//
// The runtime's statics are put back before each boot, as the MGS port does
// with the game's: .bss (in PSRAM, tpm_rt.ld) zeroed, .data restored from a
// copy taken before the first launch. Its big buffers (guest RAM, VRAM, SPU,
// transfers) come from the shared-memory pool (plat_alloc) and go back to it
// when the cart quits.
std::vector<uint8_t> g_data_snapshot;

void reset_statics() {
  const size_t data_size = _tpm_data_end - _tpm_data_start;
  if (g_data_snapshot.empty()) {
    g_data_snapshot.assign(_tpm_data_start, _tpm_data_end);
  } else {
    memcpy(_tpm_data_start, g_data_snapshot.data(), data_size);
  }
  memset(_tpm_bss_start, 0, _tpm_bss_end - _tpm_bss_start);
  logger.info("statics: {} B .bss (PSRAM), {} B .data", static_cast<size_t>(_tpm_bss_end - _tpm_bss_start),
              data_size);
}

// ---- bring-up aids (Kconfig "Test:" options) ----

struct Press {
  uint32_t vblank;
  uint16_t bits;
};
std::vector<Press> g_presses;
constexpr uint32_t PRESS_HOLD = 8;

void parse_presses(const char *spec) {
  static const struct {
    const char *name;
    int bit;
  } names[] = {{"select", 0}, {"start", 3}, {"up", 4},  {"right", 5}, {"down", 6},      {"left", 7},     {"l2", 8},
               {"r2", 9},     {"l1", 10},   {"r1", 11}, {"triangle", 12}, {"circle", 13}, {"cross", 14}, {"square", 15}};
  g_presses.clear();
  std::string all(spec);
  size_t pos = 0;
  while (pos < all.size()) {
    size_t end = all.find(',', pos);
    if (end == std::string::npos) end = all.size();
    const std::string item = all.substr(pos, end - pos);
    pos = end + 1;
    const size_t colon = item.find(':');
    if (colon == std::string::npos) continue;
    const auto when = static_cast<uint32_t>(strtoul(item.c_str(), nullptr, 10));
    const std::string button = item.substr(colon + 1);
    for (const auto &n : names) {
      if (button == n.name) {
        g_presses.push_back({when, static_cast<uint16_t>(1u << n.bit)});
      }
    }
  }
  if (!g_presses.empty()) {
    logger.warn("scripted presses: {}", spec);
  }
}

uint16_t scripted_buttons(uint32_t vblank) {
  uint16_t b = 0;
  for (const auto &p : g_presses) {
    if (vblank >= p.vblank && vblank < p.vblank + PRESS_HOLD) b |= p.bits;
  }
  return b;
}

// the frame, at half resolution, as base64 lines on the console (the
// USB-serial console moves ~10 KB/s: a full frame would take 20 s)
constexpr size_t SHOT_W = FRAME_W / 2, SHOT_H = FRAME_H / 2;
uint16_t *g_shot{nullptr};
size_t g_shot_w{SHOT_W}, g_shot_h{SHOT_H};
uint32_t g_shot_vblank{0};
TaskHandle_t g_shot_task{nullptr};

void print_screenshot(const uint16_t *px, size_t w, size_t h, uint32_t vblank) {
  static const char tbl[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
  const auto *b = reinterpret_cast<const uint8_t *>(px);
  const size_t N = w * h * 2;
  constexpr size_t CHUNK = 960; // bytes per line, a multiple of 3
  char line[CHUNK / 3 * 4 + 1];
  printf("TPMFB begin %u %u %u\n", static_cast<unsigned>(w), static_cast<unsigned>(h),
         static_cast<unsigned>(vblank));
  for (size_t off = 0; off < N; off += CHUNK) {
    const size_t n = std::min(CHUNK, N - off);
    size_t o = 0;
    for (size_t i = 0; i < n; i += 3) {
      const uint32_t v = (b[off + i] << 16) | ((i + 1 < n ? b[off + i + 1] : 0) << 8) | (i + 2 < n ? b[off + i + 2] : 0);
      line[o++] = tbl[(v >> 18) & 63];
      line[o++] = tbl[(v >> 12) & 63];
      line[o++] = i + 1 < n ? tbl[(v >> 6) & 63] : '=';
      line[o++] = i + 2 < n ? tbl[v & 63] : '=';
    }
    line[o] = 0;
    printf("TPMFB %u %s\n", static_cast<unsigned>(off / CHUNK), line);
  }
  printf("TPMFB end\n");
  fflush(stdout);
}

void shot_task(void *) {
  while (!g_stop.load()) {
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0) {
      print_screenshot(g_shot, g_shot_w, g_shot_h, g_shot_vblank);
    }
  }
  g_shot_task = nullptr;
  vTaskDelete(nullptr);
}

// A sampling profiler for the game thread and the GPU worker
// (CONFIG_TPM_PROFILE_VBLANK): a task above the sampled one on its core
// wakes every tick; the sampled task is then preempted, its register frame
// on its stack, the TCB's first word pointing at it (mepc, ra, sp, ...). The
// samples go to the console as "TPMPROF pc ra" (game) / "TPMPROFG pc ra"
// (GPU) for addr2line on the desktop.
#if CONFIG_TPM_PROFILE_VBLANK > 0
void prof_task(void *arg) {
  const bool gpu = arg != nullptr;
  const char *tag = gpu ? "TPMPROFG" : "TPMPROF";
  constexpr int N = 8000;
  auto *pcs = static_cast<uint32_t *>(heap_caps_malloc(N * 2 * sizeof(uint32_t), MALLOC_CAP_SPIRAM));
  while (!g_stop.load() && rt_frame_count() < CONFIG_TPM_PROFILE_VBLANK) {
    vTaskDelay(pdMS_TO_TICKS(100));
  }
  int n = 0;
  while (!g_stop.load() && pcs && n < N) {
    vTaskDelay(1);
    TaskHandle_t t = gpu ? g_gpu_task : g_game_task;
    if (t && eTaskGetState(t) == eReady) {
      const uint32_t *top = *reinterpret_cast<uint32_t *const *>(t);
      pcs[2 * n] = top[0];
      pcs[2 * n + 1] = top[1];
      n++;
    }
  }
  if (gpu) {
    vTaskDelay(pdMS_TO_TICKS(1500)); // after the game's
  }
  for (int i = 0; i < n; i++) {
    printf("%s %08x %08x\n", tag, static_cast<unsigned>(pcs[2 * i]), static_cast<unsigned>(pcs[2 * i + 1]));
    if ((i & 63) == 63) vTaskDelay(1);
  }
  printf("%s end %d\n", tag, n);
  heap_caps_free(pcs);
  vTaskDelete(nullptr);
}
#endif

#if CONFIG_TPM_TEST_PAUSE_VBLANK > 0
void pause_test_task(void *) {
  while (!g_stop.load() && rt_frame_count() < CONFIG_TPM_TEST_PAUSE_VBLANK) {
    vTaskDelay(pdMS_TO_TICKS(50));
  }
  for (int round = 0; round < 3 && !g_stop.load(); round++) {
    pause();
    const auto f = last_frame();
    const auto *px = reinterpret_cast<const uint16_t *>(f.data());
    std::string rows;
    size_t lit_total = 0;
    for (size_t y = 0; px && y < FRAME_H; y++) {
      size_t lit = 0;
      for (size_t x = 0; x < FRAME_W; x++) lit += px[y * FRAME_W + x] != 0;
      lit_total += lit;
      if (y % 24 == 0) rows += fmt::format(" {}", lit);
    }
    logger.warn("pause test {}: last frame {} (buffers {} {}), {} lit pixels; per 24th row:{}", round,
                fmt::ptr(px), fmt::ptr(g_frames[0]), fmt::ptr(g_frames[1]), lit_total, rows);
    vTaskDelay(pdMS_TO_TICKS(500));
    resume();
    vTaskDelay(pdMS_TO_TICKS(3000));
  }
  vTaskDelete(nullptr);
}
#endif

// ---- the disc ----

// Read-ahead: the game streams the disc sequentially, and on the card every
// separate read pays the SD's latency. The window is read at 512-byte file
// offsets into a 128-byte-aligned PSRAM buffer, so FATFS hands the SD driver
// whole sectors at a DMA-able address: an unaligned read makes the driver
// bounce through internal DMA memory, which the Tab5 does not have to spare
// (reads failed outright once it ran out, and the game stalled on the disc).
constexpr uint32_t RA_BYTES = 64 * 1024;
uint8_t *g_ra{nullptr};
uint64_t g_ra_off{0};
uint32_t g_ra_len{0};

bool read_sector(uint32_t lba, uint8_t *out) {
  if (!g_disc || lba >= g_disc_sectors) {
    return false;
  }
  const uint64_t off = static_cast<uint64_t>(lba) * 2352;
  if (!g_ra || off < g_ra_off || off + 2352 > g_ra_off + g_ra_len) {
    if (!g_ra) {
      return fseek(g_disc, static_cast<long>(off), SEEK_SET) == 0 && fread(out, 1, 2352, g_disc) == 2352;
    }
    g_ra_off = off & ~static_cast<uint64_t>(511);
    g_ra_len = 0;
    if (fseek(g_disc, static_cast<long>(g_ra_off), SEEK_SET) != 0) {
      return false;
    }
    g_ra_len = static_cast<uint32_t>(fread(g_ra, 1, RA_BYTES, g_disc));
    if (off + 2352 > g_ra_off + g_ra_len) {
      return false;
    }
  }
  memcpy(out, g_ra + (off - g_ra_off), 2352);
  return true;
}

// SLUS_008.84 out of the image's root directory (ISO9660, Mode 2 Form 1)
bool exe_from_disc(std::vector<uint8_t> &exe) {
  std::vector<uint8_t> sec(2352);
  if (!read_sector(16, sec.data()) || memcmp(sec.data() + 24 + 1, "CD001", 5) != 0) {
    return false;
  }
  const uint8_t *root = sec.data() + 24 + 156;
  uint32_t lba = root[2] | (root[3] << 8) | (root[4] << 16) | (root[5] << 24);
  uint32_t size = root[10] | (root[11] << 8) | (root[12] << 16) | (root[13] << 24);
  for (uint32_t off = 0; off < size; off += 2048) {
    if (!read_sector(lba + off / 2048, sec.data())) {
      return false;
    }
    const uint8_t *d = sec.data() + 24;
    for (size_t p = 0; p < 2048 && d[p];) {
      const uint8_t len = d[p];
      const uint8_t name_len = d[p + 32];
      const std::string name(reinterpret_cast<const char *>(d + p + 33), name_len);
      if (name.rfind("SLUS_008.84", 0) == 0) {
        const uint32_t f_lba = d[p + 2] | (d[p + 3] << 8) | (d[p + 4] << 16) | (d[p + 5] << 24);
        const uint32_t f_size = d[p + 10] | (d[p + 11] << 8) | (d[p + 12] << 16) | (d[p + 13] << 24);
        exe.resize(f_size);
        for (uint32_t o = 0; o < f_size; o += 2048) {
          if (!read_sector(f_lba + o / 2048, sec.data())) {
            return false;
          }
          memcpy(exe.data() + o, sec.data() + 24, std::min<uint32_t>(2048, f_size - o));
        }
        return true;
      }
      p += len;
    }
  }
  return false;
}

// ---- tasks ----

// Each task's share of its core since the last call (FreeRTOS run-time
// stats): where the board's two cores go
void log_task_load() {
  static std::vector<TaskStatus_t> st;
  static std::vector<std::pair<TaskHandle_t, configRUN_TIME_COUNTER_TYPE>> last;
  static configRUN_TIME_COUNTER_TYPE last_total = 0;
  st.resize(uxTaskGetNumberOfTasks() + 4);
  configRUN_TIME_COUNTER_TYPE total = 0;
  const UBaseType_t n = uxTaskGetSystemState(st.data(), st.size(), &total);
  const auto window = static_cast<float>(total - last_total);
  std::string line;
  for (UBaseType_t i = 0; i < n; i++) {
    const auto &t = st[i];
    configRUN_TIME_COUNTER_TYPE prev = 0;
    for (auto &l : last) {
      if (l.first == t.xHandle) prev = l.second;
    }
    const float pct = window > 0 ? 100.0f * static_cast<float>(t.ulRunTimeCounter - prev) / window : 0;
    if (pct >= 2.0f) {
      line += fmt::format(" {} {:.0f}%", t.pcTaskName, pct);
    }
  }
  last.clear();
  for (UBaseType_t i = 0; i < n; i++) {
    last.emplace_back(st[i].xHandle, st[i].ulRunTimeCounter);
  }
  last_total = total;
  logger.info("  cores:{}", line);
}

// The displayed part of VRAM as the panel's RGB565 frame, one pass from VRAM.
// On the GPU worker at the vblank marker, so it sees exactly the drawing
// queued before it: converted later from another core, the glass in the first
// room and the pause menu came out wrong (the game draws into the frame on
// display at times).
void convert_frame(const RtDisplay &d, uint16_t *dst) {
  if (!d.enabled) {
    memset(dst, 0, FRAME_W * FRAME_H * 2);
  } else {
    // centre narrower modes, sample wider ones; 480-line modes show every
    // other line
    const int w = d.w, h = d.h;
    const int step_y = h > static_cast<int>(FRAME_H) ? 2 : 1;
    const int x_pad = w < static_cast<int>(FRAME_W) ? (FRAME_W - w) / 2 : 0;
    for (size_t y = 0; y < FRAME_H; y++) {
      uint16_t *out = dst + y * FRAME_W;
      const int line = static_cast<int>(y) * step_y;
      if (line >= h || line < d.top || line >= d.top + d.lines) {
        // below the mode's height, or outside the vertical display range
        // (the letterbox of the cutscenes)
        memset(out, 0, FRAME_W * 2);
        continue;
      }
      if (x_pad) {
        memset(out, 0, x_pad * 2);
        memset(out + FRAME_W - x_pad, 0, x_pad * 2);
      }
      const size_t n = std::min<size_t>(w, FRAME_W);
      // the display area's first row shows on the range's first line
      const uint16_t *vrow = &g_rt_vram[((d.y + line - d.top) & 511) * 1024];
      if (d.rgb24) {
        const auto *row = reinterpret_cast<const uint8_t *>(vrow);
        for (size_t x = 0; x < n; x++) {
          const size_t sx = w > static_cast<int>(FRAME_W) ? x * w / FRAME_W : x;
          const uint8_t *p = row + ((d.x * 2 + sx * 3) & 2047);
          out[x_pad + x] = static_cast<uint16_t>(((p[0] & 0xF8) << 8) | ((p[1] & 0xFC) << 3) | (p[2] >> 3));
        }
      } else if (w == static_cast<int>(FRAME_W) && (d.x & 1) == 0 && d.x + w <= 1024) {
        // the usual case: two pixels per word, BGR555 -> RGB565 with the
        // top green bit repeated into the low one
        const auto *src = reinterpret_cast<const uint32_t *>(vrow + d.x);
        auto *o = reinterpret_cast<uint32_t *>(out);
        for (size_t x = 0; x < FRAME_W / 2; x++) {
          const uint32_t v = src[x];
          o[x] = ((v & 0x001F001Fu) << 11) | ((v & 0x03E003E0u) << 1) | ((v & 0x02000200u) >> 4) |
                 ((v >> 10) & 0x001F001Fu);
        }
      } else {
        const uint16_t *row = vrow;
        for (size_t x = 0; x < n; x++) {
          const size_t sx = w > static_cast<int>(FRAME_W) ? x * w / FRAME_W : x;
          const uint16_t v = row[(d.x + sx) & 1023];
          const uint16_t r = v & 0x1F, g = (v >> 5) & 0x1F, b = (v >> 10) & 0x1F;
          out[x_pad + x] = static_cast<uint16_t>((r << 11) | (((g << 1) | (g >> 4)) << 5) | b);
        }
      }
    }
  }
}

void video_task(void *) {
  while (!g_stop.load()) {
    if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) == 0) {
      continue;
    }
    const RtDisplay d = g_display;
    auto *dst = reinterpret_cast<uint16_t *>(g_frames[g_frame_index]);
    if (g_paused.load()) { // converted just before the pause: keep the menu
      g_video_busy = false;
      continue;
    }
    g_last_frame = g_frames[g_frame_index];
    Tab5Emu::get().push_frame(dst);
    g_frame_index ^= 1;
    static int64_t last_us = 0;
    const int64_t now = esp_timer_get_time();
    if (last_us) {
      update_frame_time(static_cast<uint64_t>(now - last_us));
    }
    last_us = now;
    static int64_t last_status_us = 0, last_shot_us = 0;
    static uint32_t presented = 0;
    presented++;
    if (now - last_status_us >= 5000000) {
      static uint32_t last_flips = 0;
      const uint32_t flips = g_flips.load();
      logger.info("vblank {} | presented {} | game {:.1f} fps | {}x{} at ({},{}){}{}", rt_frame_count(), presented,
                  (flips - last_flips) * 1e6f / (now - last_status_us), d.w, d.h, d.x, d.y, d.rgb24 ? " 24-bit" : "",
                  d.enabled ? "" : " (display off)");
      last_flips = flips;
      // the game thread's time over the same window
      static RtProf last_prof{};
      const RtProf p = g_rt_prof;
      const float win = static_cast<float>(now - last_status_us) / 100.0f; // -> percent
      logger.info("  gpu worker (core 1) {:.0f}% busy, band split row {}; game thread: spu {:.0f}% cd {:.0f}% "
                  "sleep {:.0f}%",
                  (p.gpu - last_prof.gpu) / win, g_rt_gpu_split, (p.spu - last_prof.spu) / win,
                  (p.cd - last_prof.cd) / win, (p.sleep - last_prof.sleep) / win);
      last_prof = p;
      static uint64_t last_wait[2];
      logger.info("  rendezvous waits: worker 1 {:.0f}%, worker 0 {:.0f}%",
                  (g_rt_gpu_wait_us[1] - last_wait[1]) / win, (g_rt_gpu_wait_us[0] - last_wait[0]) / win);
      last_wait[0] = g_rt_gpu_wait_us[0];
      last_wait[1] = g_rt_gpu_wait_us[1];
      log_task_load();
      last_status_us = now;
    }
#if CONFIG_TPM_SERIAL_SCREENSHOT_SECONDS > 0
    static bool full_done = CONFIG_TPM_SERIAL_FULL_SHOT_VBLANK == 0;
    if (g_shot && g_shot_task && !full_done &&
        rt_frame_count() + 1 > static_cast<uint32_t>(CONFIG_TPM_SERIAL_FULL_SHOT_VBLANK)) {
      // once, at full resolution
      // once: all of VRAM at half resolution (as RGB565)
      full_done = true;
      for (size_t y = 0; y < 256; y++) {
        for (size_t x = 0; x < 512; x++) {
          const uint16_t v = g_rt_vram[(2 * y) * 1024 + 2 * x];
          const uint16_t r = v & 0x1F, g = (v >> 5) & 0x1F, b = (v >> 10) & 0x1F;
          g_shot[y * 512 + x] = static_cast<uint16_t>((r << 11) | (((g << 1) | (g >> 4)) << 5) | b);
        }
      }
      g_shot_w = 512;
      g_shot_h = 256;
      g_shot_vblank = rt_frame_count();
      xTaskNotifyGive(g_shot_task);
      last_shot_us = now + 30000000; // the console is busy for a while
    } else if (g_shot && g_shot_task && now - last_shot_us >= CONFIG_TPM_SERIAL_SCREENSHOT_SECONDS * 1000000LL) {
      g_shot_w = SHOT_W;
      g_shot_h = SHOT_H;
      for (size_t y = 0; y < SHOT_H; y++) {
        for (size_t x = 0; x < SHOT_W; x++) {
          g_shot[y * SHOT_W + x] = dst[2 * y * FRAME_W + 2 * x];
        }
      }
      g_shot_vblank = rt_frame_count();
      xTaskNotifyGive(g_shot_task);
      last_shot_us = now;
    }
#endif
    (void)last_shot_us;
    g_video_busy = false;
  }
  vTaskDelete(nullptr);
}

void audio_task(void *) {
  constexpr size_t CHUNK = 512;
  static int16_t buf[CHUNK * 2];
  auto &emu = Tab5Emu::get();
  while (!g_stop.load()) {
    const uint32_t r = g_ring_r.load(), w = g_ring_w.load(std::memory_order_acquire);
    const uint32_t avail = w - r;
    if (avail < CHUNK || g_paused.load()) {
      vTaskDelay(pdMS_TO_TICKS(4));
      continue;
    }
    for (size_t i = 0; i < CHUNK; i++) {
      const size_t k = (r + i) % RING_FRAMES;
      buf[2 * i] = g_ring[2 * k];
      buf[2 * i + 1] = g_ring[2 * k + 1];
    }
    g_ring_r.store(r + CHUNK, std::memory_order_release);
    emu.play_audio(reinterpret_cast<const uint8_t *>(buf), sizeof buf);
  }
  vTaskDelete(nullptr);
}

// the PSX GPU's command stream, behind the game's back (rt_gpu_set_async):
// worker 1 on core 1, worker 0 on core 0; each draws a band of rows
void gpu_task(void *arg) {
  const int idx = static_cast<int>(reinterpret_cast<intptr_t>(arg));
  rt_gpu_worker_n(&g_gpu_stop, idx);
  g_gpu_done[idx] = true;
  vTaskSuspend(nullptr); // deinit deletes it (its stack is in PSRAM)
}

#if TPM_GPU_BENCH
// the embedded recording through the rasterizer, on the GPU worker's core
void bench_task(void *arg) {
  const size_t n = _binary_gpubench_bin_end - _binary_gpubench_bin_start;
  auto *rec = static_cast<uint32_t *>(heap_caps_malloc(n, MALLOC_CAP_SPIRAM));
  if (rec) {
    memcpy(rec, _binary_gpubench_bin_start, n);
    static const char *const modes[] = {"all", "no span pixels", "setup only", "no triangles",
                                        "span pixels to internal RAM", "all, per-row dispatch",
                                        "all, no pre-modulated palettes",
                                        "per-row dispatch, texels from internal RAM"};
    for (int mode = 0; mode < 8; mode++) {
      rt_gpu_bench_mode(mode);
      const uint32_t us = rt_gpu_bench(rec, static_cast<uint32_t>(n / 4), 3);
      printf("TPMBENCH %u us per vblank (%s)\n", static_cast<unsigned>(us), modes[mode]);
    }
    rt_gpu_bench_mode(0);
    heap_caps_free(rec);
  }
  xTaskNotifyGive(static_cast<TaskHandle_t>(arg));
  vTaskSuspend(nullptr);
}
#endif

void game_task(void *) {
#if TPM_GPU_BENCH
  TaskHandle_t bench = nullptr;
  xTaskCreatePinnedToCoreWithCaps(bench_task, "tpm_bench", 8192, xTaskGetCurrentTaskHandle(), 4, &bench, 1,
                                  MALLOC_CAP_SPIRAM);
  ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
  vTaskDelay(1);
  vTaskDeleteWithCaps(bench);
#endif
  logger.info("booting SLUS_008.84 ({} bytes)", g_exe.size());
  rt_set_realtime(1);
  rt_gpu_set_async(g_gpu_task ? (g_gpu0_task ? 2 : 1) : 0);
#if CONFIG_TPM_GPU_TRACE_VBLANK > 0
  rt_set_gpu_trace(CONFIG_TPM_GPU_TRACE_VBLANK);
#endif
  rt_boot(g_exe.data(), static_cast<uint32_t>(g_exe.size()));
  rt_shutdown();
  logger.info("game stopped");
  g_game_done = true;
  vTaskSuspend(nullptr);
}

std::string find_disc(const std::string &dir) {
  std::error_code ec;
  std::string best;
  uintmax_t best_size = 0;
  for (const auto &e : std::filesystem::directory_iterator(dir, ec)) {
    auto ext = e.path().extension().string();
    for (auto &c : ext) {
      c = static_cast<char>(tolower(c));
    }
    if (ext != ".bin") {
      continue;
    }
    const auto size = e.file_size(ec);
    if (!ec && size > best_size) {
      best = e.path().string();
      best_size = size;
    }
  }
  return best;
}

} // namespace

bool init(const std::string &data_dir) {
  const auto disc = find_disc(data_dir);
  if (disc.empty()) {
    logger.error("no disc image (.bin, raw 2352-byte sectors) in {}", data_dir);
    return false;
  }
  g_disc = fopen(disc.c_str(), "rb");
  if (!g_disc) {
    logger.error("cannot open {}", disc);
    return false;
  }
  setvbuf(g_disc, nullptr, _IONBF, 0); // the read-ahead buffers instead
  g_ra = static_cast<uint8_t *>(heap_caps_aligned_alloc(128, RA_BYTES, MALLOC_CAP_SPIRAM | MALLOC_CAP_DMA));
  g_ra_len = 0;
  fseek(g_disc, 0, SEEK_END);
  g_disc_sectors = static_cast<uint32_t>(ftell(g_disc) / 2352);
  logger.info("disc {}: {} sectors", disc, g_disc_sectors);

  g_exe.clear();
  if (FILE *f = fopen((data_dir + "/SLUS_008.84").c_str(), "rb")) {
    fseek(f, 0, SEEK_END);
    g_exe.resize(ftell(f));
    fseek(f, 0, SEEK_SET);
    if (fread(g_exe.data(), 1, g_exe.size(), f) != g_exe.size()) {
      g_exe.clear();
    }
    fclose(f);
  }
  if (g_exe.empty() && !exe_from_disc(g_exe)) {
    logger.error("SLUS_008.84 not found (neither in {} nor on the disc)", data_dir);
    deinit();
    return false;
  }

  auto &emu = Tab5Emu::get();
  g_frames[0] = emu.frame_buffer0();
  g_frames[1] = emu.frame_buffer1();
  if (!g_frames[0] || !g_frames[1]) {
    logger.error("no frame buffers");
    deinit();
    return false;
  }
  emu.palette(nullptr, 0);
  emu.native_size(FRAME_W, FRAME_H);
  emu.audio_sample_rate(AUDIO_RATE);
  emu.audio_max_wait_ms(30);

  g_ring = static_cast<int16_t *>(heap_caps_calloc(RING_FRAMES * 2, sizeof(int16_t), MALLOC_CAP_SPIRAM));
  g_resume = xSemaphoreCreateBinary();
  g_stop = false;
  g_paused = false;
  g_game_done = false;
  g_ring_w = g_ring_r = 0;
  reset_statics();
  logger.info("internal free {} B (DMA-capable {} B), PSRAM free {} B (largest {})",
              heap_caps_get_free_size(MALLOC_CAP_INTERNAL), heap_caps_get_free_size(MALLOC_CAP_INTERNAL | MALLOC_CAP_DMA),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM), heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM));
  parse_presses(CONFIG_TPM_AUTOPRESS);

  xTaskCreatePinnedToCore(video_task, "tpm_video", 8192, nullptr, 6, &g_video_task, 1);
#if CONFIG_TPM_SERIAL_SCREENSHOT_SECONDS > 0
  if (!g_shot) {
    g_shot = static_cast<uint16_t *>(heap_caps_malloc(512 * 256 * 2, MALLOC_CAP_SPIRAM));
  }
  // above the GPU worker: printing mostly waits on the console
  xTaskCreatePinnedToCore(shot_task, "tpm_shot", 4096, nullptr, 5, &g_shot_task, 1);
#endif
  // the rasterizer, on the other core (below the video and audio tasks there:
  // they are short and must not wait behind a frame's drawing)
  g_gpu_stop = 0;
  g_gpu_done[0] = false;
  g_gpu_done[1] = false;
  // stacks in PSRAM: on the P4 the cores reach internal RAM more slowly than
  // cached PSRAM, and these stacks are hot (the recompiled code spills guest
  // registers to its stack all the time)
  if (xTaskCreatePinnedToCoreWithCaps(gpu_task, "tpm_gpu", 8192, reinterpret_cast<void *>(1), 4, &g_gpu_task, 1,
                                      MALLOC_CAP_SPIRAM) != pdPASS) {
    g_gpu_task = nullptr;
    logger.warn("no GPU worker: drawing on the game's thread");
  } else if (CONFIG_TPM_GPU_WORKERS > 1 &&
             xTaskCreatePinnedToCoreWithCaps(gpu_task, "tpm_gpu0", 8192, reinterpret_cast<void *>(0), 4, &g_gpu0_task,
                                             0, MALLOC_CAP_SPIRAM) != pdPASS) {
    // the second worker sits above the game (5) on its core: the drawing is
    // the bottleneck, and at the rendezvous (VRAM moves, the vblank) worker 1
    // waits for it; it sleeps whenever it has nothing to draw
    g_gpu0_task = nullptr;
    logger.warn("no second GPU worker: core 1 draws everything");
  }
#if CONFIG_TPM_PROFILE_VBLANK > 0
  xTaskCreatePinnedToCore(prof_task, "tpm_prof", 4096, nullptr, 10, nullptr, 0);
  xTaskCreatePinnedToCore(prof_task, "tpm_profg", 4096, reinterpret_cast<void *>(1), 10, nullptr, 1);
#endif
  xTaskCreatePinnedToCore(audio_task, "tpm_audio", 4096, nullptr, 7, &g_audio_task, 1);
#if CONFIG_TPM_TEST_PAUSE_VBLANK > 0
  xTaskCreatePinnedToCore(pause_test_task, "tpm_ptest", 4096, nullptr, 5, nullptr, 0);
#endif
  // The recompiled code nests a C frame per guest call (<= 304 bytes on
  // RV32, guest chains seen up to 25 deep including interrupts): 32 KB, in
  // internal RAM since those frames are the guest's registers; PSRAM if the
  // internal heap cannot spare it.
  constexpr uint32_t STACK = 32 * 1024;
  if (xTaskCreatePinnedToCoreWithCaps(game_task, "tpm_game", STACK, nullptr, 5, &g_game_task, 0,
                                      MALLOC_CAP_SPIRAM) != pdPASS) {
    logger.error("could not create the game task");
    deinit();
    return false;
  }
  return true;
}

void deinit() {
  if (g_game_task) {
    rt_request_exit();
    rt_pause(0);
    if (g_resume) {
      xSemaphoreGive(g_resume);
    }
    for (int i = 0; i < 200 && !g_game_done.load(); i++) {
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    vTaskDeleteWithCaps(g_game_task);
    g_game_task = nullptr;
  }
  if (g_gpu_task || g_gpu0_task) {
    g_gpu_stop = 1;
    for (int i = 0; i < 100 && ((g_gpu_task && !g_gpu_done[1]) || (g_gpu0_task && !g_gpu_done[0])); i++) {
      if (g_gpu_task) xTaskNotifyGive(g_gpu_task);
      if (g_gpu0_task) xTaskNotifyGive(g_gpu0_task);
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    if (g_gpu_task) vTaskDeleteWithCaps(g_gpu_task);
    if (g_gpu0_task) vTaskDeleteWithCaps(g_gpu0_task);
    g_gpu_task = g_gpu0_task = nullptr;
  }
  g_stop = true;
  if (g_video_task) {
    xTaskNotifyGive(g_video_task);
  }
  vTaskDelay(pdMS_TO_TICKS(150)); // the helpers notice g_stop and delete themselves
  g_video_task = g_audio_task = nullptr;
  if (g_disc) {
    fclose(g_disc);
    g_disc = nullptr;
  }
  heap_caps_free(g_ra);
  g_ra = nullptr;
  g_ra_len = 0;
  heap_caps_free(g_ring);
  g_ring = nullptr;
  if (g_resume) {
    vSemaphoreDelete(g_resume);
    g_resume = nullptr;
  }
  g_last_frame = nullptr;
  // the runtime's buffers (rt_shutdown dropped its pointers if the game got
  // to return; the next launch resets the statics either way)
  shared_mem_clear();
  g_exe.clear();
  g_exe.shrink_to_fit();
}

bool running() { return g_game_task && !g_game_done.load(); }

void pause() {
  g_paused = true;
  rt_pause(1);
}

void resume() {
  Tab5Emu::get().native_size(FRAME_W, FRAME_H);
  g_paused = false;
  rt_pause(0);
  if (g_resume) {
    xSemaphoreGive(g_resume);
  }
}

std::span<uint8_t> last_frame() {
  auto *f = g_last_frame.load();
  return f ? std::span<uint8_t>{f, FRAME_W * FRAME_H * 2} : std::span<uint8_t>{};
}

} // namespace tpm::engine

// ---- what the runtime needs from the board (rt_platform.h) ----

extern "C" {

int plat_read_sector(uint32_t lba, uint8_t *out) { return tpm::engine::read_sector(lba, out) ? 0 : -1; }
uint32_t plat_disc_sectors(void) { return tpm::engine::g_disc_sectors; }

void plat_vblank(void) {
  using namespace tpm::engine;
  static int last_x = -1, last_y = -1;
  RtDisplay d;
  rt_gpu_display(&d);
  if (d.x != last_x || d.y != last_y) {
    g_flips++;
    last_x = d.x;
    last_y = d.y;
  }
}

// On the GPU worker (core 1), after the drawing queued before the vblank.
void plat_display_ready(const RtDisplay *dp) {
  using namespace tpm::engine;
  // present when the game shows a new frame (the display moved to the other
  // buffer), at most every other vblank (it runs at 30 or less), and every
  // fourth anyway for screens drawn in place; a conversion is a tenth of the
  // GPU worker's time, so not repeating the same frame matters. Never queue
  // behind a conversion that is still running, and nothing while the
  // emulator's pause menu is up (the GPU worker still works through what the
  // game queued before it stopped, and its frames would cover the menu).
  static uint32_t since;
  static int shown_x = -1, shown_y = -1;
  since++;
  const bool flipped = dp->x != shown_x || dp->y != shown_y;
  if (!((flipped && since >= 2) || since >= 4) || g_paused.load() || g_video_busy.load() || !g_video_task ||
      !g_rt_vram) {
    return;
  }
  since = 0;
  shown_x = dp->x;
  shown_y = dp->y;
  g_display = *dp;
  convert_frame(g_display, reinterpret_cast<uint16_t *>(g_frames[g_frame_index]));
  g_video_busy = true;
  xTaskNotifyGive(g_video_task);
}

void plat_gpu_kick(void) {
  if (tpm::engine::g_gpu_task) {
    xTaskNotifyGive(tpm::engine::g_gpu_task);
  }
  if (tpm::engine::g_gpu0_task) {
    xTaskNotifyGive(tpm::engine::g_gpu0_task);
  }
}

// A worker sleeps until kicked; the game's thread, waiting for room in the
// queue, sleeps until a worker made some (it must not spin: the second
// worker shares its core)
void plat_gpu_idle(void) { ulTaskNotifyTake(pdTRUE, 1); }

void plat_gpu_wake_producer(void) {
  if (tpm::engine::g_game_task) {
    xTaskNotifyGive(tpm::engine::g_game_task);
  }
}

// The HAL's merged gamepad (touch zones, USB HID pad, keyboard) as a PSX
// digital pad. Face buttons by position, as in the MGS port: the HAL
// normalises a pad's face buttons to A = east, B = south, X = north,
// Y = west, so B is Cross, A Circle, Y Square and X Triangle. A USB pad's
// trigger is folded into its shoulder bit and also reported alone (l2 / r2),
// so L1 is "l without l2". The left stick drives the d-pad.
uint16_t plat_pad_buttons(int port) {
  if (port != 0) {
    return 0;
  }
  auto &emu = Tab5Emu::get();
  const auto s = emu.gamepad_state();
  bool up = s.up, down = s.down, left = s.left, right = s.right;
  if (emu.usb_gamepad_present()) {
    constexpr int DEAD = 32767 * 4 / 10;
    const auto ax = emu.gamepad_axes();
    left |= ax.lx < -DEAD;
    right |= ax.lx > DEAD;
    up |= ax.ly < -DEAD;
    down |= ax.ly > DEAD;
  }
  uint16_t b = 0;
  if (s.select) b |= 1u << 0;
  if (s.start) b |= 1u << 3;
  if (up) b |= 1u << 4;
  if (right) b |= 1u << 5;
  if (down) b |= 1u << 6;
  if (left) b |= 1u << 7;
  if (s.l2) b |= 1u << 8;            // L2
  if (s.r2) b |= 1u << 9;            // R2
  if (s.l && !s.l2) b |= 1u << 10;   // L1
  if (s.r && !s.r2) b |= 1u << 11;   // R1
  if (s.x) b |= 1u << 12;            // triangle (north)
  if (s.a) b |= 1u << 13;            // circle (east)
  if (s.b) b |= 1u << 14;            // cross (south)
  if (s.y) b |= 1u << 15;            // square (west)
  return b | tpm::engine::scripted_buttons(rt_frame_count());
}

void plat_audio(const int16_t *samples, int frames) {
  using namespace tpm::engine;
  if (!g_ring) {
    return;
  }
  uint32_t w = g_ring_w.load();
  const uint32_t r = g_ring_r.load(std::memory_order_acquire);
  for (int i = 0; i < frames; i++) {
    if (w - r >= RING_FRAMES) {
      break; // the audio task is behind: drop
    }
    const size_t k = w % RING_FRAMES;
    g_ring[2 * k] = samples[2 * i];
    g_ring[2 * k + 1] = samples[2 * i + 1];
    w++;
  }
  g_ring_w.store(w, std::memory_order_release);
}

uint64_t plat_time_us(void) { return static_cast<uint64_t>(esp_timer_get_time()); }

void *plat_alloc(uint32_t size) {
  shared_mem_request_t req{.size = size, .region = SHARED_MEM_CACHE_LINE, .storage = SHARED_MEM_PSRAM};
  void *p = shared_mem_allocate(&req); // zeroed
  if (!p) {
    tpm::engine::logger.error("no PSRAM for {} bytes", size);
  }
  return p;
}

void plat_free(void *) {} // the pool is cleared when the cart quits

void plat_sleep_us(uint32_t us) {
  // the game is ahead of real time: give the core away for a tick or more
  vTaskDelay(std::max<TickType_t>(1, pdMS_TO_TICKS(us / 1000)));
}

void plat_pause_wait(void) {
  if (tpm::engine::g_resume) {
    xSemaphoreTake(tpm::engine::g_resume, pdMS_TO_TICKS(100));
  } else {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

} // extern "C"

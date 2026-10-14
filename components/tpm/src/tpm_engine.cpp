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
#include <csetjmp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <string>
#include <vector>

#include <esp_app_desc.h>
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
extern uint8_t *g_ram; // guest RAM (the save state test hashes it)
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

// ---- the arena ----
//
// The runtime's big buffers (guest RAM, VRAM, transfers, the GPU queue, the
// SPU) and the game thread's stack, in one PSRAM block, as the MGS port
// does: a save state is full of absolute pointers into them (the recompiled
// code keeps host pointers in its stack frames), so it loads only while they
// sit at the same addresses. The block is kept across the cart's quits for
// that reason; the launcher releases it before it starts any other cart.
// Launched first after a boot, the game normally gets the same block back.
//
// The stack is the block's first STACK bytes. Its lowest ALT bytes above the
// canary are where the game thread waits while a state is put back over the
// rest (the P4's stack guard faults on a stack pointer outside the task's
// stack, so the spare stack is inside it); a save point is far above them.
constexpr uint32_t STACK = 32 * 1024;
constexpr uint32_t ALT_LO = 1024, ALT_HI = 6 * 1024;
constexpr size_t ARENA_ALIGN = 64;
uint8_t *g_arena{nullptr};
size_t g_arena_size{0}, g_arena_used{0};
bool g_arena_overflow{false}; // some allocation came from the heap: no states
StaticTask_t g_game_tcb;

size_t arena_bytes() {
  int count = 0;
  const uint32_t bytes = rt_mem_bytes(&count);
  return STACK + bytes + static_cast<size_t>(count) * ARENA_ALIGN;
}

bool acquire_arena() {
  const size_t total = arena_bytes();
  if (g_arena && g_arena_size != total) {
    heap_caps_free(g_arena);
    g_arena = nullptr;
  }
  if (!g_arena) {
    g_arena = static_cast<uint8_t *>(heap_caps_aligned_alloc(ARENA_ALIGN, total, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (!g_arena) {
      logger.error("no {} KB of PSRAM for the game (free {} KB, largest {} KB)", total / 1024,
                   heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024,
                   heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024);
      return false;
    }
    g_arena_size = total;
    logger.info("arena: {} KB at {}", total / 1024, fmt::ptr(g_arena));
  }
  memset(g_arena, 0, g_arena_size);
  g_arena_used = STACK;
  g_arena_overflow = false;
  return true;
}

// ---- save states ----
//
// Taken and put back by the menu's task while the game's thread waits in
// plat_pause_wait, where it checkpoints itself (setjmp) for a save, or for a
// load moves to the spare stack and waits there until the memory is back,
// then jumps to the state's checkpoint. A state is: the runtime's statics
// (.data, .bss), the arena's buffers and the live part of the game's stack.
enum SnapReq : int { SNAP_NONE, SNAP_SAVE, SNAP_LOAD };
std::atomic<int> g_snap_req{SNAP_NONE};
std::atomic<bool> g_snap_ack{false};  // the game's thread: checkpoint taken / parked
std::atomic<bool> g_snap_done{false}; // the menu's task: go on
jmp_buf g_snap_jb;
uint8_t *g_snap_sp{nullptr};

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

// Save states (CONFIG_TPM_TEST_STATE "A:B"): at vblank A a state is saved,
// at B loaded. From the save on, both passes log a hash of guest RAM at each
// of 120 vblanks: the game is deterministic in emulated time, so the run
// after the load must repeat the first pass's hashes.
std::atomic<uint32_t> g_hash_from{0};
constexpr uint32_t HASH_VBLANKS = 120;
#define TPM_STATE_TEST (sizeof(CONFIG_TPM_TEST_STATE) > 1)
void state_test_task(void *) {
  const uint32_t a = static_cast<uint32_t>(strtoul(CONFIG_TPM_TEST_STATE, nullptr, 10));
  const char *colon = strchr(CONFIG_TPM_TEST_STATE, ':');
  const uint32_t b = colon ? static_cast<uint32_t>(strtoul(colon + 1, nullptr, 10)) : 0;
  const std::string path = "/sdcard/tpm_state_test.sav";
  const uint32_t marks[2] = {a, b};
  // Only the load when the card already has a state from this build (an
  // earlier boot's, after a reset): the hashes then start from the loaded
  // vblank, to compare with that boot's
  bool have = false;
  if (FILE *f = fopen(path.c_str(), "rb")) {
    uint8_t head[40];
    have = fread(head, 1, sizeof head, f) == sizeof head && memcmp(head, "TPMS", 4) == 0 &&
           memcmp(head + 8, esp_app_get_description()->app_elf_sha256, 32) == 0;
    fclose(f);
  }
  if (have) {
    logger.warn("state test: a state of this build is on the card: loading it at vblank {}", b);
  }
  for (int step = have ? 1 : 0; step < 2 && !g_stop.load(); step++) {
    while (!g_stop.load() && rt_frame_count() < marks[step]) {
      vTaskDelay(pdMS_TO_TICKS(20));
    }
    pause();
    vTaskDelay(pdMS_TO_TICKS(300));
    const uint32_t at = rt_frame_count();
    const bool ok = step == 0 ? save_state(path) : load_state(path);
    logger.warn("state test: {} at vblank {}: {} (vblank now {})", step == 0 ? "save" : "load", at,
                ok ? "ok" : "FAILED", rt_frame_count());
    if (step == 0 || (have && ok)) {
      g_hash_from = (step == 0 ? at : rt_frame_count()) + 1;
    }
    vTaskDelay(pdMS_TO_TICKS(300));
    resume();
  }
  vTaskDelete(nullptr);
}

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
      static uint32_t last_skipped = 0;
      const uint32_t skipped = g_rt_gpu_skipped;
      logger.info("vblank {} | presented {} | game {:.1f} fps, drawn {:.1f} | {}x{} at ({},{}){}{}", rt_frame_count(),
                  presented, (flips - last_flips) * 1e6f / (now - last_status_us),
                  (static_cast<int>(flips - last_flips) - static_cast<int>(skipped - last_skipped)) * 1e6f /
                      (now - last_status_us),
                  d.w, d.h, d.x, d.y, d.rgb24 ? " 24-bit" : "", d.enabled ? "" : " (display off)");
      last_flips = flips;
      last_skipped = skipped;
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
  for (;;) {
    rt_gpu_worker_n(&g_gpu_stop, idx);
    g_gpu_done[idx] = true;
    // stopped for a save state (started again after it), or for good:
    // deinit deletes the task (its stack is in PSRAM)
    while (g_gpu_stop) {
      ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
    }
    g_gpu_done[idx] = false;
  }
}

// the workers out of the GPU's state (each finishes what it took from the
// queue; what is left stays queued, which is part of the state), and back
bool gpu_stop_workers() {
  g_gpu_stop = 1;
  for (int i = 0; i < 300; i++) {
    if ((!g_gpu_task || g_gpu_done[1]) && (!g_gpu0_task || g_gpu_done[0])) {
      return true;
    }
    if (g_gpu_task) xTaskNotifyGive(g_gpu_task);
    if (g_gpu0_task) xTaskNotifyGive(g_gpu0_task);
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  logger.error("the GPU workers did not stop");
  return false;
}

void gpu_start_workers() {
  g_gpu_stop = 0;
  for (int i = 0; i < 300; i++) {
    if ((!g_gpu_task || !g_gpu_done[1]) && (!g_gpu0_task || !g_gpu_done[0])) {
      return;
    }
    if (g_gpu_task) xTaskNotifyGive(g_gpu_task);
    if (g_gpu0_task) xTaskNotifyGive(g_gpu0_task);
    vTaskDelay(pdMS_TO_TICKS(1));
  }
  logger.error("the GPU workers did not start again");
}

#if TPM_GPU_BENCH
// the embedded recording through the rasterizer, on the GPU worker's core
void bench_task(void *arg) {
  const size_t n = _binary_gpubench_bin_end - _binary_gpubench_bin_start;
  // read in place when aligned (the rodata is in PSRAM too): a copy is 3 MB
  // the cart's own buffers may need
  const bool in_place = (reinterpret_cast<uintptr_t>(_binary_gpubench_bin_start) & 3) == 0;
  auto *rec = in_place ? reinterpret_cast<uint32_t *>(const_cast<uint8_t *>(_binary_gpubench_bin_start))
                       : static_cast<uint32_t *>(heap_caps_malloc(n, MALLOC_CAP_SPIRAM));
  printf("TPMBENCH recording %u bytes, %s; PSRAM free %u KB\n", static_cast<unsigned>(n), in_place ? "in place" : "copied",
         static_cast<unsigned>(heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024));
  if (rec) {
    if (!in_place) {
      memcpy(rec, _binary_gpubench_bin_start, n);
    }
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
    if (!in_place) {
      heap_caps_free(rec);
    }
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
  logger.info("booting SLUS_008.84 ({} bytes); PSRAM free {} KB, largest block {} KB", g_exe.size(),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM) / 1024, heap_caps_get_largest_free_block(MALLOC_CAP_SPIRAM) / 1024);
  rt_set_realtime(CONFIG_TPM_BENCH_FROM > 0 ? 0 : 1);
  rt_gpu_set_async(g_gpu_task ? (g_gpu0_task ? 2 : 1) : 0);
  rt_gpu_set_frameskip(CONFIG_TPM_FRAMESKIP);
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

  if (!acquire_arena()) {
    deinit();
    return false;
  }
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
  g_hash_from = 0;
  if (TPM_STATE_TEST) {
    xTaskCreatePinnedToCore(state_test_task, "tpm_stest", 8192, nullptr, 5, nullptr, 0);
  }
  // The recompiled code nests a C frame per guest call (<= 304 bytes on
  // RV32, guest chains seen up to 25 deep including interrupts): 32 KB, the
  // arena's first part (PSRAM: on the P4 the cores reach cached PSRAM faster
  // than internal RAM, and save states need it at a fixed address)
  g_game_task = xTaskCreateStaticPinnedToCore(game_task, "tpm_game", STACK, nullptr, 5,
                                              reinterpret_cast<StackType_t *>(g_arena), &g_game_tcb, 0);
  if (!g_game_task) {
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
    vTaskDelete(g_game_task); // static: its stack is the arena's
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
  g_snap_req = SNAP_NONE;
  // the runtime's buffers are the arena's, kept for the next launch (see
  // release_arena); the pool holds only what did not fit in it
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

void release_arena() {
  if (g_game_task) {
    return; // the game is running in it
  }
  heap_caps_free(g_arena);
  g_arena = nullptr;
  g_arena_size = g_arena_used = 0;
}

namespace {

constexpr uint32_t kSnapVersion = 1;
struct SnapHeader {
  char magic[4];
  uint32_t version;
  uint8_t elf_sha[32]; // the build that wrote it: return addresses in the stack
  uint32_t arena, arena_size, arena_used;
  uint32_t bss, bss_size, data, data_size;
  uint32_t stack_lo; // the saved part of the stack: [stack_lo, arena + STACK)
  uint32_t jb_size;
  uint8_t jb[sizeof(jmp_buf)];
};

struct SnapRegion {
  uint8_t *p;
  size_t n;
};

// in the order of the file, after the header
std::vector<SnapRegion> snap_regions(uint8_t *stack_lo) {
  return {
      {_tpm_data_start, static_cast<size_t>(_tpm_data_end - _tpm_data_start)},
      {_tpm_bss_start, static_cast<size_t>(_tpm_bss_end - _tpm_bss_start)},
      {g_arena + STACK, g_arena_used - STACK},
      {stack_lo, static_cast<size_t>(g_arena + STACK - stack_lo)},
  };
}

bool io_chunked(FILE *f, uint8_t *p, size_t n, bool writing) {
  while (n > 0) {
    const size_t chunk = std::min<size_t>(n, 64 * 1024);
    if ((writing ? fwrite(p, 1, chunk, f) : fread(p, 1, chunk, f)) != chunk) {
      return false;
    }
    p += chunk;
    n -= chunk;
  }
  return true;
}

// ask the game's thread (waiting in plat_pause_wait) for a checkpoint or to
// park; true once it has
bool snap_request(SnapReq req) {
  g_snap_ack = false;
  g_snap_done = false;
  g_snap_req = req;
  for (int i = 0; i < 300; i++) {
    xSemaphoreGive(g_resume);
    if (g_snap_ack.load()) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  g_snap_req = SNAP_NONE;
  logger.error("{} state: the game's thread did not answer", req == SNAP_SAVE ? "save" : "load");
  return false;
}

void snap_release() {
  g_snap_done = true;
  xSemaphoreGive(g_resume);
  xTaskNotifyGive(g_game_task);
}

} // namespace

bool save_state(const std::string &path) {
  if (!g_game_task || !g_paused.load() || g_game_done.load()) {
    logger.error("save state: the game must be running and paused");
    return false;
  }
  if (g_arena_overflow) {
    logger.error("save state: part of the game's memory is outside its arena");
    return false;
  }
  const int64_t t0 = esp_timer_get_time();
  if (!snap_request(SNAP_SAVE)) {
    return false;
  }
  // the game's thread is at its checkpoint, waiting below it
  uint8_t *const stack_lo = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(g_snap_sp - 256) & ~uintptr_t{15});
  if (stack_lo < g_arena + ALT_HI) {
    logger.error("save state: the game's stack is too deep here ({} bytes)", g_arena + STACK - g_snap_sp);
    snap_release();
    return false;
  }
  SnapHeader h{};
  memcpy(h.magic, "TPMS", 4);
  h.version = kSnapVersion;
  memcpy(h.elf_sha, esp_app_get_description()->app_elf_sha256, sizeof h.elf_sha);
  h.arena = reinterpret_cast<uint32_t>(g_arena);
  h.arena_size = g_arena_size;
  h.arena_used = g_arena_used;
  h.bss = reinterpret_cast<uint32_t>(_tpm_bss_start);
  h.bss_size = _tpm_bss_end - _tpm_bss_start;
  h.data = reinterpret_cast<uint32_t>(_tpm_data_start);
  h.data_size = _tpm_data_end - _tpm_data_start;
  h.stack_lo = reinterpret_cast<uint32_t>(stack_lo);
  h.jb_size = sizeof(jmp_buf);
  memcpy(h.jb, &g_snap_jb, sizeof(jmp_buf));
  bool ok = gpu_stop_workers();
  size_t total = sizeof h;
  if (ok) {
    FILE *f = fopen(path.c_str(), "wb");
    ok = f && fwrite(&h, 1, sizeof h, f) == sizeof h;
    for (const auto &r : snap_regions(stack_lo)) {
      ok = ok && io_chunked(f, r.p, r.n, true);
      total += r.n;
    }
    if (f) {
      ok = fclose(f) == 0 && ok;
    }
  }
  gpu_start_workers();
  snap_release();
  logger.info("save state: {} {} ({} KB, stack {} B, in {} ms)", ok ? "wrote" : "FAILED to write", path,
              total / 1024, g_arena + STACK - stack_lo, (esp_timer_get_time() - t0) / 1000);
  return ok;
}

bool load_state(const std::string &path) {
  if (!g_game_task || !g_paused.load() || g_game_done.load()) {
    logger.error("load state: the game must be running and paused");
    return false;
  }
  const int64_t t0 = esp_timer_get_time();
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    logger.error("load state: cannot open {}", path);
    return false;
  }
  SnapHeader h{};
  const auto refuse = [&](const char *why) {
    logger.error("load state: {}: {}", path, why);
    fclose(f);
    return false;
  };
  if (fread(&h, 1, sizeof h, f) != sizeof h || memcmp(h.magic, "TPMS", 4) != 0 || h.version != kSnapVersion ||
      h.jb_size != sizeof(jmp_buf)) {
    return refuse("not a save state of this format");
  }
  if (memcmp(h.elf_sha, esp_app_get_description()->app_elf_sha256, sizeof h.elf_sha) != 0) {
    return refuse("written by another firmware build; states load only into the build that wrote them");
  }
  if (h.arena != reinterpret_cast<uint32_t>(g_arena) || h.arena_size != g_arena_size ||
      h.arena_used != g_arena_used) {
    return refuse("the game's memory sits elsewhere in this session (another game ran first?): "
                  "launch the game first after a restart to load it");
  }
  auto *const stack_lo = reinterpret_cast<uint8_t *>(h.stack_lo);
  if (h.bss != reinterpret_cast<uint32_t>(_tpm_bss_start) || h.data != reinterpret_cast<uint32_t>(_tpm_data_start) ||
      stack_lo < g_arena + ALT_HI || stack_lo >= g_arena + STACK) {
    return refuse("layout mismatch");
  }
  const auto regions = snap_regions(stack_lo);
  {
    // all of it must be there before the first byte of the game is overwritten
    size_t expected = sizeof h;
    for (const auto &r : regions) {
      expected += r.n;
    }
    fseek(f, 0, SEEK_END);
    const long actual = ftell(f);
    fseek(f, sizeof h, SEEK_SET);
    if (actual < 0 || static_cast<size_t>(actual) != expected) {
      return refuse("truncated or corrupt");
    }
  }
  if (!gpu_stop_workers()) {
    gpu_start_workers();
    fclose(f);
    return false;
  }
  if (!snap_request(SNAP_LOAD)) {
    gpu_start_workers();
    fclose(f);
    return false;
  }
  // the game's thread waits on the spare stack: everything else may go
  void **slots[16];
  void *kept[16];
  const int n_slots = rt_snap_preserve(slots, 16);
  for (int i = 0; i < n_slots; i++) {
    kept[i] = *slots[i];
  }
  bool ok = true;
  for (const auto &r : regions) {
    ok = ok && io_chunked(f, r.p, r.n, false);
  }
  fclose(f);
  for (int i = 0; i < n_slots; i++) {
    *slots[i] = kept[i];
  }
  if (!ok) {
    // half restored: the game cannot go on; the cart quits
    logger.error("load state: short read from {}: the game's memory is inconsistent, stopping it", path);
    g_game_done = true;
    return false;
  }
  rt_snap_restored();
  memcpy(&g_snap_jb, h.jb, sizeof(jmp_buf));
  gpu_start_workers();
  snap_release(); // the game's thread jumps into the state's checkpoint
  logger.info("load state: {} restored in {} ms", path, (esp_timer_get_time() - t0) / 1000);
  return true;
}

} // namespace tpm::engine

// ---- what the runtime needs from the board (rt_platform.h) ----

extern "C" {

int plat_read_sector(uint32_t lba, uint8_t *out) { return tpm::engine::read_sector(lba, out) ? 0 : -1; }
uint32_t plat_disc_sectors(void) { return tpm::engine::g_disc_sectors; }

void plat_vblank(void) {
  using namespace tpm::engine;
#if CONFIG_TPM_BENCH_FROM > 0
  {
    // (the game's thread time is the total less its waits for the GPU)
    static int64_t t0;
    static uint64_t w0;
    const uint32_t n = rt_frame_count();
    if (n == CONFIG_TPM_BENCH_FROM) {
      t0 = esp_timer_get_time();
      w0 = g_rt_gpu_prod_wait_us;
    } else if (n == CONFIG_TPM_BENCH_TO) {
      const int64_t all = esp_timer_get_time() - t0;
      const int64_t wait = static_cast<int64_t>(g_rt_gpu_prod_wait_us - w0);
      logger.info("bench: vblanks {}..{} in {} ms, {} ms of it waiting for the GPU: game {} ms",
                  CONFIG_TPM_BENCH_FROM, CONFIG_TPM_BENCH_TO, all / 1000, wait / 1000, (all - wait) / 1000);
    }
  }
#endif
  if (TPM_STATE_TEST) {
    const uint32_t from = g_hash_from.load(), n = rt_frame_count();
    if (from && n >= from && n < from + HASH_VBLANKS && g_ram) {
      const auto *w = reinterpret_cast<const uint32_t *>(g_ram);
      uint32_t h = 2166136261u;
      for (size_t i = 0; i < 2 * 1024 * 1024 / 4; i++) {
        h = (h ^ w[i]) * 16777619u;
      }
      printf("TPMHASH %u %08x\n", static_cast<unsigned>(n), static_cast<unsigned>(h));
    }
  }
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
  if (dp->stale) {
    // frameskip: this buffer's frame was not drawn; keep the last picture,
    // and the next drawn frame counts as new wherever it is
    shown_x = shown_y = -1;
    return;
  }
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
  using namespace tpm::engine;
  // the arena, in rt_boot's fixed order (rt_mem_bytes sized it)
  const size_t off = (g_arena_used + ARENA_ALIGN - 1) & ~(ARENA_ALIGN - 1);
  if (g_arena && off + size <= g_arena_size) {
    g_arena_used = off + size;
    memset(g_arena + off, 0, size);
    return g_arena + off;
  }
  g_arena_overflow = true;
  tpm::engine::logger.warn("{} bytes outside the arena: no save states this run", size);
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

// the game's thread, on the spare stack while a state is read over its own;
// then into the state's checkpoint
[[noreturn]] void tpm_snap_parked(void) {
  using namespace tpm::engine;
  g_snap_ack = true;
  while (!g_snap_done.load()) {
    ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(10));
  }
  g_snap_req = SNAP_NONE;
  longjmp(g_snap_jb, 1);
}

// after the checkpoint: below it, until the state is written
static void __attribute__((noinline)) snap_saved_wait(void) {
  using namespace tpm::engine;
  g_snap_req = SNAP_NONE;
  g_snap_ack = true;
  while (!g_snap_done.load()) {
    xSemaphoreTake(g_resume, pdMS_TO_TICKS(10));
  }
}

void plat_pause_wait(void) {
  using namespace tpm::engine;
  const int req = g_snap_req.load();
  if (req == SNAP_SAVE) {
    // a load of this state comes back here (setjmp returns 1) and returns to
    // the runtime's pause loop, as the saving run does
    if (setjmp(g_snap_jb) == 0) {
      uint8_t *sp;
      asm volatile("mv %0, sp" : "=r"(sp));
      g_snap_sp = sp;
      snap_saved_wait();
    }
    return;
  }
  if (req == SNAP_LOAD) {
    // to the spare stack: the state is read over this one
    uint8_t *const top = g_arena + ALT_HI;
    asm volatile("mv sp, %0\n\tjr %1" : : "r"(top), "r"(&tpm_snap_parked) : "memory");
    __builtin_unreachable();
  }
  if (g_resume) {
    xSemaphoreTake(g_resume, pdMS_TO_TICKS(100));
  } else {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

} // extern "C"

#include "mgs.hpp"

#include "sdkconfig.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "esp_app_desc.h"
#include "esp_attr.h"
#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
#include <vector>

#include "../mgs_reversing/port/mgs_snapshot.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "logger.hpp"
#include "tab5-emu.hpp"

#include "mgs_platform.hpp"

// The decompiled game is native code with its own cooperative scheduler (mts)
// mapped onto FreeRTOS tasks by port/esp32_threads.c, so there is no "run one
// frame" entry point: init() starts the game in a task and the cart only polls
// for the menu. The game assumes it owns the machine (the PSX booted from
// scratch every time), so stopping it is the hard part: every static of
// libmgs.a is bracketed by linker symbols (linker.lf), .bss/COMMON are zeroed
// and .data restored from a snapshot taken before the first launch, so a
// relaunch sees exactly what the first launch did. This file's own statics
// are kept out of that range (linker.lf: mgs (default)).

namespace {
espp::Logger logger({.tag = "mgs", .level = espp::Logger::Verbosity::INFO});
mgs::Config g_config;
bool g_initialized{false};
bool g_paused{false};
std::atomic<bool> g_main_returned{false};
TaskHandle_t g_main_task{nullptr};
// The main game task's stack at a fixed address, so a save state can hold
// its context (the mts "system" thread parks on this stack).
constexpr size_t kMainStackBytes = 16 * 1024;
EXT_RAM_BSS_ATTR StackType_t g_main_stack[kMainStackBytes / sizeof(StackType_t)];
StaticTask_t g_main_tcb;
// addresses inside the game regions that belong to this run, not to a state
std::vector<std::pair<void *, size_t>> g_preserve;
int g_report_after_resume{0};
uint8_t *g_data_snapshot{nullptr}; // libmgs.a's .data as linked; survives relaunches
} // namespace

#if defined(CONFIG_MGS_ENGINE)
extern "C" {
int mgs_main(void);                  // source/main/main.c, renamed by the build
void Mgs_SetDataRoot(const char *p); // port/platform_headless.c
void Mgs_StartVblank(void);          // port/esp32_vblank.c: vblank tick + scanout tasks
void Mgs_PauseVblank(void);
void Mgs_ResumeVblank(void);
void Mgs_StopVblank(void);
void Psyz_GpuSync(void);              // psyz libgpu.c: wait for the raster worker
extern unsigned short g_RawVram[];    // psyz soft raster: 1024x512 RGB1555
extern unsigned char mgs_main_ram[];  // port/psyz_port.c: the PSX's 2 MB, as laid out
void Mgs_ThreadsPause(void); // port/esp32_threads.c: the live PSX thread
void Mgs_ThreadsResume(void);
void Mgs_ThreadsStopAll(void);
void Mgs_CdInit(void); // port/virtual_cd.c: open / close the disc files
void Mgs_CdDeinit(void);
void Draw_Reset(void); // port/soft_render.c: rebind the rasterizer to VRAM
int lcd_init(void);    // mgs_platform.cpp: the HAL frame buffers
extern int mts_active_task_800C0DB0;   // source/mts/mts_new.c
extern volatile int psyz_critical_depth; // psyz libapi.c
extern unsigned mgs_frame_seq;         // port/esp32_vblank.c
extern unsigned mgs_vblank_count;
extern volatile const char *mgs_tick_phase;
extern volatile int mgs_in_printf;     // port/psyz_port.c
int Mgs_CurrentThread(void);
void Mgs_ThreadsReport(const char *when);
void Mgs_HeapCheck(const char *where);
extern unsigned mts_ready_tasks_800C0DB4;
extern unsigned char padbuf_800C1480[];
extern int GV_PauseLevel;
extern unsigned GM_GameStatus;
int Mgs_ThreadSample(int i, unsigned *pc, unsigned *ra, unsigned *sp);
int Mgs_ThreadStackScan(int i, unsigned *out, int max);
extern const char *mgs_where;
// linker.lf SURROUND symbols for libmgs.a's statics
extern char _mgs_bss_start[], _mgs_bss_end[];
extern char _mgs_common_start[], _mgs_common_end[];
extern char _mgs_xbss_start[], _mgs_xbss_end[];
extern char _mgs_data_start[], _mgs_data_end[];
extern char _mgs_sbss_start[], _mgs_sbss_end[];
extern char _mgs_sdata_start[], _mgs_sdata_end[];
}

namespace {
constexpr int MTS_TASK_IDLE = 11;

// Put every static of libmgs.a back to its as-linked value. Only valid while
// none of the game's tasks exist.
void reset_statics() {
  const size_t data_size = _mgs_data_end - _mgs_data_start;
  const size_t sdata_size = _mgs_sdata_end - _mgs_sdata_start;
  if (!g_data_snapshot) {
    // one snapshot for both initialised regions: .data then the renamed .sdata
    g_data_snapshot =
        static_cast<uint8_t *>(heap_caps_malloc(data_size + sdata_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_data_snapshot) {
      memcpy(g_data_snapshot, _mgs_data_start, data_size);
      memcpy(g_data_snapshot + data_size, _mgs_sdata_start, sdata_size);
    }
    logger.info("statics: bss {} B, common {} B, psram bss {} B, sbss {} B, data {} B, sdata {} B (snapshot {})",
                _mgs_bss_end - _mgs_bss_start, _mgs_common_end - _mgs_common_start,
                _mgs_xbss_end - _mgs_xbss_start, _mgs_sbss_end - _mgs_sbss_start, data_size, sdata_size,
                g_data_snapshot ? "ok" : "FAILED");
    return;
  }
  memset(_mgs_bss_start, 0, _mgs_bss_end - _mgs_bss_start);
  memset(_mgs_common_start, 0, _mgs_common_end - _mgs_common_start);
  memset(_mgs_xbss_start, 0, _mgs_xbss_end - _mgs_xbss_start);
  memset(_mgs_sbss_start, 0, _mgs_sbss_end - _mgs_sbss_start);
  memcpy(_mgs_data_start, g_data_snapshot, data_size);
  memcpy(_mgs_sdata_start, g_data_snapshot + data_size, sdata_size);
}

// Wait (up to max_ms) for the mts scheduler to reach its idle task outside a
// critical section: at that point no game thread holds a lock (file, heap,
// stdio) and the whole thing can be frozen or torn down safely.
bool wait_for_idle(int max_ms) {
  for (int waited = 0; waited < max_ms; waited += 2) {
    if (mts_active_task_800C0DB0 == MTS_TASK_IDLE && psyz_critical_depth == 0) {
      return true;
    }
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  return false;
}

// Freeze the game at a quiescent point: hold the vblank tick first (nothing
// can wake an mts thread without it), then the running thread.
void freeze() {
  const bool idle = wait_for_idle(500);
  Mgs_PauseVblank();
  if (!idle && !(mts_active_task_800C0DB0 == MTS_TASK_IDLE && psyz_critical_depth == 0)) {
    // the tick is held now; give the running thread a last chance to park
    wait_for_idle(200);
  }
  Mgs_ThreadsPause();
  if (mts_active_task_800C0DB0 != MTS_TASK_IDLE) {
    logger.warn("frozen with mts task {} active (not idle)", mts_active_task_800C0DB0);
  }
}

// Hang detector. A task blocked on a lock is not a stuck core, so no watchdog
// reports a wedged game; this timer watches that the vblank tick and the
// game's frames keep advancing and, when they stop, prints the scheduler
// state through esp_rom_printf (which bypasses the stdio locks a wedged
// printf would hold).
esp_timer_handle_t g_hang_timer{nullptr};
volatile bool g_hang_enabled{false};
unsigned g_hang_vbl{0}, g_hang_frame{0};
int64_t g_hang_vbl_since{0}, g_hang_frame_since{0};

void hang_check(void *) {
  if (!g_hang_enabled) {
    return;
  }
  if (!heap_caps_check_integrity_all(false)) {
    esp_rom_printf("[heap] CORRUPT (periodic check, vbl %u)\n", mgs_vblank_count);
    heap_caps_check_integrity_all(true);
  }
  if (g_report_after_resume > 0 && (g_report_after_resume -= 120) <= 0) {
    Mgs_ThreadsReport("2 s after resume");
    esp_rom_printf("[state] mts active %d ready %08x crit %d | pad %02x %02x %02x %02x | pause %d status %08x | vbl %u frame %u\n",
                   mts_active_task_800C0DB0, mts_ready_tasks_800C0DB4, psyz_critical_depth, padbuf_800C1480[0],
                   padbuf_800C1480[1], padbuf_800C1480[2], padbuf_800C1480[3], GV_PauseLevel, GM_GameStatus,
                   mgs_vblank_count, mgs_frame_seq);
  }
  const int64_t now = esp_timer_get_time();
  if (mgs_vblank_count != g_hang_vbl) {
    g_hang_vbl = mgs_vblank_count;
    g_hang_vbl_since = now;
  }
  if (mgs_frame_seq != g_hang_frame) {
    g_hang_frame = mgs_frame_seq;
    g_hang_frame_since = now;
  }
  const bool tick_stuck = now - g_hang_vbl_since > 8000000;     // the tick runs every 16 ms
  const bool frame_stuck = now - g_hang_frame_since > 30000000; // a stage load takes seconds
  if (!tick_stuck && !frame_stuck) {
    return;
  }
  esp_rom_printf("[mgs] *** %s for %d ms: mts active %d crit %d vbl %u frame %u | tick in '%s', "
                 "in-printf %d, thread %d ***\n",
                 tick_stuck ? "vblank tick stalled" : "no new frame",
                 (int)((now - (tick_stuck ? g_hang_vbl_since : g_hang_frame_since)) / 1000),
                 mts_active_task_800C0DB0, psyz_critical_depth, mgs_vblank_count, mgs_frame_seq,
                 mgs_tick_phase, mgs_in_printf, Mgs_CurrentThread());
  esp_rom_printf("[mgs] where: %s\n", mgs_where);
  for (int i = 0; i < 8; i++) {
    unsigned pc = 0, ra = 0, sp = 0;
    const int st = Mgs_ThreadSample(i, &pc, &ra, &sp);
    if (st >= 0) {
      esp_rom_printf("[mgs] thread %d: state %d pc 0x%08x ra 0x%08x sp 0x%08x\n", i, st, pc, ra, sp);
      unsigned hits[20];
      const int n = Mgs_ThreadStackScan(i, hits, 20);
      esp_rom_printf("[mgs]   stack:");
      for (int k = 0; k < n; k++) {
        esp_rom_printf(" %08x", hits[k]);
      }
      esp_rom_printf("\n");
    }
  }
  // report again in 10 s if still stuck
  g_hang_vbl_since = g_hang_frame_since = now - 20000000;
}

void start_hang_detector() {
  const int64_t now = esp_timer_get_time();
  g_hang_vbl = mgs_vblank_count;
  g_hang_frame = mgs_frame_seq;
  g_hang_vbl_since = g_hang_frame_since = now;
  if (!g_hang_timer) {
    esp_timer_create_args_t args = {};
    args.callback = hang_check;
    args.name = "mgs_hang";
    esp_timer_create(&args, &g_hang_timer);
    esp_timer_start_periodic(g_hang_timer, 2000000);
  }
  g_hang_enabled = true;
}

void stop_hang_detector() {
  g_hang_enabled = false;
  if (g_hang_timer) {
    esp_timer_stop(g_hang_timer);
    esp_timer_delete(g_hang_timer);
    g_hang_timer = nullptr;
  }
}
} // namespace
#endif

namespace mgs {

const Config &config() { return g_config; }

bool init(const Config &config) {
  if (g_initialized) {
    logger.error("the game is already running");
    return false;
  }
  g_config = config;
  std::error_code ec;
  const auto stage_dir = std::filesystem::path(config.data_dir) / "STAGE.DIR";
  if (!std::filesystem::exists(stage_dir, ec)) {
    logger.error("no {}: extract the disc with port/extract_disc.py first", stage_dir.string());
    return false;
  }
#if defined(CONFIG_MGS_ENGINE)
  logger.info("internal free {} B, PSRAM free {} B", heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
  // a fresh machine for the game (see the top of this file); the rasterizer
  // caches the VRAM pointer in one of those statics
  reset_statics();
  Draw_Reset();
  g_preserve.clear();
  Mgs_ThreadsSnapshotSetup();
  Mgs_CdSnapshotSetup();
  Mgs_VblankSnapshotSetup();
  Mgs_PrintfSnapshotSetup();
  Psyz_GpuSnapshotSetup();
  g_main_returned = false;
  g_paused = false;
  // The game opens "cdrom:\MGS\NAME;1"; the translator maps that onto
  // <root>/MGS/NAME, so the root is the directory *above* the data dir, and
  // the directory must be called MGS (FAT is case-insensitive about it).
  const auto root = std::filesystem::path(config.data_dir).parent_path().string();
  Mgs_SetDataRoot(root.c_str());
  mgs_platform_init(config.data_dir.c_str());
  // the frame buffers the scanout converts VRAM into (the S3 port's app_main
  // does this; here it is the cart's job)
  if (lcd_init() != 0) {
    logger.error("could not get the HAL's frame buffers");
    return false;
  }
  // open the disc files (on the card: read on demand, sector by sector), then
  // the vblank the PSX gave for free: mts blocks on it during boot, and it
  // drives the scanout, so VRAM reaches the screen continuously
  Mgs_CdInit();
  mgs_platform_audio_start();
  Mgs_StartVblank();
  // The game's main(): it builds the mts scheduler, opens its threads and
  // hands over to them, so this task mostly sleeps suspended afterwards.
  // Core 0 with every mts task and the vblank tick (see esp32_threads.c for
  // why they must share a core); the scanout and the HAL's video task are
  // on core 1. Priority 5, the mts threads' own: the cart's poll loop runs
  // on this core at the main task's priority and must not starve it.
  g_main_task = xTaskCreateStaticPinnedToCore(
      [](void *) {
        logger.info("entering the game's main()");
        const int rc = mgs_main();
        logger.warn("the game's main() returned {}", rc);
        g_main_returned = true;
        vTaskSuspend(nullptr);
      },
      "mgs_main", kMainStackBytes / sizeof(StackType_t), nullptr, 5, g_main_stack, &g_main_tcb, 0);
  if (!g_main_task) {
    logger.error("could not create the game task");
    Mgs_StopVblank();
    mgs_platform_audio_stop();
    Mgs_CdDeinit();
    return false;
  }
  g_initialized = true;
  start_hang_detector();
  Mgs_HeapCheck("init: done");
  return true;
#else
  logger.error("built without CONFIG_MGS_ENGINE");
  return false;
#endif
}

bool running() { return g_initialized && !g_main_returned.load(); }

extern "C" void Mgs_SnapshotPreserve(void *p, size_t n) { g_preserve.emplace_back(p, n); }
extern "C" void Mgs_MainTaskStack(void **base, size_t *size) {
  *base = g_main_stack;
  *size = sizeof g_main_stack;
}

namespace {
struct SnapRegion {
  uint32_t addr;
  uint32_t size;
};
struct SnapHeader {
  char magic[4];
  uint32_t version;
  uint8_t elf_sha[32];     // informational: which build wrote it
  uint32_t n_regions;
  SnapRegion regions[7];
  uint32_t n_threads;
  MgsThreadSnap threads[MGS_SNAPSHOT_THREADS];
  uint32_t layout[12];     // what the state really depends on, see layout_signature()
};
constexpr uint32_t kSnapVersion = 3;

// A state is raw memory full of absolute addresses: pointers into the game's
// statics and into its code (actor callbacks, scheduler entries). It loads
// correctly into any build in which those addresses are unchanged -- not only
// the build that wrote it. The regions' addresses and sizes cover the data;
// a sample of the game's and the port's functions covers the code.
extern "C" {
int mgs_main(void);
void GV_ExecActorSystem(int);
void DG_SwapFrame(void);
void GCL_ExecBlock(void);
int Psyz_GpuExeque(void);
unsigned long OpenTh(unsigned long (*)(), unsigned long, unsigned long);
void mts_VSyncCallback(void);
void Psyz_SpuPullSamples(short *, int);
}
void layout_signature(uint32_t out[12]) {
  out[0] = reinterpret_cast<uint32_t>(&mgs_main);
  out[1] = reinterpret_cast<uint32_t>(&GV_ExecActorSystem);
  out[2] = reinterpret_cast<uint32_t>(&DG_SwapFrame);
  out[3] = reinterpret_cast<uint32_t>(&GCL_ExecBlock);
  out[4] = reinterpret_cast<uint32_t>(&Psyz_GpuExeque);
  out[5] = reinterpret_cast<uint32_t>(&OpenTh);
  out[6] = reinterpret_cast<uint32_t>(&mts_VSyncCallback);
  out[7] = reinterpret_cast<uint32_t>(&Psyz_SpuPullSamples);
  out[8] = reinterpret_cast<uint32_t>(&mgs_main_ram);
  out[9] = reinterpret_cast<uint32_t>(g_RawVram);
  out[10] = sizeof(SnapHeader);
  out[11] = MGS_SNAPSHOT_THREADS;
}

std::vector<SnapRegion> snapshot_regions() {
  return {
      {reinterpret_cast<uint32_t>(_mgs_bss_start), static_cast<uint32_t>(_mgs_bss_end - _mgs_bss_start)},
      {reinterpret_cast<uint32_t>(_mgs_common_start),
       static_cast<uint32_t>(_mgs_common_end - _mgs_common_start)},
      {reinterpret_cast<uint32_t>(_mgs_xbss_start), static_cast<uint32_t>(_mgs_xbss_end - _mgs_xbss_start)},
      {reinterpret_cast<uint32_t>(_mgs_data_start), static_cast<uint32_t>(_mgs_data_end - _mgs_data_start)},
      {reinterpret_cast<uint32_t>(_mgs_sbss_start), static_cast<uint32_t>(_mgs_sbss_end - _mgs_sbss_start)},
      {reinterpret_cast<uint32_t>(_mgs_sdata_start), static_cast<uint32_t>(_mgs_sdata_end - _mgs_sdata_start)},
      {reinterpret_cast<uint32_t>(g_main_stack), static_cast<uint32_t>(sizeof g_main_stack)},
  };
}

bool io_chunked(FILE *f, void *p, size_t n, bool writing) {
  auto *b = static_cast<uint8_t *>(p);
  while (n > 0) {
    const size_t chunk = n > 65536 ? 65536 : n;
    const size_t got = writing ? fwrite(b, 1, chunk, f) : fread(b, 1, chunk, f);
    if (got != chunk) {
      return false;
    }
    b += chunk;
    n -= chunk;
  }
  return true;
}
} // namespace

bool save_state(const std::string &path) {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized || !g_paused) {
    logger.error("save state: the game must be paused");
    return false;
  }
  const int64_t t0 = esp_timer_get_time();
  Psyz_GpuSync();
  SnapHeader h{};
  memcpy(h.magic, "MGSS", 4);
  h.version = kSnapVersion;
  memcpy(h.elf_sha, esp_app_get_description()->app_elf_sha256, sizeof h.elf_sha);
  const auto regions = snapshot_regions();
  h.n_regions = regions.size();
  for (size_t i = 0; i < regions.size(); i++) {
    h.regions[i] = regions[i];
  }
  h.n_threads = MGS_SNAPSHOT_THREADS;
  Mgs_ThreadsSnapshot(h.threads, MGS_SNAPSHOT_THREADS);
  layout_signature(h.layout);
  FILE *f = fopen(path.c_str(), "wb");
  if (!f) {
    logger.error("save state: cannot open {}", path);
    return false;
  }
  bool ok = fwrite(&h, 1, sizeof h, f) == sizeof h;
  size_t total = sizeof h;
  for (const auto &r : regions) {
    ok = ok && io_chunked(f, reinterpret_cast<void *>(r.addr), r.size, true);
    total += r.size;
  }
  fclose(f);
  logger.info("save state: {} {} ({} KB in {} ms)", ok ? "wrote" : "FAILED", path, total / 1024,
              (esp_timer_get_time() - t0) / 1000);
  return ok;
#else
  return false;
#endif
}

bool load_state(const std::string &path) {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized || !g_paused) {
    logger.error("load state: the game must be paused");
    return false;
  }
  const int64_t t0 = esp_timer_get_time();
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) {
    logger.error("load state: cannot open {}", path);
    return false;
  }
  SnapHeader h{};
  if (fread(&h, 1, sizeof h, f) != sizeof h || memcmp(h.magic, "MGSS", 4) != 0 || h.version != kSnapVersion) {
    logger.error("load state: {} is not a save state of this format", path);
    fclose(f);
    return false;
  }
  const bool same_build =
      memcmp(h.elf_sha, esp_app_get_description()->app_elf_sha256, sizeof h.elf_sha) == 0;
  uint32_t layout[12];
  layout_signature(layout);
  if (memcmp(h.layout, layout, sizeof layout) != 0) {
    logger.error("load state: {} was saved by a build whose game code or data sits at other addresses; "
                 "it cannot be loaded here",
                 path);
    fclose(f);
    return false;
  }
  const auto regions = snapshot_regions();
  if (h.n_regions != regions.size() || h.n_threads != MGS_SNAPSHOT_THREADS) {
    logger.error("load state: layout mismatch");
    fclose(f);
    return false;
  }
  for (size_t i = 0; i < regions.size(); i++) {
    if (h.regions[i].addr != regions[i].addr || h.regions[i].size != regions[i].size) {
      logger.error("load state: region {} differs (file {:#x}+{} vs {:#x}+{})", i, h.regions[i].addr,
                   h.regions[i].size, regions[i].addr, regions[i].size);
      fclose(f);
      return false;
    }
  }
  Psyz_GpuSync();
  // the one platform task that runs during a pause and reads this file's
  // statics: parked until the image and this run's handles are in place
  Mgs_CdHoldReadAhead();
  // this run's OS resources, out of the way of the memory image
  std::vector<std::vector<uint8_t>> kept;
  kept.reserve(g_preserve.size());
  for (const auto &[p, n] : g_preserve) {
    kept.emplace_back(static_cast<uint8_t *>(p), static_cast<uint8_t *>(p) + n);
  }
  bool ok = true;
  for (const auto &r : regions) {
    ok = ok && io_chunked(f, reinterpret_cast<void *>(r.addr), r.size, false);
  }
  fclose(f);
  for (size_t i = 0; i < g_preserve.size(); i++) {
    memcpy(g_preserve[i].first, kept[i].data(), kept[i].size());
  }
  if (!ok) {
    Mgs_CdReleaseReadAhead();
    logger.error("load state: short read from {} -- the game memory is now inconsistent, reset", path);
    return false;
  }
  if (!Mgs_ThreadsRestore(h.threads, MGS_SNAPSHOT_THREADS)) {
    Mgs_CdReleaseReadAhead();
    logger.error("load state: could not restore the game threads -- reset");
    return false;
  }
  Mgs_CdAfterRestore();
  Mgs_CdReleaseReadAhead();
  Psyz_GpuAfterRestore();
  Mgs_PrintfAfterRestore();
  mgs_frame_seq++; // show the restored frame
  g_hang_vbl = mgs_vblank_count;
  g_hang_frame = mgs_frame_seq;
  g_hang_vbl_since = g_hang_frame_since = esp_timer_get_time();
  logger.info("load state: {} restored in {} ms{}", path, (esp_timer_get_time() - t0) / 1000,
              same_build ? "" : " (written by another build with the same layout)");
  Mgs_ThreadsReport("after restore");
  g_report_after_resume = 120; // frames: the pad and scheduler state once running
  return true;
#else
  return false;
#endif
}

extern "C" void Mgs_DumpVram(const char *path) {
  Psyz_GpuSync();
  if (FILE *f = fopen(path, "wb")) {
    const size_t n = fwrite(g_RawVram, 2, 1024u * 512u, f);
    fclose(f);
    logger.info("VRAM dump: {} ({} px)", path, n);
  } else {
    logger.warn("VRAM dump: could not open {}", path);
  }
}

extern "C" void Mgs_HeapCheck(const char *where) {
  const bool ok = heap_caps_check_integrity_all(true);
  esp_rom_printf("[heap] %s: %s\n", where, ok ? "ok" : "CORRUPT");
}

void pause() {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized || g_paused) {
    return;
  }
  Mgs_HeapCheck("pause: before freeze");
  g_hang_enabled = false;
  freeze();
  Mgs_HeapCheck("pause: after freeze");
  mgs_platform_audio_pause();
  g_paused = true;
  {
    // Debug: the whole 1024x512x16 VRAM to the card, so what the textures and
    // the font areas actually hold can be inspected on a PC (raw RGB1555,
    // little-endian, row stride 1024 pixels).
    Psyz_GpuSync();
    if (FILE *f = fopen("/sdcard/mgs_vram.bin", "wb")) {
      const size_t n = fwrite(g_RawVram, 2, 1024u * 512u, f);
      fclose(f);
      logger.info("VRAM dump: /sdcard/mgs_vram.bin ({} px)", n);
    } else {
      logger.warn("VRAM dump: could not open /sdcard/mgs_vram.bin");
    }
  }
  Mgs_HeapCheck("pause: after vram dump");
#endif
}

void resume() {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized || !g_paused) {
    return;
  }
  g_paused = false;
  mgs_platform_audio_resume();
  Mgs_ThreadsResume();
  Mgs_ResumeVblank();
  start_hang_detector();
#endif
}

void deinit() {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized) {
    return;
  }
  g_initialized = false;
  stop_hang_detector();
  // stop at a quiescent point (see freeze()); if the menu paused us the game
  // is already frozen there
  if (!g_paused) {
    freeze();
  }
  g_paused = true;
  // nothing may wake an mts thread from here on
  Mgs_StopVblank();
  Mgs_ThreadsStopAll();
  if (g_main_task) {
    vTaskDelete(g_main_task);
    g_main_task = nullptr;
  }
  // let the scheduler retire the deleted tasks before their memory goes away
  vTaskDelay(pdMS_TO_TICKS(5));
  mgs_platform_audio_stop(); // before the SPU state below is wiped
  Mgs_CdDeinit();
  // the game is gone: its statics can be put back for the next launch
  reset_statics();
  Mgs_HeapCheck("deinit: done");
  logger.info("stopped; internal free {} B, PSRAM free {} B", heap_caps_get_free_size(MALLOC_CAP_INTERNAL),
              heap_caps_get_free_size(MALLOC_CAP_SPIRAM));
#endif
}

bool reset() {
  const auto config = g_config;
  deinit();
  return init(config);
}

std::pair<size_t, size_t> video_size() { return {MGS_FRAME_W, MGS_FRAME_H}; }

std::span<uint8_t> video_buffer_rgb565() { return mgs_platform_last_frame(); }

} // namespace mgs

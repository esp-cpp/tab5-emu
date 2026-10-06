#include "mgs.hpp"

#include "sdkconfig.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "esp_heap_caps.h"
#include "esp_rom_sys.h"
#include "esp_timer.h"
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
// linker.lf SURROUND symbols for libmgs.a's statics
extern char _mgs_bss_start[], _mgs_bss_end[];
extern char _mgs_common_start[], _mgs_common_end[];
extern char _mgs_xbss_start[], _mgs_xbss_end[];
extern char _mgs_data_start[], _mgs_data_end[];
}

namespace {
constexpr int MTS_TASK_IDLE = 11;

// Put every static of libmgs.a back to its as-linked value. Only valid while
// none of the game's tasks exist.
void reset_statics() {
  const size_t data_size = _mgs_data_end - _mgs_data_start;
  if (!g_data_snapshot) {
    // first launch: the sections are pristine, remember .data
    g_data_snapshot = static_cast<uint8_t *>(heap_caps_malloc(data_size, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT));
    if (g_data_snapshot) {
      memcpy(g_data_snapshot, _mgs_data_start, data_size);
    }
    logger.info("statics: bss {} B, common {} B, psram bss {} B, data {} B (snapshot {})",
                _mgs_bss_end - _mgs_bss_start, _mgs_common_end - _mgs_common_start,
                _mgs_xbss_end - _mgs_xbss_start, data_size, g_data_snapshot ? "ok" : "FAILED");
    return;
  }
  memset(_mgs_bss_start, 0, _mgs_bss_end - _mgs_bss_start);
  memset(_mgs_common_start, 0, _mgs_common_end - _mgs_common_start);
  memset(_mgs_xbss_start, 0, _mgs_xbss_end - _mgs_xbss_start);
  memcpy(_mgs_data_start, g_data_snapshot, data_size);
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
  auto ok = xTaskCreatePinnedToCoreWithCaps(
      [](void *) {
        logger.info("entering the game's main()");
        const int rc = mgs_main();
        logger.warn("the game's main() returned {}", rc);
        g_main_returned = true;
        vTaskSuspend(nullptr);
      },
      "mgs_main", 16 * 1024, nullptr, 5, &g_main_task, 0, MALLOC_CAP_SPIRAM);
  if (ok != pdPASS) {
    logger.error("could not create the game task");
    Mgs_StopVblank();
    mgs_platform_audio_stop();
    Mgs_CdDeinit();
    return false;
  }
  g_initialized = true;
  start_hang_detector();
  return true;
#else
  logger.error("built without CONFIG_MGS_ENGINE");
  return false;
#endif
}

bool running() { return g_initialized && !g_main_returned.load(); }

void pause() {
#if defined(CONFIG_MGS_ENGINE)
  if (!g_initialized || g_paused) {
    return;
  }
  g_hang_enabled = false;
  freeze();
  mgs_platform_audio_pause();
  g_paused = true;
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
    vTaskDeleteWithCaps(g_main_task);
    g_main_task = nullptr;
  }
  // let the scheduler retire the deleted tasks before their memory goes away
  vTaskDelay(pdMS_TO_TICKS(5));
  mgs_platform_audio_stop(); // before the SPU state below is wiped
  Mgs_CdDeinit();
  // the game is gone: its statics can be put back for the next launch
  reset_statics();
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

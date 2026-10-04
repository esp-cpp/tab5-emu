#include "mgs.hpp"

#include "sdkconfig.h"

#include <atomic>
#include <cstdio>
#include <cstring>
#include <filesystem>

#include "esp_heap_caps.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "logger.hpp"
#include "tab5-emu.hpp"

#include "mgs_platform.hpp"

namespace {
espp::Logger logger({.tag = "mgs", .level = espp::Logger::Verbosity::INFO});
mgs::Config g_config;
std::atomic<bool> g_started{false};
std::atomic<bool> g_main_returned{false};
TaskHandle_t g_main_task{nullptr};
} // namespace

#if defined(CONFIG_MGS_ENGINE)
extern "C" {
int mgs_main(void);                  // source/main/main.c, renamed by the build
void Mgs_SetDataRoot(const char *p); // port/platform_headless.c
void Mgs_StartVblank(void);          // port/esp32_vblank.c: vblank tick + scanout tasks
void Mgs_CdInit(void);               // port/virtual_cd.c: open the disc files
extern volatile int mgs_paused;      // port/esp32_vblank.c
int lcd_init(void);                  // mgs_platform.cpp: the HAL frame buffers
}
#endif

namespace mgs {

const Config &config() { return g_config; }

bool init(const Config &config) {
  if (g_started.load()) {
    logger.error("the game cannot be started twice in one boot");
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
  // the vblank the PSX gave for free; mts blocks on it during boot. It also
  // drives the scanout, so VRAM reaches the screen continuously.
  mgs_paused = 0;
  Mgs_StartVblank();
  // open the disc files (on the card: read on demand, sector by sector)
  Mgs_CdInit();
  g_main_returned = false;
  g_started = true;
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
    return false;
  }
  return true;
#else
  logger.error("built without CONFIG_MGS_ENGINE");
  return false;
#endif
}

bool running() { return g_started.load() && !g_main_returned.load(); }

void pause() {
#if defined(CONFIG_MGS_ENGINE)
  mgs_paused = 1;
#endif
}

void resume() {
#if defined(CONFIG_MGS_ENGINE)
  mgs_paused = 0;
#endif
}

void deinit() {
  if (!g_started.load()) {
    return;
  }
  // The game's statics (its heaps, the mts scheduler, psyz's GPU state) are
  // laid out once by main() and never torn down; upstream never stops it
  // either. Until that exists, leaving the game means starting over.
  logger.warn("stopping the game: rebooting (the game cannot be restarted in place)");
  fflush(stdout);
  vTaskDelay(pdMS_TO_TICKS(100));
  esp_restart();
}

std::pair<size_t, size_t> video_size() { return {MGS_FRAME_W, MGS_FRAME_H}; }

std::span<uint8_t> video_buffer_rgb565() { return mgs_platform_last_frame(); }

} // namespace mgs

// The board-specific half of the upstream port's platform layer, for the Tab5:
// what esp32/main/{lcd_esp32,esp_input,esp_sd}.c are on the S3 boards. The
// portable half (threads, vblank, virtual CD, software GPU) is compiled from
// the upstream tree unchanged, and calls into these by name.

#include "mgs_platform.hpp"

#include <atomic>
#include <cstring>
#include <string>

#include "esp_attr.h"

#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "statistics.hpp"
#include "tab5-emu.hpp"

#include "libpad.h"

namespace {
std::string g_sd_root; // the directory with STAGE.DIR etc.
uint8_t *g_frames[2]{nullptr, nullptr};
int g_frame_index{0};
std::atomic<uint8_t *> g_last_frame{nullptr};
unsigned char *g_pad_buf[2]{nullptr, nullptr};
} // namespace

void mgs_platform_init(const char *data_dir) { g_sd_root = data_dir ? data_dir : ""; }

// ---------------------------------------------------------------------------
// Audio: psyz emulates the SPU (24 ADPCM voices, the game's own driver talks
// to it through the PSY-Q libspu); its mixer is pull-based at the PSX's
// 44.1 kHz. A task on core 1 pulls ~10 ms at a time into the HAL's queue,
// paced by the DAC (play_audio waits for room), and sits suspended while the
// menu is up.
// ---------------------------------------------------------------------------

extern "C" {
void Psyz_SpuInit(void);
void Psyz_SpuPullSamples(short *out, int num_frames);
extern volatile unsigned psyz_spu_irq_raised, psyz_spu_voices_active, psyz_spu_irq_caught_up; // psyz_spu.c
extern volatile unsigned mgs_spu_irq_delivered;                        // port/esp32_vblank.c
extern unsigned mgs_prof_present_us;                                   // port/esp32_vblank.c
void Psyz_SpuDebug(unsigned *spucnt, unsigned *irq_addr, int v, unsigned *cur_addr, unsigned *active);
}

namespace {
constexpr int kAudioRate = 44100;
constexpr int kAudioFrames = 221; // 5 ms: the SPU interrupt is raised at pull time
TaskHandle_t g_audio_task{nullptr};
std::atomic<bool> g_audio_stop{false};
std::atomic<bool> g_audio_done{false};

void audio_task(void *) {
  auto &emu = Tab5Emu::get();
  static int16_t buf[kAudioFrames * 2];
  int64_t last_report = esp_timer_get_time();
  unsigned loud = 0, pulls = 0;
  int64_t mix_us = 0;
  while (!g_audio_stop.load()) {
    const int64_t t0 = esp_timer_get_time();
    Psyz_SpuPullSamples(buf, kAudioFrames);
    mix_us += esp_timer_get_time() - t0;
    emu.play_audio(reinterpret_cast<const uint8_t *>(buf), sizeof(buf));
    pulls++;
    for (int i = 0; i < kAudioFrames * 2; i += 8) {
      if (buf[i] > 64 || buf[i] < -64) {
        loud++;
      }
    }
    const int64_t now = esp_timer_get_time();
    if (now - last_report > 5000000) {
      unsigned cnt, irq, cur, act;
      Psyz_SpuDebug(&cnt, &irq, 23, &cur, &act);
      printf("[audio] pulls %u loud-samples %u voices %u spu-irq raised %u delivered %u | spucnt %04x irq_addr %04x "
             "v23 cur %05x active %u | mix %u ms present %u ms caught-up %u\n",
             pulls, loud, psyz_spu_voices_active, psyz_spu_irq_raised, mgs_spu_irq_delivered, cnt, irq, cur, act,
             (unsigned)(mix_us / 1000), mgs_prof_present_us / 1000u, psyz_spu_irq_caught_up);
      mix_us = 0;
      mgs_prof_present_us = 0;
      pulls = loud = 0;
      last_report = now;
    }
  }
  g_audio_done = true;
  vTaskSuspend(nullptr);
}
} // namespace

void mgs_platform_audio_start() {
  if (g_audio_task) {
    return;
  }
  Psyz_SpuInit();
  auto &emu = Tab5Emu::get();
  emu.audio_sample_rate(kAudioRate);
  emu.audio_max_wait_ms(30);
  g_audio_stop = false;
  g_audio_done = false;
  // Core 0: the game core has slack while it waits for the rasterizer on
  // core 1, and the mixer (13-30% of a core) was eating into drawing time.
  xTaskCreatePinnedToCore(audio_task, "mgs_audio", 4096, nullptr, 7, &g_audio_task, 0);
}

void mgs_platform_audio_stop() {
  if (!g_audio_task) {
    return;
  }
  g_audio_stop = true;
  vTaskResume(g_audio_task); // in case it was paused
  while (!g_audio_done.load()) {
    vTaskDelay(pdMS_TO_TICKS(2));
  }
  vTaskDelete(g_audio_task);
  g_audio_task = nullptr;
}

void mgs_platform_audio_pause() {
  if (g_audio_task) {
    vTaskSuspend(g_audio_task);
  }
}

void mgs_platform_audio_resume() {
  if (g_audio_task) {
    vTaskResume(g_audio_task);
  }
}

std::span<uint8_t> mgs_platform_last_frame() {
  auto *f = g_last_frame.load();
  if (!f) {
    return {};
  }
  return {f, MGS_FRAME_W * MGS_FRAME_H * 2};
}

extern "C" {

// ---------------------------------------------------------------------------
// Display: the scanout task (port/esp32_vblank.c) hands over the display area
// of VRAM whenever the game flips a frame.
// ---------------------------------------------------------------------------

int lcd_init(void) {
  auto &emu = Tab5Emu::get();
  g_frames[0] = emu.frame_buffer0();
  g_frames[1] = emu.frame_buffer1();
  emu.palette(nullptr, 0); // RGB565 native frames
  emu.native_size(MGS_FRAME_W, MGS_FRAME_H);
  return (g_frames[0] && g_frames[1]) ? 0 : -1;
}

// src points at the top-left of the 320x240 display area inside the PSX's
// 1024-halfword-wide VRAM, in BGR555 (R bits 0-4, G 5-9, B 10-14, bit 15 the
// mask bit). The HAL wants RGB565, so convert into one of its two frame
// buffers (alternating: the frame handed to push_frame() must stay valid until
// the next one) and queue it; the HAL's video task scales it to the panel.
void lcd_present(const unsigned short *src) {
  auto *dst = reinterpret_cast<uint16_t *>(g_frames[g_frame_index]);
  if (!dst) {
    return;
  }
  for (size_t y = 0; y < MGS_FRAME_H; y++) {
    const uint16_t *row = src + y * 1024;
    uint16_t *out = dst + y * MGS_FRAME_W;
    for (size_t x = 0; x < MGS_FRAME_W; x++) {
      const uint16_t v = row[x];
      const uint16_t r = v & 0x1F;
      const uint16_t g = (v >> 5) & 0x1F;
      const uint16_t b = (v >> 10) & 0x1F;
      out[x] = static_cast<uint16_t>((r << 11) | (((g << 1) | (g >> 4)) << 5) | b);
    }
  }
  g_last_frame = g_frames[g_frame_index];
  Tab5Emu::get().push_frame(dst);
  g_frame_index ^= 1;
  // the menu's frame statistics: the interval between presented frames (the
  // game flips on its own tasks; nothing else here is per-frame)
  static int64_t last_us = 0;
  const int64_t now = esp_timer_get_time();
  if (last_us) {
    update_frame_time(static_cast<uint64_t>(now - last_us));
  }
  last_us = now;
}

void lcd_wait_snapshot(void) {}

// ---------------------------------------------------------------------------
// Disc data: the card is mounted by the HAL before any cart starts, and the
// SDMMC slot shares nothing with the display, so the SPI arbitration the S3
// boards need is a no-op here.
// ---------------------------------------------------------------------------

int Mgs_SdCardInit(void) { return 0; }
void Mgs_SpiBusTake(void) {}
void Mgs_SpiBusGive(void) {}
const char *Mgs_SdRoot(void) { return g_sd_root.c_str(); }

// ---------------------------------------------------------------------------
// Pads: mts_pad.c registers two PAD_RECV_BUF buffers through PadInitDirect()
// and parses them every vsync as raw SIO pad frames: byte 0 result (0 = ok),
// byte 1 terminal type / size (0x41 = digital pad, one halfword), bytes 2-3
// the buttons, ACTIVE LOW, as (hi << 8 | lo):
//   hi: 0x80 LEFT  0x40 DOWN  0x20 RIGHT  0x10 UP
//       0x08 START 0x04 R3    0x02 L3     0x01 SELECT
//   lo: 0x80 SQUARE 0x40 CROSS 0x20 CIRCLE 0x10 TRIANGLE
//       0x08 R1     0x04 L1    0x02 R2     0x01 L2
// The HAL's merged gamepad (touch zones, USB pad, keyboard) is sampled by the
// vblank tick through Mgs_PadsUpdate(). Face buttons map by position: A
// (south) = CROSS, B (east) = CIRCLE, X (west) = SQUARE, Y (north) = TRIANGLE,
// which is where they sit on a DualShock.
// ---------------------------------------------------------------------------

void PadInitDirect(unsigned char *pad1, unsigned char *pad2) {
  g_pad_buf[0] = pad1;
  g_pad_buf[1] = pad2;
  if (pad1) {
    pad1[0] = 0xFF; // nothing parsed yet
  }
  if (pad2) {
    pad2[0] = 0xFF; // port 1: nothing connected
  }
}

void PadStartCom(void) {} // PadStopCom: port/psyq_compat.c

int PadGetState(int port) {
  // "a plain controller is present" on port 0; mts's discovery machine
  // settles into IDENTIFIED and parses the buffer
  return port == 0 ? PadStateFindCTP1 : PadStateDiscon;
}

void PadSetAct(int port, unsigned char *data, int len) {
  (void)port;
  (void)data;
  (void)len; // no rumble
}

int PadSetActAlign(int port, char *data) {
  (void)port;
  (void)data;
  return 0;
}

// called by the vblank tick (port/esp32_vblank.c) right before the mts
// callback, so the frame the game parses this field is this field's state
void Mgs_PadsUpdate(void) {
  if (!g_pad_buf[0]) {
    return;
  }
  const auto s = Tab5Emu::get().gamepad_state();
  unsigned pressed = 0;
  if (s.left) pressed |= 0x8000;
  if (s.down) pressed |= 0x4000;
  if (s.right) pressed |= 0x2000;
  if (s.up) pressed |= 0x1000;
  if (s.start) pressed |= 0x0800;
  if (s.select) pressed |= 0x0100;
  if (s.x) pressed |= 0x0080; // SQUARE
  if (s.a) pressed |= 0x0040; // CROSS
  if (s.b) pressed |= 0x0020; // CIRCLE
  if (s.y) pressed |= 0x0010; // TRIANGLE
  if (s.r) pressed |= 0x0008; // R1
  if (s.l) pressed |= 0x0004; // L1
  g_pad_buf[0][0] = 0x00;
  g_pad_buf[0][1] = 0x41;
  g_pad_buf[0][2] = static_cast<unsigned char>(~(pressed >> 8) & 0xFF);
  g_pad_buf[0][3] = static_cast<unsigned char>(~pressed & 0xFF);
  if (g_pad_buf[1]) {
    g_pad_buf[1][0] = 0xFF;
  }
}

// called once per frame from the game's own loop (libdg/dgd.c); the serial
// taps the S3 boards age here have no equivalent on the Tab5
void Mgs_PadConsumed(void) {}

} // extern "C"

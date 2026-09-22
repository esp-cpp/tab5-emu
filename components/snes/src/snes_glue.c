// snes9x2005 host glue (ported from its libretro.c: settings, display
// buffers, audio pull, save states) without libretro.
#include "snes_glue.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <esp_heap_caps.h>

#include "snes9x.h"
#include "memmap.h"
#include "apu_blargg.h"
#include "gfx.h"
#include "cpuexec.h"
#include "ppu.h"
#include "display.h"
#include "sa1.h"
#include "spc7110.h"
#include "srtc.h"
#include "soundux.h"
#include "cheats.h"

#include <libretro.h>

#define SNES_AUDIO_RATE 32040

// core options the libretro front end owns (defaults: no overclock)
bool overclock_cycles = false;
bool reduce_sprite_flicker = false;
int one_c = 6, slow_one_c = 8, two_c = 12;
#define SCREEN_SAFETY 32

static uint8_t *screen_buffers[2] = {NULL, NULL};
static int screen_index = 0;
static int16_t *audio_buffer = NULL;
static size_t audio_buffer_frames = 0;

static void *psram_malloc(size_t n) {
  void *p = heap_caps_malloc(n, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
  return p ? p : malloc(n);
}

static void init_settings(void) {
  memset(&Settings, 0, sizeof(Settings));
  Settings.JoystickEnabled = false;
  Settings.SoundPlaybackRate = SNES_AUDIO_RATE;
  Settings.SoundInputRate = SNES_AUDIO_RATE;
  Settings.CyclesPercentage = 100;
  Settings.DisableSoundEcho = false;
  Settings.InterpolatedSound = true;
  Settings.APUEnabled = true;
  Settings.H_Max = SNES_CYCLES_PER_SCANLINE;
  Settings.FrameTimePAL = 20000;
  Settings.FrameTimeNTSC = 16667;
  Settings.DisableMasterVolume = false;
  Settings.Mouse = false;
  Settings.SuperScope = false;
  Settings.MultiPlayer5 = false;
  Settings.ControllerOption = SNES_JOYPAD;
  Settings.SoundSync = false;
  Settings.ApplyCheats = false;
  Settings.HBlankStart = (256 * Settings.H_Max) / SNES_HCOUNTER_MAX;
}

// like S9xInitDisplay, with two output screens so the video task can convert
// one while the PPU draws the next (all in PSRAM)
static bool init_display(void) {
  const int32_t h = IMAGE_HEIGHT;
  GFX.Pitch = IMAGE_WIDTH * 2;
  for (int i = 0; i < 2; i++) {
    screen_buffers[i] = (uint8_t *)psram_malloc(GFX.Pitch * h + SCREEN_SAFETY);
    if (!screen_buffers[i]) return false;
    memset(screen_buffers[i], 0, GFX.Pitch * h + SCREEN_SAFETY);
  }
  GFX.Screen_buffer = screen_buffers[0];
  GFX.SubScreen_buffer = (uint8_t *)psram_malloc(GFX.Pitch * h + SCREEN_SAFETY);
  GFX.ZBuffer_buffer = (uint8_t *)psram_malloc((GFX.Pitch >> 1) * h + SCREEN_SAFETY);
  GFX.SubZBuffer_buffer = (uint8_t *)psram_malloc((GFX.Pitch >> 1) * h + SCREEN_SAFETY);
  if (!GFX.SubScreen_buffer || !GFX.ZBuffer_buffer || !GFX.SubZBuffer_buffer) return false;
  memset(GFX.SubScreen_buffer, 0, GFX.Pitch * h + SCREEN_SAFETY);
  memset(GFX.ZBuffer_buffer, 0, (GFX.Pitch >> 1) * h + SCREEN_SAFETY);
  memset(GFX.SubZBuffer_buffer, 0, (GFX.Pitch >> 1) * h + SCREEN_SAFETY);
  GFX.Screen = GFX.Screen_buffer + SCREEN_SAFETY;
  GFX.SubScreen = GFX.SubScreen_buffer + SCREEN_SAFETY;
  GFX.ZBuffer = GFX.ZBuffer_buffer + SCREEN_SAFETY;
  GFX.SubZBuffer = GFX.SubZBuffer_buffer + SCREEN_SAFETY;
  GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
  return true;
}

static void deinit_display(void) {
  for (int i = 0; i < 2; i++) {
    free(screen_buffers[i]);
    screen_buffers[i] = NULL;
  }
  free(GFX.SubScreen_buffer);
  free(GFX.ZBuffer_buffer);
  free(GFX.SubZBuffer_buffer);
  GFX.Screen = GFX.Screen_buffer = NULL;
  GFX.SubScreen = GFX.SubScreen_buffer = NULL;
  GFX.ZBuffer = GFX.ZBuffer_buffer = NULL;
  GFX.SubZBuffer = GFX.SubZBuffer_buffer = NULL;
}

// blargg APU: pull everything it has produced. Registered as the APU's
// samples-available callback (S9xAPUExecute calls it unconditionally, so it
// must exist) and also run at the end of each frame.
static void audio_pull(void) {
  S9xFinalizeSamples();
  const size_t samples = S9xGetSampleCount(); // interleaved stereo samples
  if (samples == 0) return;
  if (samples / 2 > audio_buffer_frames) {
    free(audio_buffer);
    audio_buffer_frames = samples / 2 + 512;
    audio_buffer = (int16_t *)psram_malloc(audio_buffer_frames * 2 * sizeof(int16_t));
    if (!audio_buffer) {
      audio_buffer_frames = 0;
      return;
    }
  }
  S9xMixSamples(audio_buffer, samples);
  snes_host_audio(audio_buffer, samples / 2);
}

static size_t loaded_rom_size = 0;
size_t snes_glue_rom_size(void) { return loaded_rom_size; }

static bool snes_glue_init_common(uint8_t *rom, size_t rom_size);

bool snes_glue_init(uint8_t *rom, size_t rom_size) {
  if (!rom || rom_size < 0x8000) {
    printf("snes: no ROM data (%u bytes)\n", (unsigned)rom_size);
    return false;
  }
  return snes_glue_init_common(rom, rom_size);
}

bool snes_glue_init_file(const char *path) {
  // the core's buffer is MAX_ROM_SIZE + 0x200 (+ 0x8000 in front); read the
  // file into it and let LoadROM strip a copier header in place
  return snes_glue_init_common((uint8_t *)path, (size_t)-1);
}

static bool snes_glue_init_common(uint8_t *rom, size_t rom_size) {
  init_settings();
  CPU.Flags = 0;
  loaded_rom_size = 0;
  if (!S9xInitMemory()) { printf("snes: S9xInitMemory failed\n"); return false; }
  if (rom_size == (size_t)-1) {
    // rom is a path: read the file into Memory.ROM
    const char *path = (const char *)rom;
    FILE *f = fopen(path, "rb");
    if (!f) { printf("snes: cannot open %s\n", path); return false; }
    size_t total = 0;
    const size_t cap = MAX_ROM_SIZE + 0x200;
    while (total < cap) {
      size_t chunk = cap - total < 65536 ? cap - total : 65536;
      size_t n = fread(Memory.ROM + total, 1, chunk, f);
      total += n;
      if (n < chunk) break;
    }
    fclose(f);
    if (total < 0x8000) { printf("snes: %s: only %u bytes\n", path, (unsigned)total); return false; }
    rom = Memory.ROM;
    rom_size = total;
  }
  if (!S9xInitAPU()) { printf("snes: S9xInitAPU failed\n"); return false; }
  if (!init_display()) { printf("snes: display buffers failed\n"); return false; }
  if (!S9xInitGFX()) { printf("snes: S9xInitGFX failed\n"); return false; }
  S9xInitSound(0, 0);
  S9xSetSamplesAvailableCallback(audio_pull);
  S9xSetSoundMute(false);
  CPU.SaveStateVersion = 0;
  struct retro_game_info game = {0};
  game.data = rom;
  game.size = rom_size;
  if (!LoadROM(&game)) { printf("snes: LoadROM failed (%u bytes)\n", (unsigned)rom_size); return false; }
  loaded_rom_size = rom_size;
  Settings.FrameTime = Settings.PAL ? Settings.FrameTimePAL : Settings.FrameTimeNTSC;
  screen_index = 0;
  return true;
}

void snes_glue_deinit(void) {
  if (Settings.SPC7110) Del7110Gfx();
  S9xDeinitGFX();
  deinit_display();
  S9xDeinitAPU();
  S9xDeinitMemory();
  free(audio_buffer);
  audio_buffer = NULL;
  audio_buffer_frames = 0;
}

void snes_glue_reset(void) {
  CPU.Flags = 0;
  S9xSoftReset();
}

void snes_glue_run_frame(bool render) {
  IPPU.RenderThisFrame = render;
  S9xMainLoop();
  audio_pull();
}

uint8_t *snes_glue_screen_swap(void) {
  screen_index ^= 1;
  GFX.Screen_buffer = screen_buffers[screen_index];
  GFX.Screen = GFX.Screen_buffer + SCREEN_SAFETY;
  GFX.Delta = (GFX.SubScreen - GFX.Screen) >> 1;
  return GFX.Screen;
}

uint8_t *snes_glue_last_screen(void) { return screen_buffers[screen_index ^ 1] + SCREEN_SAFETY; }
int snes_glue_width(void) { return IPPU.RenderedScreenWidth; }
int snes_glue_height(void) { return IPPU.RenderedScreenHeight; }
int snes_glue_pitch(void) { return GFX.Pitch; }
bool snes_glue_pal(void) { return Settings.PAL; }
uint32_t snes_glue_audio_rate(void) { return SNES_AUDIO_RATE; }

// ---- save states (retro_serialize / retro_unserialize, blargg APU build) ----
size_t snes_glue_state_size(void) {
  return sizeof(CPU) + sizeof(ICPU) + sizeof(PPU) + sizeof(DMA) + 0x10000 + 0x20000 + 0x20000 + 0x8000 +
         SPC_SAVE_STATE_BLOCK_SIZE + sizeof(SA1) + sizeof(s7r) + sizeof(rtc_f9);
}

bool snes_glue_state_save(void *data, size_t size) {
  if (size < snes_glue_state_size()) return false;
  uint8_t *buffer = data;
  S9xPackStatus();
  S9xUpdateRTC();
  S9xSRTCPreSaveState();
  memcpy(buffer, &CPU, sizeof(CPU)); buffer += sizeof(CPU);
  memcpy(buffer, &ICPU, sizeof(ICPU)); buffer += sizeof(ICPU);
  memcpy(buffer, &PPU, sizeof(PPU)); buffer += sizeof(PPU);
  memcpy(buffer, &DMA, sizeof(DMA)); buffer += sizeof(DMA);
  memcpy(buffer, Memory.VRAM, 0x10000); buffer += 0x10000;
  memcpy(buffer, Memory.RAM, 0x20000); buffer += 0x20000;
  memcpy(buffer, Memory.SRAM, 0x20000); buffer += 0x20000;
  memcpy(buffer, Memory.FillRAM, 0x8000); buffer += 0x8000;
  S9xAPUSaveState(buffer); buffer += SPC_SAVE_STATE_BLOCK_SIZE;
  SA1.Registers.PC = SA1.PC - SA1.PCBase;
  S9xSA1PackStatus();
  memcpy(buffer, &SA1, sizeof(SA1)); buffer += sizeof(SA1);
  memcpy(buffer, &s7r, sizeof(s7r)); buffer += sizeof(s7r);
  memcpy(buffer, &rtc_f9, sizeof(rtc_f9));
  return true;
}

bool snes_glue_state_load(const void *data, size_t size) {
  const uint8_t *buffer = data;
  uint32_t sa1_old_flags = SA1.Flags;
  static SSA1 sa1_state; // 33 KB (two 4096-entry maps): not on the task stack
  if (size != snes_glue_state_size()) return false;
  S9xReset();
  memcpy(&CPU, buffer, sizeof(CPU)); buffer += sizeof(CPU);
  memcpy(&ICPU, buffer, sizeof(ICPU)); buffer += sizeof(ICPU);
  memcpy(&PPU, buffer, sizeof(PPU)); buffer += sizeof(PPU);
  memcpy(&DMA, buffer, sizeof(DMA)); buffer += sizeof(DMA);
  memcpy(Memory.VRAM, buffer, 0x10000); buffer += 0x10000;
  memcpy(Memory.RAM, buffer, 0x20000); buffer += 0x20000;
  memcpy(Memory.SRAM, buffer, 0x20000); buffer += 0x20000;
  memcpy(Memory.FillRAM, buffer, 0x8000); buffer += 0x8000;
  S9xAPULoadState(buffer); buffer += SPC_SAVE_STATE_BLOCK_SIZE;
  memcpy(&sa1_state, buffer, sizeof(sa1_state)); buffer += sizeof(sa1_state);
  SA1.Flags = sa1_state.Flags;
  SA1.NMIActive = sa1_state.NMIActive;
  SA1.IRQActive = sa1_state.IRQActive;
  SA1.WaitingForInterrupt = sa1_state.WaitingForInterrupt;
  SA1.op1 = sa1_state.op1;
  SA1.op2 = sa1_state.op2;
  SA1.arithmetic_op = sa1_state.arithmetic_op;
  SA1.sum = sa1_state.sum;
  SA1.overflow = sa1_state.overflow;
  memcpy(&SA1.Registers, &sa1_state.Registers, sizeof(SA1.Registers));
  memcpy(&s7r, buffer, sizeof(s7r)); buffer += sizeof(s7r);
  memcpy(&rtc_f9, buffer, sizeof(rtc_f9));
  S9xFixSA1AfterSnapshotLoad();
  SA1.Flags |= sa1_old_flags & (TRACE_FLAG);
  FixROMSpeed();
  IPPU.ColorsChanged = true;
  IPPU.OBJChanged = true;
  CPU.InDMA = false;
  S9xFixColourBrightness();
  ICPU.ShiftedPB = ICPU.Registers.PB << 16;
  ICPU.ShiftedDB = ICPU.Registers.DB << 16;
  S9xSetPCBase(ICPU.ShiftedPB + ICPU.Registers.PC);
  S9xUnpackStatus();
  S9xFixCycles();
  S9xReschedule();
  return true;
}

// ---- host hooks the core expects (from libretro.c) ----
uint32_t S9xReadJoypad(int32_t port) { return snes_host_read_joypad(port); }
bool S9xReadMousePosition(int32_t which1, int32_t *x, int32_t *y, uint32_t *buttons) { return false; }
bool S9xReadSuperScopePosition(int32_t *x, int32_t *y, uint32_t *buttons) { return false; }
bool JustifierOffscreen(void) { return false; }
void JustifierButtons(uint32_t *justifiers) { (void)justifiers; }

#pragma once

// C interface between the OpenJKDF2 ESP32 platform layer (C) and the C++
// glue in jk.cpp (which talks to the Tab5Emu HAL).

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint16_t buttons;   // GamepadState::buttons bit layout (a,b,x,y,select,start,up,down,left,right)
  int16_t touch_x;    // logical (landscape) touch position, -1 if none
  int16_t touch_y;
  uint8_t touch_down;
  // USB HID keyboard: bitmap over HID usage IDs (== SDL scancodes), bit
  // (usage & 7) of keys[usage >> 3]; modifiers at usages 0xE0..0xE7
  uint8_t keys[32];
  // USB HID mouse: buttons (bit0 left, bit1 right, bit2 middle) and the
  // motion pending since it was last taken with jk_esp_mouse_take()
  uint8_t mouse_buttons;
  int16_t mouse_dx;
  int16_t mouse_dy;
  int16_t mouse_wheel;
} jk_esp_input_t;

/// Present an 8-bit paletted frame (pal is 256 RGB triplets).
void jk_esp_present_8bpp(const uint8_t *pixels, int width, int height, int pitch, const uint8_t *pal24);
/// Present an 8-bit paletted frame with a separate 8-bit overlay (HUD) that
/// shares its palette. The overlay is drawn 1:1 (index 0 transparent), centered,
/// over the frame after the frame has been integer-upscaled to at least the
/// overlay's height, so a HUD drawn at a higher resolution than the world stays
/// sharp. overlay may be NULL.
void jk_esp_present_8bpp_overlay(const uint8_t *pixels, int width, int height, int pitch,
                                 const uint8_t *overlay, int overlay_width, int overlay_height,
                                 int overlay_pitch, const uint8_t *pal24);
/// Log presentation statistics (presents, video task time, wait time) since
/// the previous call; used by the engine's periodic frame report.
void jk_esp_present_report(void);
/// Present an RGB565 frame.
void jk_esp_present_rgb565(const uint16_t *pixels, int width, int height, int pitch);
/// Read the current input state (non-consuming).
void jk_esp_read_input(jk_esp_input_t *out);
/// Take (and clear) the mouse motion accumulated since the last take.
void jk_esp_mouse_take(int *dx, int *dy, int *wheel);
/// Whether a USB keyboard / mouse is attached
int jk_esp_keyboard_present(void);
int jk_esp_mouse_present(void);
/// Queue interleaved 16-bit stereo PCM for playback (non-blocking); returns
/// the number of frames accepted (fewer than num_frames when the output
/// queue is full).
size_t jk_esp_audio_write(const int16_t *stereo_pcm, size_t num_frames);
/// Configure the audio output sample rate.
void jk_esp_audio_set_rate(uint32_t sample_rate);
/// The audio output sample rate the mixer should produce.
uint32_t jk_esp_audio_rate(void);
/// Lock protecting the mixer's voice list: the glue's audio task mixes under
/// it, the engine takes it when it starts/stops/frees buffers.
void jk_esp_audio_lock(void);
void jk_esp_audio_unlock(void);
/// Music (CD soundtrack) streaming: Ogg Vorbis files decoded on a background
/// task and mixed into the software mixer's output.
/// Open `path` (relative to the game dir or absolute) and start streaming;
/// returns 1 on success. Replaces whatever was playing.
int jk_esp_music_open(const char *path);
void jk_esp_music_stop(void);
/// 1 while the current file still has audio to play
int jk_esp_music_playing(void);
/// Music volume 0..1
void jk_esp_music_volume(float volume);
/// Mix the pending music into `frames` stereo frames at `rate` (called by the
/// software mixer under the audio lock)
void jk_esp_music_mix(int16_t *stereo, size_t frames, uint32_t rate);
/// Milliseconds since boot.
uint32_t jk_esp_time_ms(void);
/// Internal, DMA-capable memory (SD card reads); NULL when exhausted.
void *jk_esp_malloc_dma(size_t bytes);
size_t jk_esp_psram_free(void);
void jk_esp_free_dma(void *ptr);
/// Called by the engine at safe points (the frame loop, the modal menu
/// loops): blocks while the emulator has the engine paused. Returns 1 if it
/// did block (the caller may want to redraw).
int jk_esp_park_point(void);
void jk_esp_file_release_buffers(void);
/// Microseconds since boot.
uint64_t jk_esp_time_us(void);
/// Sleep for the given milliseconds (yields to other tasks).
void jk_esp_sleep_ms(uint32_t ms);
/// Large allocation from PSRAM (engine heap) / free.
void *jk_esp_malloc(size_t size);
void *jk_esp_realloc(void *ptr, size_t size);
void jk_esp_free(void *ptr);
/// Same, tagged with the engine call site (debug builds track live
/// allocations per site; jk_esp_alloc_dump() lists the biggest).
void *jk_esp_malloc_site(size_t size, const void *site);
void *jk_esp_realloc_site(void *ptr, size_t size, const void *site);
void jk_esp_alloc_dump(int top);
/// Log line
void jk_esp_log(const char *fmt, ...);
/// Set to 1 by the glue when the engine should quit at the next opportunity.
extern volatile int jk_esp_quit_requested;
/// Internal (native) render size configured for the world buffer.
int jk_esp_internal_width(void);
int jk_esp_internal_height(void);
/// The game data directory (engine working directory), e.g. /sdcard/jk
const char *jk_esp_game_dir(void);
/// Command line for the engine (CONFIG_JK_ENGINE_ARGS)
const char *jk_esp_engine_args(void);

#ifdef __cplusplus
}
#endif

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
  int16_t mouse_dx;   // relative mouse motion since the last read (USB mouse later)
  int16_t mouse_dy;
  uint8_t mouse_buttons;
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
/// Read the current input state.
void jk_esp_read_input(jk_esp_input_t *out);
/// Queue interleaved 16-bit stereo PCM for playback; returns bytes accepted.
size_t jk_esp_audio_write(const int16_t *stereo_pcm, size_t num_frames);
/// Configure the audio output sample rate.
void jk_esp_audio_set_rate(uint32_t sample_rate);
/// Milliseconds since boot.
uint32_t jk_esp_time_ms(void);
/// Microseconds since boot.
uint64_t jk_esp_time_us(void);
/// Sleep for the given milliseconds (yields to other tasks).
void jk_esp_sleep_ms(uint32_t ms);
/// Large allocation from PSRAM (engine heap) / free.
void *jk_esp_malloc(size_t size);
void *jk_esp_realloc(void *ptr, size_t size);
void jk_esp_free(void *ptr);
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

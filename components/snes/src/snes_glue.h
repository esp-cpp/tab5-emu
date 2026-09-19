#pragma once
// C side of the snes9x2005 integration (the core headers are C only)
#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// host callbacks the C++ side provides
uint32_t snes_host_read_joypad(int port); // SNES_*_MASK bits
void snes_host_audio(const int16_t *stereo, size_t frames);

bool snes_glue_init(uint8_t *rom, size_t rom_size);
void snes_glue_deinit(void);
void snes_glue_reset(void);
// run one frame; render = whether the PPU should draw it (frameskip)
void snes_glue_run_frame(bool render);
// the frame buffer the *next* frame will be drawn into / the last drawn one
uint8_t *snes_glue_screen_swap(void);
uint8_t *snes_glue_last_screen(void);
int snes_glue_width(void);
int snes_glue_height(void);
int snes_glue_pitch(void); // bytes
bool snes_glue_pal(void);
uint32_t snes_glue_audio_rate(void);
size_t snes_glue_state_size(void);
bool snes_glue_state_save(void *buf, size_t size);
bool snes_glue_state_load(const void *buf, size_t size);

#ifdef __cplusplus
}
#endif

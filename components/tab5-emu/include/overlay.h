#pragma once

#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/// Make a game core's code runnable: in builds that execute from flash, the
/// cores share one PSRAM code overlay (overlay.ld) and this copies the named
/// core's code into it (if it is not the one there already) and, on the
/// core's first load, runs its static constructors. Call it before any of
/// the core's code runs (its cart's constructor) and only while no other
/// core's code runs. With XIP from PSRAM every core is always there and this
/// does nothing.
///
/// `core` is the overlay member's name: gba, snes, genesis, doom, nes, gbc,
/// sms, msx, pico8, jk, darkforces, mgs or scummvm. Returns false for any
/// other name.
bool tab5_overlay_load(const char *core);

/// The core whose code is in the overlay now ("" for none; always "" with XIP
/// from PSRAM).
const char *tab5_overlay_loaded(void);

#ifdef __cplusplus
}
#endif

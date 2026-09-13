# tab5-emu: plan

Goal: a Jedi Knight (Dark Forces II) port running on the M5Stack Tab5, in a
project shaped like esp-box-emu so game/emulator glue can move between the two.

## Why OpenJKDF2, and why it is feasible

- OpenJKDF2 is a ~96% complete C reimplementation of JK.EXE, permissively
  licensed, single-threaded, 32-bit-native (the original was a 32-bit Win32
  binary), and already has two 16 MB / no-SDL / no-GL ports that define the
  platform contract a new target implements:
  - Nintendo DSi (`src/Platform/TWL`): ARM9 @ 133 MHz, 16 MB, fixed point,
    DS GPU for triangles.
  - Dreamcast (`src/Platform/Dreamcast`): SH4 @ 200 MHz, 16 MB, float
    (single precision only), PowerVR for triangles.
- Since 2026-07 it also has a CPU rasterizer (`RDRASTER_SOFTWARE_RENDERER`,
  `src/Raster/rdZRaster.c`: perspective-correct, z-buffered, 8-bit paletted
  output through the classic light tables). Upstream gates it off on the retro
  targets for CPU reasons; the ESP32-P4 (2x RISC-V @ 360 MHz with FPU) is
  several times faster than either retro target, so the plan is:
  **`TARGET_RETRO_HOMEBREW` memory diet + software rasterizer**, rendering at
  a low internal resolution and letting the P4's PPA scale/rotate to 1280x720.
- Original 1997 requirements: Pentium 90, 16 MB RAM, all-software rendering at
  320x200..640x480. Tab5: 32 MB PSRAM (~27 MB usable for the engine after
  display buffers).
- Game data streams from the SD card through plain `fopen`/`fread` (the GOB
  container is read on demand; only the entry table is resident). Res1hi.gob +
  Res2.gob are ~57 MB; levels are a few MB each.

## Platform contract (what the port has to provide)

Modeled on `src/Platform/TWL/*` and `src/Platform/Dreamcast/*`:

| File (components/jk/platform) | Role |
|---|---|
| `Window_Esp.c` | main loop entry (`Window_Main_Linux`), message pump, mouse/keyboard events from the HAL |
| `stdDisplay_Esp.c` | 8-bit `tVBuffer` implementation (alloc/lock/copy/fill), master palette, mode set (internal resolution), flip |
| `std3D_Esp.c` | "no hardware 3D" backend: texture cache stubs, `std3D_DrawMenu` presents the 8-bit world/menu buffer via the HAL |
| `stdSound_Esp.c` | software mixer feeding the ES8388 through the HAL audio task |
| `stdControl_Esp.c` | keys / axes from the HAL gamepad state (touch now, USB HID later) |
| `jkGUIDisplay_Esp.c` | the "Display" options page, trimmed |
| `stdPlatform` hooks | file I/O: `fopen` on `/sdcard/jk/...`; allocation: PSRAM heap |

## Video path

```
engine 8bpp frame (W x H, e.g. 320x240)   -- Video_pSwWorldBuffer / Video_menuBuffer
  -> palette LUT -> RGB565 staging (internal SRAM)      [video task, CPU]
  -> PPA SRM: scale to display size + rotate 90        [hardware]
  -> written directly into the MIPI-DSI frame buffer (no extra copy)
```

The GUI (LVGL) is not driven while a game runs, so the panel frame buffer is
the game's. `Tab5Emu::display_size()` picks original/fit/fill scaling.

## Memory budget (32 MB PSRAM)

| | |
|---|---|
| DPI frame buffer (720x1280 RGB565) | 1.8 MB (BSP) |
| LVGL draw buffers + PPA scratch | ~0.7 MB (BSP) |
| RGB565 staging (native res) | 0.15 MB (internal if possible) |
| Engine 8bpp buffers (world, menu, z-buffer) | ~1.5 MB at 640x480 menu / 320x240 world |
| Engine heap (world, models, materials LRU, COG) | remainder, ~25 MB |

## Phases

0. **Scaffold** (this repo, done): HAL, launcher, pause menu, carts, CI,
   PPA video path, test-pattern cart, OpenJKDF2 fork + submodule.
1. **Engine compiles and links** for riscv32 as an ESP-IDF component
   (`CONFIG_JK_ENGINE=y`): Dreamcast-style flags (all float, single precision,
   `-fno-fast-math -ffp-contract=off`), `TARGET_RETRO_HOMEBREW` +
   `RDRASTER_SOFTWARE_RENDERER`, platform layer stubs. Milestone: main menu
   renders (2D path is 100% decompiled and pure software).
2. **World renders** through `rdZRaster` at 320x240 (or 426x240 16:9);
   profile; move hot rasterizer loops to IRAM; tune PSRAM cache behavior.
   Milestone: first level playable at >= 20 fps with the 20 Hz physics tick.
3. **Audio**: software mixer (port of the DSi `stdSound.c`) at 22050 Hz into
   the HAL audio path; music from `MUSIC/*.ogg` deferred (needs a decoder).
4. **Input**: USB HID host on the Tab5's USB-A port (keyboard + mouse, or a
   gamepad) via `usb_host_hid`; IMU aim as a stretch. Touch remains for menus.
5. **Save / load** through the engine's native `.jks` saves under
   `/sdcard/jk/player`, wired to the pause menu slots.
6. **Cutscenes** (`.SMK` via libsmacker) if memory allows; else skip.
7. **Dark Forces** cart shared with the esp-box-emu `dark-forces-port`
   worktree: the HAL API mirrors BoxEmu (`display_size`, `native_size`,
   `palette`, `push_frame`, `gamepad_state`, `play_audio`, ...), so the glue
   ports by swapping the HAL type. Engine code is not shared (different GOB
   container, formats and engine).

## Risks

- PSRAM latency for z-buffer/texture reads; mitigations: internal-SRAM
  staging, lower internal resolution, L2 cache config, IRAM for hot loops.
- Level memory on `TARGET_RETRO_HOMEBREW` limits is proven on 16 MB targets;
  we have ~25 MB.
- ESP-IDF newlib vs the engine's POSIX expectations (`fopen` case-insensitive
  paths are handled by `fcaseopen`; long file names need `CONFIG_FATFS_LFN`).
- Touch-only input is not a good way to play JK; USB HID is the real target.

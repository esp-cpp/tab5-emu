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

0. **Scaffold** (done): HAL, launcher, pause menu, carts, CI, PPA video
   path, test-pattern cart, OpenJKDF2 fork + submodule.
1. **Engine compiles and links** for riscv32 as an ESP-IDF component
   (`CONFIG_JK_ENGINE=y`): Dreamcast-style flags (all float, single precision,
   `-fno-fast-math -ffp-contract=off`), `TARGET_RETRO_HOMEBREW` +
   `RDRASTER_SOFTWARE_RENDERER`, platform layer in
   `OpenJKDF2/src/Platform/ESP32`. **Done (2026-09-13): links, 2.6 MB app,
   1.27 MB engine .bss moved to PSRAM (tools/bss_to_psram.py).** Next
   milestone: main menu renders on hardware (2D path is 100% decompiled and
   pure software). Open items for that: verify `Main_Startup` finds the
   game data (paths are made absolute under the game dir in
   `stdPlatform.c`), stack size of the cart task (engine uses big stack
   buffers; run it on a task with a PSRAM stack), `std3D_DrawMenu` present
   rate, `stdSound_ESP32_Pump()` still needs to be called from the frame
   loop.
2. **World renders** through `rdZRaster` at 426x240 (16:9), 2D layer at
   640x480, PPA scales to the panel. **Done (2026-09-18): level 1 playable,
   ~19 fps.** The HUD stays at its native 640x480: the engine hands it to the
   HAL as a keyed overlay, the video task builds an 852x480 staging image
   (world 2x nearest + HUD 1:1, 4:3-centered) in 64 KB internal-RAM strips
   and the PPA scales each strip 1.5x into the rotated panel buffer
   (video task ~26 ms/frame on core 1, ~18 fps). Pause menu save / load /
   reset go through the engine's own save files (absolute slot paths on the
   SD card; reset = level-start autosave). Quit + relaunch in-process works
   (2026-09-18): the platform shutdown runs jkMain_GameplayLeave, the engine
   clears jkGui_GdiMode and the COG parser pool on shutdown, the engine task's
   PSRAM stack is freed with vTaskDeleteWithCaps; per-cycle PSRAM leak is now
   ~2 KB (debug allocation tracker: `alloc:` / `files:` lines after
   shutdown). Remaining: profile the
   rasterizer, IRAM for hot loops, PSRAM cache tuning, cut the engine-side
   frame/HUD copy (~4 ms/frame).
3. **Audio**: software mixer (port of the DSi `stdSound.c`) into the HAL audio
   path. **Done (2026-09-18):** mixes at the codec's 48 kHz on a 10 ms glue
   task (`jk_audio`, core 1) with back-pressure pacing and a mutex around the
   voice list; confirmed working on hardware. **Music (2026-09-18):** the
   soundtrack streams from `MUSIC/Track*.wav` (IMA ADPCM, made once with
   `tools/convert_music.py`; ~5% of a core incl. SD reads) through a
   core-1 decode task and a PCM ring the mixer resamples from; `.ogg` also
   plays via stb_vorbis but costs ~75% of a core, so it is only a fallback.
4. **Input**: USB HID host on the Tab5's USB-A port. **Done (2026-09-18):
   keyboard + mouse** through the USB host library (hubs enabled, USB DMA
   memory in PSRAM) and Espressif's HID class driver; HID usages feed the
   engine's SDL-scancode table, mouse motion is look input in game and the
   cursor in menus. Not yet: gamepads (generic HID needs per-device report
   parsing), hub verified on hardware, in-game touch layout. IMU aim as a
   stretch. Touch remains for menus.
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

## Emulator cores (2026-09-19)

- **Genesis / Mega Drive (gwenesis)**: shared with esp-box-emu (components
  `genesis` + `shared_memory`, `box-emu.hpp` aliases BoxEmu to Tab5Emu). Runs
  full speed: Sonic 1 at ~11 ms emulation per 60 Hz frame (88 fps capability).
  Internal RAM was the blocker (22 KB free at cart start): profiler tables,
  video tile and frame buffers are now PSRAM / per-cart, main stack 16 KB;
  ~150 KB free before a cart starts. Remaining levers if a core needs more:
  the engine's ~60 KB of GUI menu tables in .data, L2 cache 256 -> 128 KB.
- **SNES (snes9x2005)** and **GBA (gpSP interpreter)** are in (2026-09-19):
  `components/snes` drives the core through a port of its libretro front end
  (`snes_glue.c`); `components/gba` compiles gpSP's own libretro front end
  and hosts it with a minimal libretro environment (`gba.cpp`). Both cores'
  static state is moved to PSRAM with `tools/bss_to_psram.py`. Adaptive
  frameskip in both (skip after an over-budget frame).
  **Verified on hardware 2026-09-20:** SNES (Super Mario World, Zelda) runs
  at speed (~14 ms/frame avg incl. skipped frames, audio clean); the SNES
  ROM buffer comes from a 4 MB PSRAM arena the HAL reserves at boot (an
  8 MB contiguous block is not available after a JK session; JK releases
  the arena while it runs). GBA plays but feels slow: ~15.5 ms/frame avg
  with rendered frames up to 70 ms, i.e. ~40-50 rendered fps with
  frameskip. **GBA performance pass (2026-09-20):** -O3, the CPU registers
  / IO / OAM / palette kept in internal RAM, never skipping two frames in a
  row (gpSP allowed 30), cumulative frame pacing, the whole 16 MB ROM
  resident (SD paging stalled the emulation). Result: smooth, ~80-90% of
  real time in heavy scenes (Fire Red: ~21 ms of emulation per frame vs
  16.7 budget; light scenes run at speed and sleep). The sampling profile
  (`jk_prof` hooked into the GBA cart in debug builds) shows the time spread
  evenly over the interpreter's opcode dispatch and per-instruction loop
  header, ~40% in the two big switches; rendering is ~5%, audio <1%, the
  idle-loop skip is active. No hot spot left to fix. Remaining levers, all
  substantial: execute_arm (74 KB) in internal RAM (the executable region
  `sram_low` is 175 KB and full of IDF's IRAM code; a smaller L2 cache only
  grows the data region, so this needs a custom placement plus disabling
  the PMP I/D split), or a RISC-V dynarec for gpSP (none exists).

## Internal RAM policy (2026-09-20)

Internal RAM (~440 KB DIRAM, ~150 KB free after link) is the constraint,
not PSRAM. Rules that keep everything booting:

- Emulator cores keep no static state in internal RAM (`bss_to_psram.py`
  for .bss; big initialized tables are `const` so they live in flash).
- Plain `malloc` of >256 bytes goes to PSRAM
  (`CONFIG_SPIRAM_MALLOC_ALWAYSINTERNAL=256`); anything that needs internal
  or DMA memory asks for it explicitly. JK's level load otherwise drained
  the DMA heap and SD reads failed.
- The 64 KB video tile is allocated at boot and kept; allocating it at the
  first frame fell back to PSRAM once the GUI had fragmented the heap (JK
  dropped from 38 to 21 fps).
- The 128 KB DMA reserve pool at boot must fit: watch the
  "internal RAM after ..." log lines (currently ~76 KB free after the GUI).
  Remaining levers: the GUI's ~48 KB (not small mallocs; likely task
  stacks), JK's 60 KB of GUI tables in .data, L2 cache 256 -> 128 KB.

## USB drive (2026-09-20)

Settings > "USB drive": espp::UsbDevice (MSC, auto hand-over) over the
BSP's espp::SdCard on the USB-C port. The P4 has two full-speed PHYs; the
Tab5's USB-C is PHY 0 (GPIO24/25, the USB-Serial-JTAG console's by default)
and the OTG 1.1 controller sits on PHY 1 (GPIO26/27 = I2S). `usb_msc.cpp`
swaps the controllers with `LP_SYS.usb_ctrl` (console pads off, 300 ms
disconnect gap, then TinyUSB on port 0) and swaps back on switch-off; the
HAL restores the console mapping at boot since the register survives a
software reset. The USB host (HID) is stopped while the drive is on.
Verified: the drive appears on a Mac, files edited, card handed back.
`UsbDevice::Config::port` is in espp PR #802 (local override until
released).

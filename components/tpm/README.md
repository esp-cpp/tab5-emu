# Star Wars Episode I: The Phantom Menace, natively

The PlayStation game (Big Ape Productions / LucasArts, 1999), USA Rev 1
(`SLUS_008.84`), running natively on the ESP32-P4 through
[tpm-decomp](https://github.com/esp-cpp/tpm-decomp) (submodule `tpm-decomp/`):
its static recompiler turns the game's executable into C at build time, and
its runtime is the PlayStation around that C (BIOS, interrupts, DMA, CD-ROM,
GPU ports) on psyz's software rasterizer, GTE and SPU, the same pieces the
Metal Gear Solid port in `components/mgs` uses. The decompilation in that
repo continues alongside.

## Building

The recompiler needs **your own** copy of the executable at build time
(nothing derived from it is committed). Extract it from your disc image and
put it where `CONFIG_TPM_EXE_PATH` points (the default):

```sh
python3 components/tpm/tpm-decomp/tools/extract_disc.py "<your disc>.bin" /tmp/tpm
cp /tmp/tpm/SLUS_008.84 components/tpm/tpm-decomp/disks/us/
idf.py build
```

CMake runs `tools/recomp.py` when it configures (and again whenever the
executable or the recompiler changes); the C lands in the build directory.
Without the executable the build still succeeds, with a warning, and the
cart falls back to the load-screen viewer below.

`components/mgs` links its own psyz under the same names, so the game, the
runtime and their psyz pieces are linked into one relocatable object and
everything but the runtime's API (`tpm_rt.keep`) is made local.

## On the SD card

The disc image: the `.bin` of a bin/cue rip (raw 2352-byte sectors, one
track). The executable is read out of it, so nothing else is needed:

```
/sdcard/tpm/Star Wars - Episode I - The Phantom Menace (USA) (Rev 1).bin
```

and a metadata entry (the path names the cart; the file itself may be absent):

```
tpm/SLUS_008.84,boxart/tpm.jpg,Star Wars Episode I: The Phantom Menace
```

Only USA Rev 1 works: the recompiled code is that executable.

## Running

Status (2026-10-13, on the Tab5): boots from the card through the memory-card
notice, the title menu and the loading screens into the first level (the
Trade Federation ship), drawn correctly, at about **12 fps** against the
game's 30 (the game runs in slow motion: emulated time never skips ahead).

- Threads: the game on core 0 (32 KB internal stack); the GPU's command
  stream on core 1 (`tpm_gpu`: the rasterizer, behind the game's back like
  the real GPU's FIFO; a vblank marker in the queue copies the displayed
  rows once the drawing before it is done); RGB565 conversion and the SPU's
  audio on core 1 too.
- Time: emulated time advances with the work done and never runs ahead of
  the wall clock; when the board is slower the game runs slower (jumping the
  clock made libetc's VSync time out).
- Memory: the runtime's statics are in PSRAM (`tpm_rt.ld`) and put back
  before every launch; guest RAM, VRAM, the SPU and the GPU queue come from
  the shared-memory pool and go back to it when the cart quits. Internal RAM
  holds only the guest register file and the game task's stack.
- Disc reads go through a 32-sector read-ahead.
- Pads: the HAL's merged gamepad; A = cross, B = circle, X = square,
  Y = triangle, L/R = L1/R1.
- Movies are skipped (no MDEC yet), XA speech / music is silent (not decoded
  yet), there is no memory card (the game says so and carries on). At the
  current speed the SPU makes samples at ~40% of real time, so sound breaks
  up.
- The pause menu pauses the game at its next loop back-edge.

Where the time goes (in the level): the game thread is busy all the time,
about 10% SPU, a few % CD, the rest recompiled game code and psyz's GTE;
the GPU worker is ~80% busy. Next: the GTE (16% of the game thread before
the GPU moved), the recompiled code, the rasterizer's spans.

### Bring-up aids (menuconfig, "Test:" options)

- `CONFIG_TAB5_EMU_AUTOSTART_DELAY_MS` + `CONFIG_TAB5_EMU_AUTOSTART_MATCH`
  (main): launch the game whose name or path matches, e.g. `tpm/`.
- `CONFIG_TPM_AUTOPRESS`: scripted pad presses by vblank, e.g.
  `300:cross,600:cross,900:start,1200:cross,1500:cross` reaches the first
  level.
- `CONFIG_TPM_SERIAL_SCREENSHOT_SECONDS`: half-resolution screenshots on the
  console (`tools/serial_screenshots.py log out/` makes PNGs; printing
  stalls core 1 for a few seconds each time);
  `CONFIG_TPM_SERIAL_FULL_SHOT_VBLANK`: all of VRAM once.
- `CONFIG_TPM_GPU_TRACE_VBLANK`: the GPU state commands of one frame, to
  diff against the desktop runner's `--gpu-trace N` (the emulated timeline
  is deterministic: the same presses give the same frames).
- `CONFIG_TPM_PROFILE_VBLANK`: 8000 PC samples of the game thread, printed
  as `TPMPROF pc ra` for addr2line.
- A status line every 5 s: vblanks, frames presented, the game's frame rate,
  and the game thread's time split.

Without the engine (or without a `.bin` on the card) the cart shows the
game's 44 load screens from extracted disc files (`FILE.000` and
`SLUS_008.84` in `/sdcard/tpm/`), every `CONFIG_TPM_SLIDE_SECONDS` or on
left / right.

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

- Threads: the game on core 0 (32 KB internal stack), VRAM -> RGB565
  conversion and the SPU's audio on core 1. Time is real time: vblanks, CD
  sectors and audio samples follow the clock.
- Pads: the HAL's merged gamepad; A = cross, B = circle, X = square,
  Y = triangle, L/R = L1/R1.
- Movies are skipped (no MDEC yet), XA speech / music is silent (not decoded
  yet), there is no memory card (the game says so and carries on).
- The pause menu pauses the game at its next loop back-edge.

Without the engine (or without a `.bin` on the card) the cart shows the
game's 44 load screens from extracted disc files (`FILE.000` and
`SLUS_008.84` in `/sdcard/tpm/`), every `CONFIG_TPM_SLIDE_SECONDS` or on
left / right.

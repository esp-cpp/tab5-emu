# Metal Gear Solid, natively

Not an emulator: the FoxdieTeam decompilation of the game
([mgs_reversing](https://github.com/FoxdieTeam/mgs_reversing), via
davidmonterocrespo24's `esp32-port` branch) compiled for the ESP32-P4 against
[psyz](https://github.com/Xeeynamo/psyz) (his fork's `esp32` branch), a
PlayStation SDK reimplementation with a software GTE and GPU. The S3 port this
follows is described at
<https://velxio.dev/blog/posts/metal-gear-solid-on-esp32-s3/>.

Layout:

| | |
|---|---|
| `mgs_reversing/` | submodule: the game (497 C files) + its portable port layer (`port/`: PSX threads on FreeRTOS, the vblank, a virtual CD over files, the software GPU binding) |
| `psyz/` | submodule: psyz (`psyz/`) and the PSY-Q decomp libraries it builds on (`decomp/`) |
| `src/` | the Tab5 half of the platform layer: display (VRAM -> RGB565 -> the HAL), pads (the HAL's merged gamepad), data root, pause, lifecycle |
| `shim/` | the three headers the upstream build took from a private directory: `libpad.h`, `gtemac.h`, `libsn.h` |
| `psyq_sources.cmake` | the curated list of PSY-Q decomp units that build (from the SOTN port) |

## Game data

Nothing derived from the game is in this repository. Extract the files from
**your own** disc image with the upstream script and copy them to the card:

```sh
python3 components/mgs/mgs_reversing/port/extract_disc.py <your-disc.bin> <out-dir>
```

then put the result in a directory called `MGS` on the card (any case: the
game addresses its files as `cdrom:\MGS\NAME`), e.g. `/sdcard/mgs/STAGE.DIR`,
`RADIO.DAT`, `FACE.DAT`, ... The full 71 MB `STAGE.DIR` is wanted: it is read
sector by sector on demand, nothing is copied into RAM.

The script also writes `MGS/TEXT.DAT` (or only that, with `--text`): the item
and weapon descriptions of the menus, pulled from the disc's executable. The
decompilation targets MGS Integral and carries its Japanese descriptions as
byte arrays, which through the US disc's font draw as garbage; with the file
present the port patches in the disc's own English text at font load and
switches the glyph lookup to the US executable's table layout (its font has
no kana, so the button icons and the item-name brackets sit at different
indices). Without the file the menus show the Integral text.

The metadata entry points at the marker file:

```
mgs/STAGE.DIR,boxart/mgs.jpg,Metal Gear Solid
```

## Status

- The whole game builds (1 100-odd translation units, RISC-V toolchain:
  `-std=gnu17`, `-fno-strict-aliasing` and the PSY-Q-era diagnostic
  opt-outs, no Xtensa-specific flags).
- Boots the retail chain from the US disc: title, opening cutscene, the
  dock, the elevator, topside. Cutscenes play with their cameras and the
  frame-buffer feedback effects (zoom blur, underwater refraction); codec
  calls with portraits and text; sound effects, music and streamed speech.
- About 28 fps at the dock against the game's own 30 fps cap. Two
  rasterizer workers, one per core, draw bands of rows of the same command
  stream; the split adapts to the two cores' load. Workers rendezvous before
  a primitive that samples the frame buffer and around VRAM block moves.
- Save and load states from the pause menu (the game's static regions,
  every thread's stack and context), keyed on the memory layout so a
  rebuild that leaves the game code in place keeps old states loadable.
- Pause, reset, relaunch and quit in place.
- Memory card: the game's own saves (Mei Ling's codec frequency, 140.85;
  the title's load) go to `/sdcard/saves/mgs_card1.mcd`, a raw 128 KB card
  image as PC emulators use, made the first time the game looks. Unlike
  save states they survive any rebuild.
- Not done: full-motion video (MDEC,
  movies are skipped). 56 of the 96 stages have decompiled actors upstream.

## Measuring

The pause menu writes the last five frames' GPU batches to the card next
to a VRAM dump (`/sdcard/mgs_gpu_frame.bin`, `/sdcard/mgs_vram.bin`). At
the next cart start the rasterizer replays them before the game runs: each
frame rendered by the exact single-worker path to `mgs_cmp_f*.bin`, the
newest compared pixel for pixel under every optimisation (`[cmp]` lines),
then timed with two workers and with one (`[bench] frame replay`). The
serial telemetry every 120 frames covers both workers' raster time, the
split row, the display-list walk and the audio and CD counters.

## Where things are decided

- `mgs_reversing` carries three small platform hooks on a local `esp32p4`
  branch (SD root, mts stack memory, a pause flag); they are weak / defaulted
  so the S3 build is unaffected.
- mts task stacks live in PSRAM (`MGS_THREAD_STACK_CAPS`): the Tab5's internal
  RAM is its scarce memory, and unlike the S3 boards nothing here reads disc
  data from flash while the game runs (the cache hazard that forced internal
  stacks upstream).
- The game's `.bss` is moved to PSRAM after the archive is built
  (`tools/bss_to_psram.py`), as for the other cores.
- Every mts task and the vblank tick are pinned to core 0 (they must share a
  core, see `port/esp32_vblank.c`); the scanout and the HAL's video task run
  on core 1.

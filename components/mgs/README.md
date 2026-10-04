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
sector by sector on demand, nothing is copied into RAM. The metadata entry
points at the marker file:

```
mgs/STAGE.DIR,boxart/mgs.jpg,Metal Gear Solid
```

## Status

- Builds (the whole game, 1 100-odd translation units, with the RISC-V
  toolchain: only `-std=gnu17`, `-fno-strict-aliasing` and the PSY-Q-era
  diagnostic opt-outs, no Xtensa-specific flags).
- Boots into `CONFIG_MGS_START_STAGE` (default `s00a`, the dock). Upstream
  status applies: no audio (no SPU), no memory card (no saves), codec and
  FMV declined, 56 of the 96 stages have decompiled actors.
- The game cannot be restarted in place (its statics are laid out once by
  its `main()`); leaving the cart reboots the board.

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

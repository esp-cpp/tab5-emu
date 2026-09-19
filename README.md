# tab5-emu

Games on the [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5) (ESP32-P4),
built with [ESPP](https://github.com/esp-cpp/espp). Sibling of
[esp-box-emu](https://github.com/esp-cpp/esp-box-emu), with the same layout:
a launcher GUI, a `Cart` per game / core, and a thin hardware abstraction
layer around the espp board support package.

The first (and main) target is **Star Wars: Jedi Knight – Dark Forces II**,
via [OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2) (our fork:
[esp-cpp/OpenJKDF2](https://github.com/esp-cpp/OpenJKDF2)). See
[docs/PLAN.md](docs/PLAN.md) for the port plan and status.

> Status: **bring-up.** The launcher, pause menu, HAL, PPA video path and the
> full OpenJKDF2 engine (software rasterizer, ESP32 platform layer) compile
> and link for the ESP32-P4; nothing has run on hardware yet. Turn
> `CONFIG_JK_ENGINE` off in menuconfig to make the "Jedi Knight" cart show a
> test pattern instead, for checking the video / input / audio paths.

## Hardware

| | |
|---|---|
| SoC | ESP32-P4 (dual RISC-V @ 360 MHz, FPU, PPA 2D accelerator, 32 MB PSRAM) |
| Display | 5" 1280x720 MIPI-DSI (portrait-native, rotated to landscape) |
| Touch | GT911 capacitive |
| Audio | ES8388 codec + NS4150B amp, dual mics |
| Storage | microSD (SDMMC 4-bit) |
| Input | touch (virtual gamepad), BOOT button (pause menu); USB HID planned |

## Layout

```
main/               app_main, Cart base class, per-game carts
components/
  tab5-emu/         HAL (Tab5Emu): display / PPA video path, audio, SD, input
  gui/              LVGL launcher (rom list + boxart + settings)
  menu/             LVGL in-game pause menu (resume/reset/save/load/quit)
  rom_info/         metadata.csv parsing
  statistics/       frame timing
  jk/               OpenJKDF2 port: engine submodule + ESP32 platform layer + glue
docs/PLAN.md        the plan
```

## SD card layout

```
/sdcard/
  metadata.csv                # rom_path, boxart_path, name
  boxart/jk.jpg               # baseline JPEG, ~400x400
  jk/                         # your Jedi Knight install (copy from CD / GOG)
    jk.cd                     # (any marker file; metadata points here)
    episode/JK1.gob JK1CTF.gob JK1MP.gob
    resource/Res1hi.gob Res2.gob jk_.cd
    resource/video/*.SMK      # optional, cutscenes
    MUSIC/Track12.wav ...     # optional, the soundtrack: run
                              # tools/convert_music.py <game dir> on the GOG / Steam
                              # MUSIC folder (IMA ADPCM; the .ogg originals also
                              # play, but Vorbis decoding costs ~75% of a core)
    player/                   # saves / settings (created)
  saves/                      # tab5-emu save-slot screenshots
```

Example `metadata.csv` (one line per game: rom path, boxart, name; the
platform comes from the extension: `.md`/`.gen` Genesis, `.sfc`/`.smc` SNES,
`.gba` Game Boy Advance, `jk.cd` Jedi Knight):

```
jk/jk.cd, boxart/jk.jpg, Star Wars: Jedi Knight - Dark Forces II
genesis/sonic.md, boxart/sonic.jpg, Sonic the Hedgehog
snes/smw.sfc, boxart/smw.jpg, Super Mario World
gba/mk.gba, boxart/mk.jpg, Mario Kart: Super Circuit
```

A real `gba_bios.bin` next to a GBA ROM is used when present; otherwise the
open-source BIOS built into gpSP is used.

## Build

Requires ESP-IDF v6.x.

```
git clone --recurse-submodules=components/jk/OpenJKDF2 https://github.com/esp-cpp/tab5-emu
cd tab5-emu
idf.py set-target esp32p4
idf.py build flash monitor
```

(Do not recurse into the engine's own submodules; they are desktop-only
dependencies.)

## Controls (touch virtual gamepad, until USB HID lands)

- Left third: d-pad (relative to the zone center)
- Right third: A (lower right), B (lower left), X (upper right), Y (upper left)
- Bottom strip of the middle third: SELECT (left) / START (right)
- Top-right corner, or the BOOT button: pause menu

## License

MIT (this project). OpenJKDF2 is under its own permissive license; the game
data is not included and must come from your own copy of the game.

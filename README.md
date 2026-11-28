# tab5-emu

[![Release](https://img.shields.io/github/v/release/esp-cpp/tab5-emu?sort=semver)](https://github.com/esp-cpp/tab5-emu/releases)
[![Build](https://github.com/esp-cpp/tab5-emu/actions/workflows/build.yml/badge.svg)](https://github.com/esp-cpp/tab5-emu/actions/workflows/build.yml)
[![License](https://img.shields.io/github/license/esp-cpp/tab5-emu)](./LICENSE)
[![Platform: ESP32-P4](https://img.shields.io/badge/platform-ESP32--P4-red)](https://www.espressif.com/en/products/socs/esp32-p4)
[![Web console](https://img.shields.io/badge/web%20console-esp--cpp.github.io%2Ftab5--emu-blue)](https://esp-cpp.github.io/tab5-emu/)

> Retro games and native PC / PlayStation ports on the
> [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), an ESP32-P4
> handheld with a 5" 1280x720 touch screen. Emulators from the NES to the
> Game Boy Advance, plus Metal Gear Solid, The Phantom Menace and Jedi
> Knight running natively, all from one launcher on a microSD card.

<p align="center">
  <img src="docs/images/launcher.gif" alt="The tab5-emu launcher: scrolling down the NES games, each with its boxart and save count, then right through the consoles (Super Nintendo, Game Boy, Game Boy Color), each in its own colors" width="80%">
</p>

> [!TIP]
> **Sister project: [esp-box-emu](https://github.com/esp-cpp/esp-box-emu)**,
> the same launcher, cores and web console on a 3D-printed Game Boy Color
> style handheld built around the ESP32-S3-BOX. The two share their
> emulator cores, web console and USB link as submodules, so a fix in one
> reaches the other. See [The esp-cpp handhelds](#the-esp-cpp-handhelds).

**Contents**

- [Highlights](#highlights)
- [What it plays](#what-it-plays)
- [Native ports](#native-ports): [Metal Gear Solid](#metal-gear-solid-natively), [The Phantom Menace](#star-wars-episode-i-the-phantom-menace-recompiled), [Jedi Knight](#star-wars-jedi-knight--dark-forces-ii)
- [Gallery](#gallery)
- [The launcher](#the-launcher)
- [Web console](#web-console)
- [Hardware](#hardware)
- [Get started](#get-started): [SD card](#sd-card-layout), [build](#build)
- [Controls](#controls)
- [Development](#development)
- [The esp-cpp handhelds](#the-esp-cpp-handhelds)
- [License](#license)

## Highlights

- 🎮 **One firmware, every system**: NES, Super Nintendo,
  Game Boy / Color, Game Boy Advance, Master System, Game Gear, Genesis,
  MSX, PICO-8, Doom, Dark Forces, ScummVM (Star Trek: 25th Anniversary),
  plus the three native ports below. Each game picks its core from the ROM's
  file type, and games start from the launcher without a reboot.
- 🧠 **Native PlayStation and PC games**: Metal Gear Solid from its
  decompilation, The Phantom Menace through a static recompiler, Jedi Knight
  through OpenJKDF2, all running as C on the P4's two 360 MHz RISC-V cores.
- 🖼️ **Launcher with boxart**: games grouped by console in each console's
  colors, save counts, a console picker and filter, a settings panel, touch
  or gamepad.
- 💾 **Save states everywhere**: the pause menu (BOOT button or the top-right
  corner) saves and loads slots with screenshots; the PlayStation games keep
  their own memory-card saves too.
- 🌐 **Web console**: plug into a computer and Chrome or Edge offers a control
  panel to watch and play the screen, manage games and boxart, browse the
  SD card, change settings and update the firmware. Nothing to install.
- 🔌 **USB host**: keyboards, mice and gamepads on the USB-A port, through
  hubs, decoded from their HID report descriptors.
- 📁 **SD card as a USB drive**: copy ROMs and saves without pulling the card.
- 🔁 **Safe updates over USB**: `emulink ota` or the web console, with an
  updater app to roll back to.

## What it plays

| System | Core | Status |
|---|---|---|
| Metal Gear Solid (PlayStation) | [mgs_reversing](https://github.com/FoxdieTeam/mgs_reversing) + [psyz](https://github.com/Xeeynamo/psyz), native | ~28 fps against the game's 30 fps cap |
| Star Wars Episode I: The Phantom Menace (PlayStation) | [tpm-decomp](https://github.com/esp-cpp/tpm-decomp), statically recompiled | ~27 fps in game, movies at their full 15 fps |
| Star Wars: Jedi Knight – Dark Forces II (PC) | [OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2), native | playable with sound, music and cutscenes |
| Star Wars: Jedi Knight – Mysteries of the Sith (PC) | OpenJKDF2 (its MotS mode), native | every level loads and plays, with sound, music and cutscenes |
| Super Nintendo | snes9x2005 | full speed |
| Game Boy Advance | gpSP | close to full speed; heavy scenes drop a few frames |
| Genesis / Mega Drive | gwenesis ¹ | full speed |
| NES | nofrendo ¹ | full speed (~3 ms a frame) |
| Game Boy / Color | gnuboy ¹ | full speed |
| Master System / Game Gear | SMS Plus ¹ | full speed |
| MSX | fMSX ¹ | runs (Metal Gear) |
| PICO-8 | femto8 ¹ | `.p8` and `.p8.png` carts |
| Doom | PrBoom ¹ | ~35 fps (shareware demo) |
| Dark Forces | The Force Engine ¹ | ~50 fps in the first level |
| Star Trek: 25th Anniversary | [ScummVM](https://github.com/esp-cpp/scummvm) (startrek engine) | intro, bridge, briefings, saves |

¹ Shared with [esp-box-emu](https://github.com/esp-cpp/esp-box-emu): each
core is a submodule holding the core plus its `port/` glue, written against
`BoxEmu`, which the Tab5 aliases to `Tab5Emu`.

## Native ports

Three of the carts are not emulators. They are native ports of PC and
PlayStation games, running the games' own code as C on the two 360 MHz
RISC-V cores with a software rasterizer: Jedi Knight and Metal Gear Solid
from decompiled source, The Phantom Menace from a static recompilation of
your own copy of the game.

### Metal Gear Solid, natively

https://github.com/user-attachments/assets/24d01f38-ea30-4a68-a6e2-731dbd7e1fb0

<p>
<img src="docs/images/mgs-opening.jpg" width="32%"> <img src="docs/images/mgs-codec.jpg" width="32%"> <img src="docs/images/mgs-item-menu.jpg" width="32%">
</p>

The FoxdieTeam decompilation of the PlayStation game
([mgs_reversing](https://github.com/FoxdieTeam/mgs_reversing)) compiled for
the ESP32-P4, with [psyz](https://github.com/Xeeynamo/psyz) standing in for
the PSY-Q SDK, the GPU, the GTE and the SPU. Both on `esp32p4` branches of
esp-cpp forks. It boots the retail chain from the extracted US disc on the
card: the title, the opening, the dock, the elevator, topside; cutscenes
with the frame-buffer effects, codec calls, sound effects, music and
speech. Gameplay runs at about 28 fps against the game's own 30 fps cap,
drawn by two rasterizer workers, one per core, each taking a band of rows.
Save and load states from the pause menu. See
[components/mgs/README.md](components/mgs/README.md).

### Star Wars Episode I: The Phantom Menace, recompiled

https://github.com/user-attachments/assets/a1001e13-553d-4048-aa23-dba5409316e3

<p>
<img src="docs/images/tpm-conference.jpg" width="32%"> <img src="docs/images/tpm-corridor.jpg" width="32%"> <img src="docs/images/tpm-pause.jpg" width="32%">
</p>

The 1999 PlayStation game, USA Rev 1, through
[tpm-decomp](https://github.com/esp-cpp/tpm-decomp): its static recompiler
turns the game's executable into C at build time, and its runtime is the
PlayStation around that C (BIOS, interrupts, DMA, CD-ROM, MDEC, GPU ports)
on the same psyz rasterizer, GTE and SPU as the Metal Gear Solid port. It
boots from the disc image on the card through the title, the full-motion
movies (MDEC video with XA audio at the movies' full 15 fps) and the levels,
at about 27 fps in game with sound effects, music and speech. Save and
load states from the pause menu.

**Recompiled, not (yet) decompiled.** Unlike the other two ports, TPM does not
run from decompiled source. `tpm-decomp/tools/recomp.py` translates every
MIPS instruction of the executable into C, mechanically. That C is
effectively Sony's executable in another form, so it is never committed or
distributed. It is generated in the build directory from **your own** copy
of the game (`CONFIG_TPM_DISC_PATH`, see [Build](#build)), so a playable TPM
needs a local build. The firmware from CI, built without the executable,
carries only the cart's load-screen viewer.

The goal is a real, matching decompilation: hand-written C that compiles back
to the byte-identical executable, as mgs_reversing is for Metal Gear Solid.
tpm-decomp has started one (the executable is split and rebuilds
byte-identical, and the first functions are in C), and decompiled functions
can replace recompiled ones one at a time. Once the decompilation is
complete, TPM would build from source like the MGS port, with only the game
data on the card. Contributions are very welcome. See
[components/tpm/README.md](components/tpm/README.md).

### Star Wars: Jedi Knight – Dark Forces II

https://github.com/user-attachments/assets/2e6b5173-f53a-4591-8347-96aa53e222f2

<p>
<img src="docs/images/jk-corridor.jpg" width="32%"> <img src="docs/images/jk-explosion.jpg" width="32%"> <img src="docs/images/jk-catwalk.jpg" width="32%">
</p>

The 1997 PC game through [OpenJKDF2](https://github.com/shinyquagsire23/OpenJKDF2)
(our fork: [esp-cpp/OpenJKDF2](https://github.com/esp-cpp/OpenJKDF2),
branch `esp32p4`): the engine's software rasterizer draws the world at a
low internal resolution and the P4's 2D accelerator scales it to the panel,
with the HUD composited at full resolution. Playable on hardware with
sound, music, the full-motion cutscenes (Smacker video and audio at the
movies' full 15 fps; Start skips), saves, and a USB keyboard and mouse or a
gamepad. The port plan and its history are in [docs/PLAN.md](docs/PLAN.md).

Its 1998 expansion, Mysteries of the Sith, runs on the same port: a
metadata entry that points at a `jkmots.cd` marker starts the engine in
OpenJKDF2's MotS mode (`-motsCompat`), with the expansion's SMUSH (`.SAN`)
cutscenes. Install it in its own directory (below), not in Jedi Knight's.

## Gallery

Captured on the Tab5 over the USB link (`emulink screenshot`), at the
games' own resolution, scaled up.

| | | |
|:---:|:---:|:---:|
| <img src="docs/images/snes-dkc.png" alt="Donkey Kong Country on the Super Nintendo core" width="100%"><br>**Super Nintendo**: Donkey Kong Country | <img src="docs/images/snes-smw.png" alt="Super Mario World's title screen" width="100%"><br>**Super Nintendo**: Super Mario World | <img src="docs/images/snes-chrono-trigger.png" alt="Chrono Trigger's opening" width="100%"><br>**Super Nintendo**: Chrono Trigger |
| <img src="docs/images/gba-minish-cap.png" alt="The Minish Cap's title screen on the Game Boy Advance core" width="100%"><br>**Game Boy Advance**: The Minish Cap | <img src="docs/images/gba-metroid-fusion.png" alt="Metroid Fusion's title screen" width="100%"><br>**Game Boy Advance**: Metroid Fusion | <img src="docs/images/genesis-sonic3.png" alt="Sonic 3's opening on the Genesis core" width="100%"><br>**Genesis**: Sonic the Hedgehog 3 |
| <img src="docs/images/nes-smb3.png" alt="Super Mario Bros. 3's world map on the NES core" width="100%"><br>**NES**: Super Mario Bros. 3 | <img src="docs/images/gbc-links-awakening.png" alt="Link's Awakening DX's opening on the Game Boy Color core" width="100%"><br>**Game Boy Color**: Link's Awakening DX | <img src="docs/images/gg-streets-of-rage-2.png" alt="Streets of Rage II's title screen on the Game Gear core" width="100%"><br>**Game Gear**: Streets of Rage II |
| <img src="docs/images/doom.png" alt="Doom's first level" width="100%"><br>**Doom** (PrBoom) | <img src="docs/images/pico8-celeste.png" alt="Celeste's title screen on the PICO-8 core" width="100%"><br>**PICO-8**: Celeste | <img src="docs/images/gba-title.jpg" alt="A Game Boy Advance game on the Tab5's screen" width="100%"><br>**Game Boy Advance** on the panel |

https://github.com/user-attachments/assets/641a64b0-b3af-429b-8582-a38fc9a5cdad

## The launcher

<p align="center">
  <img src="docs/images/launcher.jpg" alt="The launcher: console tabs along the top, the NES games in a list with their save counts, Super Mario Bros. 3 selected with its boxart, its name and a Play button" width="32%">
  <img src="docs/images/launcher-picker.jpg" alt="The console picker: a tile per console in its colors with its game count, over the dimmed launcher" width="32%">
  <img src="docs/images/launcher-settings.jpg" alt="The settings panel slid in from the right: volume, mute, brightness, video scaling, resume last save and USB drive" width="32%">
</p>

- **Games by console**: the list is grouped by console, and each console
  has its own colors and background. The consoles are tabs along the top;
  left / right on the list jump to the previous / next console.
- **Save counts**: each game shows how many save states it has.
- **Console picker**: tap a console tab (or X on a gamepad, F on a keyboard)
  for a grid of the consoles, to show only one console's games or all of
  them again; left / right then move the filter to the next console.
  `emulink filter <console>` does the same over USB.
- **Boxart and names** come from `metadata.csv` on the card (see
  [SD card layout](#sd-card-layout)). Per-console art can go in
  `boxart/consoles/<key>.jpg`.
- **Settings** (B, or the gear) slide in from the right: volume, mute,
  brightness, the SD card as a USB drive, video scaling (original, fit,
  fill) and resume last save.
- **Pause menu** in game (BOOT button, the top-right corner, select+start on
  a gamepad): resume, save and load slots with screenshots, reset, quit,
  and the same settings.

## Web console

Plug the Tab5 into a computer and it brings its own control panel. Chrome or
Edge pops up a notification for
**[esp-cpp.github.io/tab5-emu](https://esp-cpp.github.io/tab5-emu/)**, the
firmware's WebUSB landing page: click it, pick the Tab5, and the page talks
to the device straight from the browser. There's nothing to install and no
server in between.

<p>
<img src="docs/images/web-console-screen.jpg" alt="Web console Screen tab: a capture of Donkey Kong Country next to an on-screen controller and save-state buttons" width="49%"> <img src="docs/images/web-console-library.jpg" alt="Web console Library tab: Super Nintendo games with boxart, console filters and Play / Remove buttons" width="49%">
</p>

- 📊 **Dashboard**: what's running (with its boxart), live frame-rate and
  memory graphs, the firmware and partition, the last reset reason, and a
  button that hands the SD card to the PC as a USB drive.
- 🖥️ **Screen**: capture the game's or the launcher's screen (or keep it
  **Live**), save it as a PNG, play with the on-screen controller or the
  keyboard (arrows, Z / X / A / S, Q / W for L / R, Enter = Start), save and
  load states, quit.
- 📚 **Library**: the games by console with their boxart and counts. Search,
  filter (the launcher too, if you like), **Play** any of them, or **Add a
  game**: choose the ROM and its cover and the page uploads both and adds the
  `metadata.csv` line. **Remove** undoes it.
- 📁 **Files**: browse the SD card, download, upload by drag and drop, new
  folder, delete.
- ⚙️ **Settings**: volume, mute, brightness, video scaling and resume last
  save, shown in the launcher at once and saved to the card a moment later.
- 🔧 **System**: chip, memory, ESP-IDF, OTA state and the services on the
  link; restart (or into download mode), and **Update firmware**, which
  restarts into the updater and opens the espp OTA console for
  `build/tab5-emu.bin`.
- 📜 **Console**: the log over Web Serial (pick the console's port),
  filtered, followed, saved to a file.

<p>
<img src="docs/images/web-console-dashboard.jpg" alt="Web console Dashboard: Donkey Kong Country running, with performance, memory, device and SD card panels" width="32%"> <img src="docs/images/web-console-settings.jpg" alt="Web console Settings tab: volume, mute, brightness, video scaling and resume last save" width="32%"> <img src="docs/images/web-console-system.jpg" alt="Web console System tab: ESP32-P4 details, firmware update and restart" width="32%">
</p>

**How it works:** the composite USB device has a vendor (WebUSB) interface
that carries the same link as `tools/emulink`, and its descriptor names the
landing page, which is how the browser knows where to go. The page is plain
HTML, CSS and ES modules in [`web/`](./web) (shared with esp-box-emu, no
build step), published to GitHub Pages from `main`. One program owns the
link at a time, so close `emulink` before opening the page and the other way
round. To work on the page, `python3 -m http.server -d web 8000` and open
http://localhost:8000 (WebUSB needs https or localhost);
[`web/README.md`](./web/README.md) has the details.

## Hardware

No hardware to build: the [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5)
runs the firmware as it comes.

| | |
|---|---|
| SoC | ESP32-P4 (dual RISC-V @ 360 MHz, FPU, PPA 2D accelerator, 32 MB PSRAM) |
| Display | 5" 1280x720 MIPI-DSI (portrait-native, rotated to landscape) |
| Touch | GT911 capacitive |
| Audio | ES8388 codec + NS4150B amp, dual mics |
| Storage | microSD (SDMMC 4-bit) |
| Input | touch (virtual gamepad), BOOT button (pause menu), USB keyboard / mouse / gamepad on the USB-A port (hubs supported) |
| USB-C | serial console, the USB link (web console, `emulink`), and the SD card as a USB drive |

## Get started

1. Build and flash the firmware ([Build](#build)). After the first flash,
   updates can go over USB from the web console or `emulink ota`.
2. Put your games, their boxart and a `metadata.csv` on a microSD card
   ([SD card layout](#sd-card-layout)). The web console's **Library** tab can
   add games one at a time over USB.
3. Insert the card, power on and pick a game.

### SD card layout

```
/sdcard/
  metadata.csv                # rom_path, boxart_path, name
  boxart/*.jpg                # baseline JPEG, ~400x400
  boxart/consoles/<key>.jpg   # optional per-console art for the launcher
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
  jkmots/                     # Mysteries of the Sith, copied from its CD's GAMEDATA/
    jkmots.cd                 # (marker file: the engine runs in its MotS mode)
    episode/JKM.goo JKM_KFY.goo JKM_MP.goo JKM_SABER.goo
    resource/JKMRES.goo JK_.CD
    resource/JKMSNDLO.goo     # from the CD's GAMEDATA/MININSTALL/
    resource/video/*.SAN      # optional, cutscenes
    MUSIC/1/Track2.ogg ...    # optional, the CD's audio tracks 2-14; then
                              # tools/convert_music.py jkmots makes the .wav
    player/                   # saves / settings (created)
  mgs/                        # Metal Gear Solid, extracted from your own US disc
    STAGE.DIR RADIO.DAT FACE.DAT ZMOVIE.STR VOX.DAT DEMO.DAT BRF.DAT
                              # (components/mgs/mgs_reversing/port/extract_disc.py)
  tpm/                        # The Phantom Menace, USA Rev 1: the .bin of a bin/cue rip
    Star Wars - Episode I - The Phantom Menace (USA) (Rev 1).bin
  startrek/                   # Star Trek: 25th Anniversary data, plus a marker
    startrek.scummvm          # file *.scummvm (ScummVM detects the game)
  doom/                       # doom1.wad, doom.wad, doom2.wad, ... (the IWADs)
  darkforces/                 # your Dark Forces install: DARK.GOB, SOUNDS.GOB, TEXTURES.GOB, SPRITES.GOB, LFDs
  nes/ snes/ gbc/ gba/ sms/ gg/ genesis/ msx/ pico8/
                              # cartridge ROMs (any folder: metadata.csv has the paths)
  saves/                      # save states and their screenshots; the PlayStation
                              # games' memory cards (tpm_card1.mcd, mgs_card1.mcd:
                              # raw 128 KB images, as PC emulators use)
```

`metadata.csv` has one line per game: rom path, boxart, name. The platform
comes from the path: `.nes` NES, `.sfc`/`.smc` SNES, `.gb`/`.gbc` Game Boy /
Color, `.gba` Game Boy Advance, `.sms` Master System, `.gg` Game Gear,
`.md`/`.gen` Genesis, `.rom` MSX, `.p8`/`.p8.png` PICO-8, `.wad` Doom,
`DARK.GOB` Dark Forces, `*.scummvm` ScummVM, `jk.cd` Jedi Knight,
`jkmots.cd` Mysteries of the Sith, `STAGE.DIR` Metal Gear Solid,
`SLUS_008.84` The Phantom Menace.

```csv
jk/jk.cd, boxart/jk.jpg, Star Wars: Jedi Knight - Dark Forces II
jkmots/jkmots.cd, boxart/jkmots.jpg, Star Wars: Jedi Knight - Mysteries of the Sith
mgs/STAGE.DIR, boxart/mgs.jpg, Metal Gear Solid
tpm/SLUS_008.84, boxart/tpm.jpg, Star Wars Episode I: The Phantom Menace
startrek/startrek.scummvm, boxart/startrek.jpg, Star Trek: 25th Anniversary
genesis/sonic.md, boxart/sonic.jpg, Sonic the Hedgehog
snes/smw.sfc, boxart/smw.jpg, Super Mario World
gba/mk.gba, boxart/mk.jpg, Mario Kart: Super Circuit
```

No game data is included. A real `gba_bios.bin` next to a GBA ROM is used
when present; otherwise the open-source BIOS built into gpSP is used.

### Build

Requires ESP-IDF v6.x.

```sh
git clone https://github.com/esp-cpp/tab5-emu
cd tab5-emu
git submodule update --init components/emulink \
    components/jk/OpenJKDF2 components/mgs/mgs_reversing components/mgs/psyz \
    components/tpm/tpm-decomp components/genesis/gwenesis components/pico8/femto8 \
    components/nes/nofrendo components/gbc/gnuboy components/msx/fmsx \
    components/sms/smsplus components/doom/prboom components/darkforces/TheForceEngine
git submodule update --init --depth 1 components/scummvm/scummvm
idf.py set-target esp32p4
idf.py -C updater build   # once: the updater app, which goes in ota_1
idf.py build flash monitor
```

The updater (`updater/`) is what over-USB updates (`tools/emulink`) reboot
into, and what a bad update rolls back to; with it built first, `idf.py
flash` writes it along with the emulator. After that, updates can go over
USB with `emulink ota` or the web console.

The flash runs at 120 MHz, which needs this project's bootloader: over-USB
updates only replace the app, so a board whose bootloader came from an older
build (80 MHz) is flashed once with `idf.py flash`. Without a BOOT button press,
`emulink reboot --bootloader` puts the Tab5 in its download mode for that.

Don't use `--recursive`: the engines' own submodules are desktop-only
dependencies (this is the list CI checks out).

The Phantom Menace is recompiled from your own disc at build time: set
`CONFIG_TPM_DISC_PATH` to your USA Rev 1 disc image (the `.bin` of a bin/cue
rip) in `idf.py menuconfig`, under "Star Wars Episode I: The Phantom
Menace", before building. Without it the build succeeds with a warning and
the TPM cart only shows the game's load screens. See
[components/tpm/README.md](components/tpm/README.md#building).

## Controls

**Touch** (virtual gamepad):

- Left third: d-pad (relative to the zone center)
- Right third: A (lower right), B (lower left), X (upper right), Y (upper left)
- Bottom strip of the middle third: SELECT (left) / START (right)
- Top-right corner, or the BOOT button: pause menu

**USB keyboard / mouse / gamepad** on the USB-A port, directly or through a
hub (the host runs at full speed, so hubs work). Devices are decoded from
their HID report descriptors (espp `hid-rp` report map), so any standard HID
gamepad works:

- Face buttons by position (A = east, B = south, X = north, Y = west).
- Shoulders and triggers = L / R; the triggers are also reported on their
  own as L2 / R2.
- Select / start, and the Consumer "back" / "menu" controls of mobile pads.
- D-pad, hat or left stick = d-pad.
- Select+start or the home button = pause menu.
- Known layouts: Xbox-style and DirectInput / DualShock numbering, Backbone
  Pro quirks.
- Keyboards: arrows, Z / X / A / S, Enter, Shift / Backspace, Q / W (L / R)
  and E / R (L2 / R2).

<details>
<summary><strong>Per-game controls</strong></summary>

**Jedi Knight** gets the full keyboard and mouse look. On a gamepad the left
stick strafes, the right stick looks, R = fire, L = secondary fire,
A = activate, B = jump, X = crouch, Y = use item.

**Metal Gear Solid** uses the PlayStation layout by position: face buttons as
Cross / Circle / Square / Triangle, L1 / R1 on the shoulders, L2 / R2 on the
triggers for the item and weapon menus, so a pad with triggers (or the
keyboard's E / R) is needed for those.

**Star Trek: 25th Anniversary**: touch is the mouse (two fingers = right
click), as are a USB mouse and the gamepad (A / B = left / right click).

**MSX**: A / B are the joystick's two fire buttons, start is the MSX's F1 key
(pause in Konami's games), select shows the on-screen keyboard and Y opens
fMSX's own menu.

New controllers: the console prints each device's decoded buttons on change
(debug level in `usb_hid.cpp`); a wrong mapping is a quirk-table entry in
espp's `hid-rp-report-map.hpp`.
</details>

## Development

### Layout

```
main/               app_main, Cart base class, per-game carts
components/
  tab5-emu/         HAL (Tab5Emu): display / PPA video path, audio, SD, input, USB
  emu-ui/           the launcher and pause menu screens (shared with esp-box-emu)
  gui/              the launcher's Tab5 side: games, boxart, settings
  menu/             the in-game pause menu (resume/reset/save/load/quit) on emu-ui
  rom_info/         metadata.csv parsing, consoles
  statistics/       frame timing
  emulink/          the USB link (shared with esp-box-emu; tools/emulink is its CLI)
  jk/               OpenJKDF2 port: engine submodule + ESP32 platform layer + glue
  mgs/              Metal Gear Solid: mgs_reversing + psyz submodules + the Tab5 platform layer
  tpm/              The Phantom Menace: tpm-decomp submodule (static recompiler + runtime) + glue
  scummvm/          ScummVM (startrek engine) with its ESP32 backend
  gba/ snes/        the Tab5-only emulator cores and their glue
  nes/ gbc/ sms/ msx/ genesis/ doom/ darkforces/ pico8/
                    the cores shared with esp-box-emu (submodules: core + port/ glue)
  pool_allocator/   the allocator MSX, Doom and Dark Forces run in their 4 MB block
web/                the web console (shared with esp-box-emu)
updater/            the updater app (ota_1)
docs/PLAN.md        the Jedi Knight plan and history
```

### emulink

`tools/emulink` drives the Tab5 over the USB link from a script: `status`,
`roms`, `launch`, `press`, `tap`, `screenshot`, `save` / `load`, `ls` /
`push` / `pull`, `add-rom`, `settings`, `filter`, `drive on|off`, `ota`,
`coredump`. It needs pyusb (in the ESP-IDF Python environment). The
screenshots in this README were taken with it.

### USB drive (file transfer)

Settings > "USB drive" exposes the SD card as a USB mass storage device on
the USB-C port, so ROMs, boxart and game data can be copied without pulling
the card. The console and the USB link stay up meanwhile (all three are
functions of one composite USB device), but nothing on the Tab5 can use the
card, so the launcher cannot start games. Ejecting the drive on the PC does
not hand it back by itself: eject it there, then switch "USB drive" off to
get the card and the updated ROM list back. `emulink drive on` / `emulink
drive off` do the same from a script (drive off ejects it first on macOS).

### Crashes

Core dumps go to flash (`coredump` partition) and are printed base64 on the
console at the next boot, so a crash while the console is unavailable (the
USB drive hand-over) can still be read; `emulink coredump` fetches the last
one over USB. Decode with:

```sh
base64 -d < dump.b64 > core.bin
python -m esp_coredump --chip esp32p4 info_corefile -t raw -c core.bin build/tab5-emu.elf
```

A terminal which opens the console port with DTR low and RTS high resets
the chip (`rst:0x17 CHIP_USB_UART_RESET`); keep both asserted, or expect a
reboot whenever the console re-enumerates (after the USB drive).

## The esp-cpp handhelds

tab5-emu and [esp-box-emu](https://github.com/esp-cpp/esp-box-emu) are one
project on two devices. Both are built on [ESPP](https://github.com/esp-cpp/espp)
and share the emulator cores, the USB link, `emulink` and the web console.

| | **tab5-emu** | **[esp-box-emu](https://github.com/esp-cpp/esp-box-emu)** |
|---|---|---|
| Device | [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), off the shelf | [ESP32-S3-BOX / BOX-3](https://github.com/espressif/esp-box) in a 3D-printed Game Boy Color style shell with a custom PCB |
| Chip | ESP32-P4, 2× RISC-V @ 360 MHz, 32 MB PSRAM | ESP32-S3, 2× Xtensa LX7 @ 240 MHz, PSRAM |
| Screen | 5" 1280x720 touch | 2.4" 320x240 touch |
| Controls | touch gamepad, USB gamepads / keyboards / mice | d-pad, A / B / X / Y, Start / Select, haptics |
| Systems | everything on the right, plus SNES, GBA, ScummVM and native MGS, TPM and Jedi Knight | NES, Game Boy / Color, Master System / Game Gear, Genesis, MSX, PICO-8, Doom, Dark Forces |
| Web console | [esp-cpp.github.io/tab5-emu](https://esp-cpp.github.io/tab5-emu/) | [esp-cpp.github.io/esp-box-emu](https://esp-cpp.github.io/esp-box-emu/) |

## License

MIT (this project). The cores, OpenJKDF2, mgs_reversing, psyz, tpm-decomp
and ScummVM are under their own licenses. No game data is included; it must
come from your own copies of the games.

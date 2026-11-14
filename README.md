# tab5-emu

Games on the [M5Stack Tab5](https://docs.m5stack.com/en/core/Tab5), an
ESP32-P4 handheld with a 5" 1280x720 display, built with
[ESPP](https://github.com/esp-cpp/espp). A launcher, a pause menu with save
states, and a `Cart` per game or emulator core, in the same shape as
[esp-box-emu](https://github.com/esp-cpp/esp-box-emu) so glue can move
between the two.

Three of the carts are not emulators. They are native ports of PC and
PlayStation games, running the games' own code as C on the two 360 MHz
RISC-V cores with a software rasterizer: Jedi Knight and Metal Gear Solid from
decompiled source, The Phantom Menace from a static recompilation of your own
copy of the game (see below).

## Metal Gear Solid, natively

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

## Star Wars Episode I: The Phantom Menace, recompiled

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
of the game (`CONFIG_TPM_DISC_PATH`, see Build below), so a playable TPM
needs a local build. The firmware from CI, built without the executable, carries only the
cart's load-screen viewer.

The goal is a real, matching decompilation: hand-written C that compiles back
to the byte-identical executable, as mgs_reversing is for Metal Gear Solid.
tpm-decomp has started one (the executable is split and rebuilds
byte-identical, and the first functions are in C), and decompiled functions
can replace recompiled ones one at a time. Once the decompilation is
complete, TPM would build from source like the MGS port, with only the game
data on the card. Contributions are very welcome. See
[components/tpm/README.md](components/tpm/README.md).

## Star Wars: Jedi Knight – Dark Forces II

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
gamepad. The port
plan and its history are in [docs/PLAN.md](docs/PLAN.md).

## Emulators

| Core | Source | Status |
|---|---|---|
| Genesis / Mega Drive | gwenesis (shared with esp-box-emu) | full speed |
| SNES | snes9x2005 | full speed |
| Game Boy Advance | gpSP | close to full speed; heavy scenes drop a few frames |

https://github.com/user-attachments/assets/641a64b0-b3af-429b-8582-a38fc9a5cdad

<p>
<img src="docs/images/gba-title.jpg" width="32%"> <img src="docs/images/gba-town.jpg" width="32%">
</p>

## Web Console

Plug the Tab5 into a computer and it brings its own control panel. Chrome or
Edge pops up a notification for
**[esp-cpp.github.io/tab5-emu](https://esp-cpp.github.io/tab5-emu/)**, the
firmware's WebUSB landing page: click it, pick the Tab5, and the page talks
to the device straight from the browser. There's nothing to install and no
server in between.

<p>
<img src="docs/images/web-console-screen.jpg" alt="Web console Screen tab: a capture of Donkey Kong Country next to an on-screen controller and save-state buttons" width="49%"> <img src="docs/images/web-console-library.jpg" alt="Web console Library tab: Super Nintendo games with boxart, console filters and Play / Remove buttons" width="49%">
</p>

- **Dashboard**: what's running (with its boxart), live frame-rate and
  memory graphs, the firmware and partition, the last reset reason, and a
  button that hands the SD card to the PC as a USB drive.
- **Screen**: capture the game's or the launcher's screen (or keep it
  **Live**), save it as a PNG, play with the on-screen controller or the
  keyboard (arrows, Z / X / A / S, Q / W for L / R, Enter = Start), save and
  load states, quit.
- **Library**: the games by console with their boxart and counts. Search,
  filter (the launcher too, if you like), **Play** any of them, or **Add a
  game**: choose the ROM and its cover and the page uploads both and adds the
  `metadata.csv` line. **Remove** undoes it.
- **Files**: browse the SD card, download, upload by drag and drop, new
  folder, delete.
- **Settings**: volume, mute, brightness, video scaling and resume last save,
  shown in the launcher at once and saved to the card a moment later.
- **System**: chip, memory, ESP-IDF, OTA state and the services on the link;
  restart (or into download mode), and **Update firmware**, which restarts
  into the updater and opens the espp OTA console for `build/tab5-emu.bin`.
- **Console**: the log over Web Serial (pick the console's port), filtered,
  followed, saved to a file.

<p>
<img src="docs/images/web-console-dashboard.jpg" alt="Web console Dashboard: Donkey Kong Country running, with performance, memory, device and SD card panels" width="32%"> <img src="docs/images/web-console-settings.jpg" alt="Web console Settings tab: volume, mute, brightness, video scaling and resume last save" width="32%"> <img src="docs/images/web-console-system.jpg" alt="Web console System tab: ESP32-P4 details, firmware update and restart" width="32%">
</p>

How it works: the composite USB device has a vendor (WebUSB) interface that
carries the same link as `tools/emulink`, and its descriptor names the
landing page, which is how the browser knows where to go. The page is plain
HTML, CSS and ES modules in [`web/`](./web) (shared with esp-box-emu, no build
step), published to GitHub Pages from `main`. One program owns the link at a
time, so close `emulink` before opening the page and the other way round. To
work on the page, `python3 -m http.server -d web 8000` and open
http://localhost:8000 (WebUSB needs https or localhost); [`web/README.md`](./web/README.md)
has the details.

## Hardware

| | |
|---|---|
| SoC | ESP32-P4 (dual RISC-V @ 360 MHz, FPU, PPA 2D accelerator, 32 MB PSRAM) |
| Display | 5" 1280x720 MIPI-DSI (portrait-native, rotated to landscape) |
| Touch | GT911 capacitive |
| Audio | ES8388 codec + NS4150B amp, dual mics |
| Storage | microSD (SDMMC 4-bit) |
| Input | touch (virtual gamepad), BOOT button (pause menu), USB keyboard / mouse / gamepad on the USB-A port (hubs supported) |
| USB-C | serial console, or the SD card as a USB drive (settings > "USB drive") |

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
  mgs/              Metal Gear Solid: mgs_reversing + psyz submodules + the Tab5 platform layer
  tpm/              The Phantom Menace: tpm-decomp submodule (static recompiler + runtime) + glue
  gba/ genesis/ snes/   the emulator cores and their glue
docs/PLAN.md        the Jedi Knight plan and history
```

## SD card layout

```
/sdcard/
  metadata.csv                # rom_path, boxart_path, name
  boxart/*.jpg                # baseline JPEG, ~400x400
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
  mgs/                        # Metal Gear Solid, extracted from your own US disc
    STAGE.DIR RADIO.DAT FACE.DAT ZMOVIE.STR VOX.DAT DEMO.DAT BRF.DAT
                              # (components/mgs/mgs_reversing/port/extract_disc.py)
  tpm/                        # The Phantom Menace, USA Rev 1: the .bin of a bin/cue rip
    Star Wars - Episode I - The Phantom Menace (USA) (Rev 1).bin
  saves/                      # save states and their screenshots; the PlayStation
                              # games' memory cards (tpm_card1.mcd, mgs_card1.mcd:
                              # raw 128 KB images, as PC emulators use)
```

Example `metadata.csv` (one line per game: rom path, boxart, name; the
platform comes from the path: `.md`/`.gen` Genesis, `.sfc`/`.smc` SNES,
`.gba` Game Boy Advance, `jk.cd` Jedi Knight, `STAGE.DIR` Metal Gear Solid,
`SLUS_008.84` The Phantom Menace):

```
jk/jk.cd, boxart/jk.jpg, Star Wars: Jedi Knight - Dark Forces II
mgs/STAGE.DIR, boxart/mgs.jpg, Metal Gear Solid
tpm/SLUS_008.84, boxart/tpm.jpg, Star Wars Episode I: The Phantom Menace
genesis/sonic.md, boxart/sonic.jpg, Sonic the Hedgehog
snes/smw.sfc, boxart/smw.jpg, Super Mario World
gba/mk.gba, boxart/mk.jpg, Mario Kart: Super Circuit
```

No game data is included. A real `gba_bios.bin` next to a GBA ROM is used
when present; otherwise the open-source BIOS built into gpSP is used.

## Build

Requires ESP-IDF v6.x.

```
git clone --recurse-submodules=components/jk/OpenJKDF2 \
          --recurse-submodules=components/mgs/mgs_reversing \
          --recurse-submodules=components/mgs/psyz \
          --recurse-submodules=components/tpm/tpm-decomp \
          https://github.com/esp-cpp/tab5-emu
cd tab5-emu
idf.py set-target esp32p4
idf.py -C updater build   # once: the updater app, which goes in ota_1
idf.py build flash monitor
```

The updater (`updater/`) is what over-USB updates (`tools/emulink`) reboot
into, and what a bad update rolls back to; with it built first, `idf.py
flash` writes it along with the emulator.

(Do not recurse into the engine's own submodules; they are desktop-only
dependencies.)

The Phantom Menace is recompiled from your own disc at build time: set
`CONFIG_TPM_DISC_PATH` to your USA Rev 1 disc image (the `.bin` of a bin/cue
rip) in `idf.py menuconfig`, under "Star Wars Episode I: The Phantom
Menace", before building. Without it the build succeeds with a warning and
the TPM cart only shows the game's load screens. See
[components/tpm/README.md](components/tpm/README.md#building).

## Controls

Touch (virtual gamepad):

- Left third: d-pad (relative to the zone center)
- Right third: A (lower right), B (lower left), X (upper right), Y (upper left)
- Bottom strip of the middle third: SELECT (left) / START (right)
- Top-right corner, or the BOOT button: pause menu

USB keyboard / mouse / gamepad on the USB-A port (directly or through a
hub; the host runs at full speed so hubs work). Devices are decoded from
their HID report descriptors (espp `hid-rp` report map), so any standard HID
gamepad works: face buttons by position (A = east, B = south, X = north,
Y = west), shoulders and triggers = L / R (the triggers are also reported on
their own as L2 / R2), select / start (also the Consumer "back" / "menu"
controls of mobile pads), d-pad / hat / left stick = d-pad, select+start or
the home button = pause menu. Known layouts: Xbox-style and DirectInput /
DualShock numbering, Backbone Pro quirks. Keyboards: arrows, Z/X/A/S, Enter,
Shift/Backspace, Q/W (L/R) and E/R (L2/R2) map to the gamepad for the
emulators.

In the launcher, left / right on the game list go to the previous / next
console's games. Tapping the console above the boxart (or X on a gamepad, F
on a keyboard) opens the console picker, which shows only one console's
games, or all of them again; an outline on the console means the list is
filtered, and left / right then move the filter to the next console.
`emulink filter <console>` (snes, gba, genesis, playstation, pc, pico8 or
all) does the same over USB.

Jedi Knight gets the full keyboard and mouse look; on a gamepad the left
stick strafes, the right stick looks, R = fire, L = secondary fire, A =
activate, B = jump, X = crouch, Y = use item.

Metal Gear Solid uses the PlayStation layout by position: face buttons as
Cross / Circle / Square / Triangle, L1 / R1 on the shoulders, L2 / R2 on the
triggers for the item and weapon menus, so a pad with triggers (or the
keyboard's E / R) is needed for those.

New controllers: the console prints each device's decoded buttons on change
(debug level in `usb_hid.cpp`); a wrong mapping is a quirk-table entry in
espp's `hid-rp-report-map.hpp`.

## Crashes

Core dumps go to flash (`coredump` partition) and are printed base64 on the
console at the next boot, so a crash while the console is unavailable (the
USB drive hand-over) can still be read. Decode with:

```
base64 -d < dump.b64 > core.bin
python -m esp_coredump --chip esp32p4 info_corefile -t raw -c core.bin build/tab5-emu.elf
```

Note that a terminal which opens the console port with DTR low and RTS high
resets the chip (`rst:0x17 CHIP_USB_UART_RESET`); keep both asserted, or
expect a reboot whenever the console re-enumerates (after the USB drive).

## USB drive (file transfer)

Settings > "USB drive" exposes the SD card as a USB mass storage device on
the USB-C port, so ROMs, boxart and game data can be copied without pulling
the card. The console and the USB link stay up meanwhile (all three are
functions of one composite USB device), but nothing on the Tab5 can use the
card, so the launcher cannot start games. Ejecting the drive on the PC does
not hand it back by itself: eject it there, then switch "USB drive" off to
get the card and the updated ROM list back. `emulink drive on` / `emulink
drive off` do the same from a script (drive off ejects it first on macOS).

## License

MIT (this project). OpenJKDF2, mgs_reversing and psyz are under their own
licenses; the game data is not included and must come from your own copies
of the games.

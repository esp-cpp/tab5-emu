# Star Wars Episode I: The Phantom Menace, natively (in progress)

The PlayStation game (Big Ape Productions / LucasArts, 1999), USA Rev 1
(`SLUS_008.84`), being decompiled in [tpm-decomp](https://github.com/esp-cpp/tpm-decomp)
(submodule `tpm-decomp/`) so that it can run here
the way Metal Gear Solid does in `components/mgs`: the game's C compiled for
the ESP32-P4 against psyz. No decompilation existed; that repo is building
one (see its `docs/PLAN.md`).

Until the game's code runs, this component is the platform half on its own:
it opens the disc files on the SD card, resolves names through the game's
own file table (read out of the executable, as the game does), and shows the
game's load screens. That is the data path the engine will use, end to end.

## Game data

Nothing derived from the game is in this repository. Pull the files out of
**your own** disc image with tpm-decomp's script and copy them to the card:

```sh
python3 components/tpm/tpm-decomp/tools/extract_disc.py "Star Wars - Episode I - The Phantom Menace (USA) (Rev 1).bin" tpm/
```

which gives `SLUS_008.84`, `FILE.000` (35 MB, all the game's data),
`FILE.001`-`FILE.003` (streamed audio and movies, ~500 MB; not used yet) and
`ZANZIBAR.000` (padding; leave it out). Put them in `/sdcard/tpm/` and add a
metadata entry pointing at the executable:

```
tpm/SLUS_008.84,boxart/tpm.jpg,Star Wars Episode I: The Phantom Menace
```

Only the USA Rev 1 executable is accepted: the file table is read from a
fixed address in it, and the original USA release is a different link.

## Status

- The load-screen viewer: `LOGO.RGB` first, then every 4 s
  (`CONFIG_TPM_SLIDE_SECONDS`) or on left / right.
- `CONFIG_TPM_ENGINE` (off) is reserved for the decompiled game.

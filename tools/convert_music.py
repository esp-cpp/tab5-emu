#!/usr/bin/env python3
"""Convert the Jedi Knight soundtrack for tab5-emu.

Decoding Ogg Vorbis on the ESP32-P4 costs ~75% of a core, which the game can't
spare, so the Tab5 build streams the soundtrack as IMA ADPCM WAV instead
(4 bits/sample, ~4:1 vs PCM, decoded for almost free). This script converts
every MUSIC/**/Track*.ogg of a game directory to a .wav next to it using
ffmpeg; the Ogg files are left in place (the Tab5 falls back to them).

    python3 tools/convert_music.py /Volumes/SDCARD/jk          # the game dir
    python3 tools/convert_music.py ~/games/jk --rate 22050     # smaller files

Requires ffmpeg on PATH.
"""
import argparse
import pathlib
import shutil
import subprocess
import sys


def convert(src: pathlib.Path, dst: pathlib.Path, rate: int, force: bool) -> str:
    if dst.exists() and not force and dst.stat().st_mtime >= src.stat().st_mtime:
        return "up to date"
    cmd = [
        "ffmpeg", "-loglevel", "error", "-y", "-i", str(src),
        "-ar", str(rate), "-ac", "2", "-c:a", "adpcm_ima_wav",
        # a block size the Tab5 decoder likes (fits its 4096-frame chunk)
        "-block_size", "2048",
        str(dst),
    ]
    subprocess.run(cmd, check=True)
    return f"{dst.stat().st_size // 1024} KB"


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__, formatter_class=argparse.RawDescriptionHelpFormatter)
    ap.add_argument("game_dir", type=pathlib.Path, help="game directory holding MUSIC/")
    ap.add_argument("--rate", type=int, default=44100, help="output sample rate (default 44100)")
    ap.add_argument("--force", action="store_true", help="re-convert even if the .wav is newer")
    args = ap.parse_args()

    if shutil.which("ffmpeg") is None:
        print("ffmpeg not found on PATH", file=sys.stderr)
        return 1
    music = args.game_dir / "MUSIC"
    if not music.is_dir():
        music = args.game_dir / "music"
    if not music.is_dir():
        print(f"no MUSIC directory under {args.game_dir}", file=sys.stderr)
        return 1
    oggs = sorted(p for p in music.rglob("*") if p.suffix.lower() == ".ogg")
    if not oggs:
        print(f"no .ogg files under {music}", file=sys.stderr)
        return 1
    for src in oggs:
        dst = src.with_suffix(".wav")
        try:
            print(f"{src.relative_to(args.game_dir)} -> {dst.name}: {convert(src, dst, args.rate, args.force)}")
        except subprocess.CalledProcessError as e:
            print(f"{src}: ffmpeg failed ({e.returncode})", file=sys.stderr)
            return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())

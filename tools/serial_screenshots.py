#!/usr/bin/env python3
"""Turn the "TPMFB" screenshot lines of a console log into PNGs.

    tools/serial_screenshots.py session.log out_dir/

The Tab5 prints them with CONFIG_TPM_SERIAL_SCREENSHOT_SECONDS: a "begin W H
vblank" line, base64 lines of the RGB565 frame, an "end" line. Incomplete
frames (lines lost or interleaved) are skipped.
"""
import base64
import os
import struct
import sys
import zlib


def write_png(path, w, h, rgb565):
    rows = []
    for y in range(h):
        row = bytearray([0])
        for x in range(w):
            v = rgb565[y * w + x]
            r, g, b = (v >> 11) & 31, (v >> 5) & 63, v & 31
            row += bytes(((r << 3) | (r >> 2), (g << 2) | (g >> 4), (b << 3) | (b >> 2)))
        rows.append(bytes(row))

    def chunk(t, d):
        return struct.pack(">I", len(d)) + t + d + struct.pack(">I", zlib.crc32(t + d) & 0xFFFFFFFF)

    with open(path, "wb") as f:
        f.write(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", struct.pack(">IIBBBBB", w, h, 8, 2, 0, 0, 0))
                + chunk(b"IDAT", zlib.compress(b"".join(rows), 6)) + chunk(b"IEND", b""))


def main():
    log, out = sys.argv[1], sys.argv[2]
    os.makedirs(out, exist_ok=True)
    frame = None
    n = 0
    with open(log, "rb") as f:
        for raw in f:
            line = raw.decode("latin-1").strip()
            i = line.find("TPMFB ")
            if i < 0:
                continue
            body = line[i + 6:]
            if body.startswith("begin "):
                w, h, vb = (int(x) for x in body.split()[1:4])
                frame = (w, h, vb, bytearray())
            elif body == "end":
                if frame and len(frame[3]) == frame[0] * frame[1] * 2:
                    w, h, vb, data = frame
                    path = os.path.join(out, "vblank%06d.png" % vb)
                    write_png(path, w, h, struct.unpack("<%dH" % (w * h), data))
                    print(path)
                    n += 1
                frame = None
            elif frame is not None:
                try:
                    frame[3].extend(base64.b64decode(body))
                except Exception:
                    frame = None
    print("%d frames" % n, file=sys.stderr)


if __name__ == "__main__":
    main()

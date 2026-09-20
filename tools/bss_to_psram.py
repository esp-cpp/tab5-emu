#!/usr/bin/env python3
"""Rename every .bss/.bss.*/.sbss/.sbss.* input section of an archive to
.ext_ram.bss.* so ESP-IDF's linker script places them in PSRAM (the
.ext_ram.bss output section, zeroed at startup). The ESP32-P4 linker
template has no ldgen placeholder for external RAM, so this is done on the
object level instead of with a linker fragment.

usage: bss_to_psram.py <objdump> <objcopy> <archive> [--keep name,name,...]
  --keep: symbols (section suffixes) that stay in internal RAM, e.g. a
          core's hottest state
"""
import subprocess
import sys

objdump, objcopy, archive = sys.argv[1:4]
keep = set()
if len(sys.argv) > 5 and sys.argv[4] == "--keep":
    keep = set(sys.argv[5].split(","))
out = subprocess.run([objdump, "-h", archive], check=True, capture_output=True, text=True).stdout
names = set()
for line in out.splitlines():
    parts = line.split()
    if len(parts) > 2 and parts[0].isdigit():
        name = parts[1]
        if name == ".bss" or name == ".sbss" or name.startswith(".bss.") or name.startswith(".sbss."):
            if name.count(".") >= 2 and name.split(".", 2)[2] in keep:
                continue
            names.add(name)
if not names:
    sys.exit(0)
args = [objcopy]
for name in sorted(names):
    suffix = name.split(".", 2)[2] if name.count(".") >= 2 else ""
    args += ["--rename-section", f"{name}=.ext_ram.bss" + (f".{suffix}" if suffix else "")]
args.append(archive)
subprocess.run(args, check=True)
print(f"bss_to_psram: moved {len(names)} sections of {archive} to .ext_ram.bss")

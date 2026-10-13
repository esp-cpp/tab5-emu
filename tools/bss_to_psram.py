#!/usr/bin/env python3
"""Rename every .bss/.bss.*/.sbss/.sbss.* input section of an archive to
.ext_ram.bss.* so ESP-IDF's linker script places them in PSRAM (the
.ext_ram.bss output section, zeroed at startup). The ESP32-P4 linker
template has no ldgen placeholder for external RAM, so this is done on the
object level instead of with a linker fragment.

usage: bss_to_psram.py <objdump> <objcopy> <archive> [--keep name,name,...] [--data]
  --keep: symbols (section suffixes) that stay in internal RAM, e.g. a
          core's hottest state
  --data: also move the initialized data (.data/.data.*/.sdata*) to PSRAM.
          Only valid with CONFIG_SPIRAM_XIP_FROM_PSRAM: the sections are
          renamed into .rodata.*, which that configuration copies from flash
          into PSRAM at boot and maps there, so the data arrives initialized
          and relocated like any other; it is writable only with
          CONFIG_ESP_SYSTEM_MEMPROT off (the PMP marks .rodata read-only).
"""
import subprocess
import sys

objdump, objcopy, archive = sys.argv[1:4]
keep = set()
data = False
rest = sys.argv[4:]
while rest:
    if rest[0] == "--keep":
        keep = set(rest[1].split(","))
        rest = rest[2:]
    elif rest[0] == "--data":
        data = True
        rest = rest[1:]
    else:
        sys.exit(f"bss_to_psram: unknown argument {rest[0]}")
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
        elif data and (name == ".data" or name == ".sdata" or name.startswith(".data.") or name.startswith(".sdata.")):
            names.add(name)
if not names:
    sys.exit(0)
args = [objcopy]
moved_data = 0
for name in sorted(names):
    suffix = name.split(".", 2)[2] if name.count(".") >= 2 else ""
    if name.startswith(".data") or name.startswith(".sdata"):
        args += ["--rename-section", f"{name}=.rodata.psram_data" + (f".{suffix}" if suffix else "")]
        moved_data += 1
    else:
        args += ["--rename-section", f"{name}=.ext_ram.bss" + (f".{suffix}" if suffix else "")]
args.append(archive)
subprocess.run(args, check=True)
print(f"bss_to_psram: moved {len(names) - moved_data} bss sections of {archive} to .ext_ram.bss"
      + (f", {moved_data} data sections to .rodata (PSRAM-XIP)" if moved_data else ""))

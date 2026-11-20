#!/usr/bin/env python3
"""Rename every .bss/.bss.*/.sbss/.sbss.* input section of an archive to
.ext_ram.bss.* so ESP-IDF's linker script places them in PSRAM (the
.ext_ram.bss output section, zeroed at startup). The ESP32-P4 linker
template has no ldgen placeholder for external RAM, so this is done on the
object level instead of with a linker fragment.

usage: bss_to_psram.py <objdump> <objcopy> <archive> [--keep name,name,...]
                       [--data [--data-into=rodata|ext_ram[:TAG]]]
                       [--also-data=PREFIX:TAG ...] [--no-bss]
                       [--overlay=NAME [--overlay-rodata] [--overlay-keep=REGEX ...]]
                       [--psram-code]
  --keep: symbols (section suffixes) that stay in internal RAM, e.g. a
          core's hottest state
  --data: also move the initialized data (.data/.data.*/.sdata*) to PSRAM:
    --data-into=rodata (the default) renames them .rodata.psram_data.*. Only
          valid with CONFIG_SPIRAM_XIP_FROM_PSRAM: that configuration copies
          .rodata from flash into PSRAM at boot and maps it there, so the data
          arrives initialized and relocated like any other; it is writable
          only with CONFIG_ESP_SYSTEM_MEMPROT off (the PMP marks .rodata
          read-only). Executing from flash, .rodata is flash and the first
          write to it is a store access fault.
    --data-into=ext_ram[:TAG] renames them .ext_ram.data[.TAG]<old name>: the
          .ext_ram.data output section in PSRAM (components/tab5-emu/
          ext_ram_data.ld), filled at boot from the copy that
          tools/ext_ram_data_image.py stores in flash. For builds that execute
          from flash.
  --also-data=PREFIX:TAG: sections whose name starts with PREFIX (e.g. a
          core's own SECTION(".mgs.sdata")) are data too, renamed into
          .ext_ram.data.TAG<old name> (ext_ram only)
  --no-bss: leave the .bss alone (it is placed some other way, e.g. ldgen)
  --overlay=NAME: the core's code (.text/.text.*) becomes .tab5_ovl.NAME*,
          the core's member of the code overlay in PSRAM (builds that execute
          from flash; components/tab5-emu/overlay.ld): every core is linked at
          the same PSRAM address and the one a game needs is copied there when
          it launches. Its static constructors (.init_array) become
          .tab5_ovl_ctors.NAME, run on its first load instead of at boot.
          Shared template code (std::, __gnu_cxx::, fmt::, espp::) stays in
          flash: the linker keeps one copy of each for the whole firmware and
          it may be this archive's.
  --overlay-rodata: the core's named read-only data (.rodata.<symbol>: its
          tables) goes into the overlay too (merged strings and constants stay
          in flash, and so does shared template data, as with the code)
  --psram-code: all of the archive's code and read-only data (.text*,
          .rodata*) goes to .tab5_res, PSRAM that is filled from flash at
          boot and stays (components/tab5-emu/overlay.ld): for code that
          runs outside the games and needs the speed (the GUI, LVGL). Not
          for code that runs before the C++ constructors.
  --overlay-keep=REGEX: more code that stays in flash (resident), matched
          against the section name after ".text." (and after "unlikely." /
          "hot." / "startup."); repeatable
"""
import re
import tempfile
import os
import subprocess
import sys

objdump, objcopy, archive = sys.argv[1:4]
keep = set()
data = False
data_into, data_tag = "rodata", ""
also = []  # (prefix, tag)
do_bss = True
overlay = ""
overlay_rodata = False
psram_code = False
overlay_keep = [re.compile(r"^_Z(NK?)?(St|Sa|Ss|9__gnu_cxx|3fmt|4espp)")]
rest = sys.argv[4:]
while rest:
    arg = rest[0]
    if arg == "--keep":
        keep = set(rest[1].split(","))
        rest = rest[2:]
        continue
    if arg == "--data":
        data = True
    elif arg.startswith("--data-into="):
        data_into, _, data_tag = arg.split("=", 1)[1].partition(":")
        if data_into not in ("rodata", "ext_ram"):
            sys.exit(f"bss_to_psram: --data-into must be rodata or ext_ram, not {data_into}")
    elif arg.startswith("--also-data="):
        prefix, _, tag = arg.split("=", 1)[1].partition(":")
        also.append((prefix, tag))
    elif arg == "--no-bss":
        do_bss = False
    elif arg.startswith("--overlay="):
        overlay = arg.split("=", 1)[1]
    elif arg == "--overlay-rodata":
        overlay_rodata = True
    elif arg == "--psram-code":
        psram_code = True
    elif arg.startswith("--overlay-keep="):
        overlay_keep.append(re.compile(arg.split("=", 1)[1]))
    else:
        sys.exit(f"bss_to_psram: unknown argument {arg}")
    rest = rest[1:]
if also and data_into != "ext_ram":
    sys.exit("bss_to_psram: --also-data needs --data-into=ext_ram")


def is_bss(name):
    return name == ".bss" or name == ".sbss" or name.startswith(".bss.") or name.startswith(".sbss.")


def is_data(name):
    return name == ".data" or name == ".sdata" or name.startswith(".data.") or name.startswith(".sdata.")


def is_text(name):
    return name == ".text" or name.startswith(".text.")


def resident(name):
    """Code or read-only data that stays in flash although its archive is an
    overlay"""
    fn = name.split(".", 2)[2] if name.count(".") >= 2 else ""
    for sub in ("unlikely.", "hot.", "startup.", "exit."):
        if fn.startswith(sub):
            fn = fn[len(sub):]
            break
    # vtables / typeinfo (_ZTV...) and function-local statics (_ZZ...) belong
    # with the code they are part of
    if fn.startswith(("_ZTV", "_ZTI", "_ZTS", "_ZTT")):
        fn = "_Z" + fn[4:]
    elif fn.startswith("_ZZ"):
        fn = "_Z" + fn[3:]
    return any(k.search(fn) for k in overlay_keep)


MERGED = re.compile(r"\.(str\d+\.\d+|cst\d+)$")  # .rodata[.<fn>].str1.4 / .cst8


def is_named_rodata(name):
    """.rodata.<symbol>, not merged strings / constants (.rodata.str1.1,
    .rodata.cst8, and with -ffunction-sections the per-function pools
    .rodata.<fn>.str1.4 / .rodata.<fn>.cst8), which shared code in the
    archive may point into"""
    return name.startswith(".rodata.") and not MERGED.search(name)


def tagged(tag):
    return ".ext_ram.data" + (f".{tag}" if tag else "")


out = subprocess.run([objdump, "-h", archive], check=True, capture_output=True, text=True).stdout
renames = {}  # old name -> new name
for line in out.splitlines():
    parts = line.split()
    if len(parts) <= 2 or not parts[0].isdigit():
        continue
    name = parts[1]
    suffix = name.split(".", 2)[2] if name.count(".") >= 2 else ""
    if do_bss and is_bss(name):
        if suffix in keep:
            continue
        renames[name] = ".ext_ram.bss" + (f".{suffix}" if suffix else "")
    elif psram_code and (is_text(name) or name == ".rodata" or name.startswith(".rodata.")):
        renames[name] = f".tab5_res{name}"
    elif overlay and is_text(name):
        if not resident(name):
            renames[name] = f".tab5_ovl.{overlay}{name}"
    elif overlay and overlay_rodata and is_named_rodata(name):
        if not resident(name):
            renames[name] = f".tab5_ovl.{overlay}{name}"
    elif overlay and (name == ".init_array" or name.startswith(".init_array.")):
        renames[name] = f".tab5_ovl_ctors.{overlay}{name[len('.init_array'):]}"
    elif data and is_data(name):
        if data_into == "rodata":
            renames[name] = ".rodata.psram_data" + (f".{suffix}" if suffix else "")
        else:
            renames[name] = tagged(data_tag) + name
    else:
        for prefix, tag in also:
            if name.startswith(prefix):
                renames[name] = tagged(tag) + name
                break
if not renames:
    sys.exit(0)
# the options in a response file: a big core has tens of thousands of sections
with tempfile.NamedTemporaryFile("w", suffix=".rsp", delete=False) as rsp:
    for old in sorted(renames):
        rsp.write(f"--rename-section {old}={renames[old]}\n")
try:
    subprocess.run([objcopy, f"@{rsp.name}", archive], check=True)
finally:
    os.unlink(rsp.name)
moved_bss = sum(1 for n in renames.values() if n.startswith(".ext_ram.bss"))
moved_text = sum(1 for n in renames.values() if n.startswith(".tab5_ovl."))
moved_ctors = sum(1 for n in renames.values() if n.startswith(".tab5_ovl_ctors."))
moved_res = sum(1 for n in renames.values() if n.startswith(".tab5_res"))
moved_data = len(renames) - moved_bss - moved_text - moved_ctors - moved_res
where = ".rodata (PSRAM-XIP)" if data_into == "rodata" else ".ext_ram.data"
print(f"bss_to_psram: moved {moved_bss} bss sections of {archive} to .ext_ram.bss"
      + (f", {moved_data} data sections to {where}" if moved_data else "")
      + (f", {moved_text} code sections to the {overlay} overlay ({moved_ctors} constructor tables)"
         if overlay else "")
      + (f", {moved_res} code / read-only data sections to PSRAM (.tab5_res)" if moved_res else ""))

#!/usr/bin/env python3
"""Store the game cores' code overlay in flash and check that nothing else
uses it (builds that execute from flash; see components/tab5-emu/overlay.ld).

usage: overlay_image.py <app.elf> <app.map> [--report]

1. Each overlay member .tab5_ovl.<core> (linked at the shared PSRAM address)
   is copied into its slot in .flash.tab5_ovl_store (symbol
   _tab5_ovl_<core>_lma) and changed to NOBITS, so elf2image leaves the PSRAM
   sections out of the image. Running it again is harmless.
2. Checks, which fail the build: code that is not in the overlay must not
   reach into it, because only one core's code is there at a time (or none):
   - no boot-time constructor (.flash.init_array) points into the overlay;
   - resident code: every call / jump / address taken / load / store into the
     overlay range (auipc or lui followed by jalr / addi / a load or store
     on the same register) must be in
     the carts (main.cpp, which load their core before using it), the loader
     (overlay.c), or a core's own resident code that only that core uses: its
     internal-RAM code (IRAM_ATTR and the like), or code it keeps in flash
     that is local or, per the linker's cross-reference table, referenced
     only from its own archive;
   - resident data: every word that is the address of an overlay function
     (or table) must belong to a core archive (its own jump tables, vtables, callbacks).
   A finding names the resident function and the overlay code it reaches;
   the usual fix is to keep that code in flash (bss_to_psram.py
   --overlay-keep in the core's CMakeLists.txt).
   --report prints the allowed references too.
"""
import bisect
import re
import shutil
import struct
import subprocess
import sys
from array import array

SHT_PROGBITS, SHT_NOBITS = 1, 8
SHF_ALLOC, SHF_EXECINSTR = 0x2, 0x4
CORES = ("gba", "snes", "genesis", "doom", "nes", "gbc", "sms", "msx", "pico8", "jk", "darkforces", "mgs", "scummvm")
CART_FILES = ("libmain.a(main.cpp.obj)",)

elf_path, map_path = sys.argv[1], sys.argv[2]
report = "--report" in sys.argv[3:]
elf = bytearray(open(elf_path, "rb").read())
(shoff,) = struct.unpack_from("<I", elf, 0x20)
shentsize, shnum, shstrndx = struct.unpack_from("<HHH", elf, 0x2E)
secs = [list(struct.unpack_from("<10I", elf, shoff + i * shentsize)) for i in range(shnum)]


def cstr(off):
    return bytes(elf[off : elf.index(b"\0", off)]).decode()


names = [cstr(secs[shstrndx][4] + s[0]) for s in secs]
symtab = secs[names.index(".symtab")]
strtab_off = secs[symtab[6]][4]
symbols = {}  # name -> (value, size, shndx)
funcs = []  # (value, size, name, bind) of functions, for naming addresses
objects = []  # data object addresses
for off in range(symtab[4], symtab[4] + symtab[5], 16):
    st_name, value, size, info, other, shndx = struct.unpack_from("<IIIBBH", elf, off)
    name = cstr(strtab_off + st_name)
    symbols.setdefault(name, (value, size, shndx))
    if info & 0xF == 2 and size:  # STT_FUNC
        funcs.append((value, size, name, info >> 4))
    elif info & 0xF == 1 and size:  # STT_OBJECT
        objects.append(value)
funcs.sort()


def sym(name):
    if name not in symbols:
        sys.exit(f"overlay_image: {name} is not linked (is components/tab5-emu/overlay.ld in the link?)")
    return symbols[name][0]


def holder(addr):
    """the loaded section that holds addr: (index, file offset)"""
    for i, s in enumerate(secs):
        # (the store is INIT_ARRAY: it also holds the constructor tables)
        if s[1] not in (0, SHT_NOBITS) and s[2] & SHF_ALLOC and s[3] <= addr < s[3] + s[5]:
            return i, s[4] + addr - s[3]
    return None, None


ovl_start, ovl_end = sym("_tab5_ovl_start"), sym("_tab5_ovl_end")
ovl_objects = [v for v in objects if ovl_start <= v < ovl_end]

# 1. store
stored = 0
for i, name in enumerate(names):
    if not name.startswith(".tab5_ovl.") or secs[i][1] != SHT_PROGBITS:
        continue
    core = name[len(".tab5_ovl.") :]
    size = secs[i][5]
    if size:
        lma = sym(f"_tab5_ovl_{core}_lma")
        hi, foff = holder(lma)
        if hi is None or names[hi] != ".flash.tab5_ovl_store" or lma + size > secs[hi][3] + secs[hi][5]:
            sys.exit(f"overlay_image: the {core} slot at {lma:#x} is not inside .flash.tab5_ovl_store")
        elf[foff : foff + size] = elf[secs[i][4] : secs[i][4] + size]
        stored += size
    struct.pack_into("<I", elf, shoff + i * shentsize + 4, SHT_NOBITS)
    secs[i][1] = SHT_NOBITS
if stored:
    open(elf_path, "wb").write(elf)
    print(f"overlay_image: {stored // 1024} KB of core code stored in flash; overlay {ovl_start:#x}-{ovl_end:#x} "
          f"({(ovl_end - ovl_start) // 1024} KB of PSRAM)")

# input sections from the map: (start, size, file, section name), and its
# cross-reference table: demangled symbol -> the files that reference it
inputs, refs = [], {}
lines = open(map_path, errors="replace").read().split("\n")
start = next(i for i, l in enumerate(lines) if l.startswith("Linker script and memory map"))
pending = None
row = re.compile(r"^ (\S+)?\s+0x([0-9a-f]+)\s+0x([0-9a-f]+) (\S.*)$")
for k in range(start, len(lines)):
    line = lines[k]
    if line.startswith("Cross Reference Table"):
        cref = k
        break
    m = row.match(line)
    if m:
        sec = m.group(1) or pending
        addr, size = int(m.group(2), 16), int(m.group(3), 16)
        if size and addr:
            inputs.append((addr, size, m.group(4).strip(), sec or ""))
        pending = None
    elif line.startswith(" ") and not line.startswith("  ") and len(line.split()) == 1:
        pending = line.strip()
    else:
        pending = None
else:
    cref = len(lines)
current = None
for line in lines[cref + 3 :]:
    if not line.strip():
        continue
    if line[0] != " ":
        current, _, file = line.rstrip().rpartition(" ")
        current = current.strip()
        refs[current] = []  # the first file defines it
    elif current is not None:
        refs[current].append(line.strip())
inputs.sort()
input_starts = [x[0] for x in inputs]


def attribute(addr):
    """(file, input section) of a resident address (the overlay members overlap: not for those)"""
    j = bisect.bisect_right(input_starts, addr) - 1
    if j >= 0 and inputs[j][0] <= addr < inputs[j][0] + inputs[j][1]:
        return inputs[j][2], inputs[j][3]
    return "?", "?"


resident_funcs = [f for f in funcs if not ovl_start <= f[0] < ovl_end]
resident_starts = [f[0] for f in resident_funcs]


def func_at(addr):
    """the resident function at addr (they do not overlap)"""
    j = bisect.bisect_right(resident_starts, addr) - 1
    if j >= 0 and addr < resident_funcs[j][0] + resident_funcs[j][1]:
        return resident_funcs[j]
    return None


ovl_funcs = [f for f in funcs if ovl_start <= f[0] < ovl_end]
# where an overlay function or table (with --overlay-rodata) starts
ovl_func_starts = {f[0] for f in ovl_funcs} | {v for v in ovl_objects}


def overlay_names(addr):
    """the overlay symbols at addr (one per core whose code has a function there)"""
    return sorted({n for v, s, n, b in ovl_funcs if v <= addr < v + s})[:4]


def core_of(file):
    for c in CORES:
        if f"/lib{c}.a(" in file or file.startswith(f"lib{c}.a("):
            return c
    return None


demangled = {}


def demangle(names_):
    tool = shutil.which("riscv32-esp-elf-c++filt") or shutil.which("c++filt")
    todo = [n for n in names_ if n not in demangled]
    if not todo:
        return
    out = subprocess.run([tool], input="\n".join(todo), capture_output=True, text=True).stdout.split("\n") if tool else todo
    for n, d in zip(todo, out):
        demangled[n] = d


problems, allowed = [], []
in_ovl = lambda a: ovl_start <= a < ovl_end

# 2a. boot-time constructors
ia = names.index(".flash.init_array") if ".flash.init_array" in names else None
if ia is not None:
    s = secs[ia]
    for (w,) in struct.iter_unpack("<I", elf[s[4] : s[4] + s[5]]):
        if in_ovl(w):
            problems.append(f"boot constructor {w:#x} ({', '.join(overlay_names(w))}) is in the overlay")

# 2b. resident code: auipc / lui, then jalr / addi on that register within 4 instructions
hits = []  # (instruction address, target)
for i, s in enumerate(secs):
    if s[1] != SHT_PROGBITS or not (s[2] & SHF_EXECINSTR) or not (s[2] & SHF_ALLOC) or in_ovl(s[3]):
        continue
    hw = array("H")
    hw.frombytes(bytes(elf[s[4] : s[4] + (s[5] & ~1)]))
    base, n, k = s[3], len(hw), 0
    recent = {}
    while k < n - 1:
        h = hw[k]
        if h & 3 != 3:
            k += 1
            continue
        ins = h | (hw[k + 1] << 16)
        op = ins & 0x7F
        if op == 0x17 or op == 0x37:  # auipc / lui
            addr = base + 2 * k
            recent[(ins >> 7) & 31] = (((addr if op == 0x17 else 0) + (ins & 0xFFFFF000)) & 0xFFFFFFFF, addr)
        elif ((op == 0x67 or op == 0x13) and not (ins >> 12) & 7) or op == 0x03 or op == 0x07:
            # jalr / addi / loads (integer, FP): imm[11:0] at bit 20
            r = recent.pop((ins >> 15) & 31, None)
            if r:
                addr = base + 2 * k
                imm = ins >> 20
                target = (r[0] + (imm - 4096 if imm & 2048 else imm)) & 0xFFFFFFFF
                if addr - r[1] <= 16 and ovl_start <= target < ovl_end:
                    hits.append((r[1], target))
        elif op == 0x23 or op == 0x27:  # stores: imm split
            r = recent.get((ins >> 15) & 31)
            if r:
                addr = base + 2 * k
                imm = ((ins >> 25) << 5) | ((ins >> 7) & 31)
                target = (r[0] + (imm - 4096 if imm & 2048 else imm)) & 0xFFFFFFFF
                if addr - r[1] <= 16 and ovl_start <= target < ovl_end:
                    hits.append((r[1], target))
        k += 2

callers = {}
for at, target in hits:
    callers.setdefault(func_at(at), []).append((at, target))
demangle([f[2] for f in callers if f])
for f, targets in callers.items():
    at, target = targets[0]
    file, sec = attribute(at)
    core = core_of(file)
    name = f[2] if f else f"{at:#x}"
    what = (f"code: {name} [{file} {sec}] -> {target:#x} ({' / '.join(overlay_names(target))})"
            + (f" and {len(targets) - 1} more" if len(targets) > 1 else ""))
    if file.endswith(CART_FILES) or file.endswith("(overlay.c.obj)"):
        ok = True
    elif core is None:
        ok = False
    elif not sec.startswith(".text"):
        ok = True  # the core's code in internal RAM
    elif f is None or f[3] == 0:
        ok = True  # local: only its own object (the core's) calls it
    else:
        users = refs.get(demangled.get(f[2], f[2]))
        ok = users is not None and all(core_of(u) == core for u in users)
        if not ok:
            what += f"; used by {', '.join(sorted({u for u in (users or ['?']) if core_of(u) != core})[:3])}"
    (allowed if ok else problems).append(what)

# 2c. resident data: addresses of overlay functions (the PSRAM data's
# initial copy in flash is attributed to its .ext_ram.data address)
img = symbols.get("tab5_ext_ram_data_image")
ext_start = symbols.get("_ext_ram_data_start", (0,))[0]
for i, s in enumerate(secs):
    if (s[1] == SHT_NOBITS or s[1] == 0 or s[2] & SHF_EXECINSTR or not (s[2] & SHF_ALLOC) or in_ovl(s[3])
            or names[i] == ".flash.tab5_ovl_store"):
        continue
    words = array("I")
    words.frombytes(bytes(elf[s[4] : s[4] + (s[5] & ~3)]))
    for o in [o for o, w in enumerate(words) if w in ovl_func_starts]:
        w = words[o]
        addr = s[3] + 4 * o
        if img and img[0] <= addr < img[0] + img[1]:
            addr = ext_start + addr - img[0]
        file, sec = attribute(addr)
        if core_of(file) and not report:
            allowed.append(None)  # (named only when printed: naming is slow)
            continue
        what = f"data: {sec} [{file}] @{addr:#x} -> {w:#x} ({' / '.join(overlay_names(w))})"
        (allowed if core_of(file) else problems).append(what)

if report:
    for a in allowed:
        print("overlay_image: allowed", a)
if problems:
    for p in sorted(set(problems)):
        print("overlay_image: ERROR", p)
    sys.exit(f"overlay_image: {len(set(problems))} references into the core overlay from code that is not in it "
             "(see the script's docstring)")
print(f"overlay_image: checked: {len(allowed)} functions / tables reach into the overlay, all the carts', "
      "the loader's or the cores' own")

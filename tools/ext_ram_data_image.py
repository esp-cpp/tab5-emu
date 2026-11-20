#!/usr/bin/env python3
"""Store the PSRAM section .ext_ram.data in flash (builds that execute from
flash; see components/tab5-emu/ext_ram_data.ld).

usage: ext_ram_data_image.py <app.elf>

The linker placed .ext_ram.data (the cores' initialised data) in PSRAM and
resolved every reference to it there, but the bootloader can only load an
image's flash and internal-RAM segments. This copies the section's linked,
relocated bytes into the flash array tab5_ext_ram_data_image
(components/tab5-emu/src/ext_ram_data.c copies them into PSRAM before the
constructors run) and changes the section to NOBITS, so elf2image leaves it
out of the image. Run on the ELF after linking and before elf2image; running
it again is harmless (a NOBITS section is left alone).
"""
import struct
import sys

SHT_NOBITS = 8
path = sys.argv[1]
elf = bytearray(open(path, "rb").read())
if elf[:4] != b"\x7fELF" or elf[4] != 1 or elf[5] != 1:
    sys.exit(f"ext_ram_data_image: {path} is not a little-endian ELF32")
(shoff,) = struct.unpack_from("<I", elf, 0x20)
shentsize, shnum, shstrndx = struct.unpack_from("<HHH", elf, 0x2E)


def section(i):
    # name, type, flags, addr, offset, size, link, info, addralign, entsize
    return list(struct.unpack_from("<10I", elf, shoff + i * shentsize))


sections = [section(i) for i in range(shnum)]
shstr_off = sections[shstrndx][4]


def cstr(off):
    return bytes(elf[off : elf.index(b"\0", off)]).decode()


names = [cstr(shstr_off + s[0]) for s in sections]
if ".ext_ram.data" not in names:
    sys.exit("ext_ram_data_image: no .ext_ram.data section (is ext_ram_data.ld linked?)")
di = names.index(".ext_ram.data")
d = sections[di]
if d[1] == SHT_NOBITS:
    print("ext_ram_data_image: .ext_ram.data is already in flash")
    sys.exit(0)
data = bytes(elf[d[4] : d[4] + d[5]])

symtab = sections[names.index(".symtab")]
strtab_off = sections[symtab[6]][4]
image = None
for off in range(symtab[4], symtab[4] + symtab[5], 16):
    st_name, st_value, st_size, st_info, st_other, st_shndx = struct.unpack_from("<IIIBBH", elf, off)
    if cstr(strtab_off + st_name) == "tab5_ext_ram_data_image":
        image = (st_value, st_size, st_shndx)
        break
if image is None:
    sys.exit("ext_ram_data_image: tab5_ext_ram_data_image is not linked (components/tab5-emu/src/ext_ram_data.c)")
value, size, shndx = image
if len(data) > size:
    sys.exit(f"ext_ram_data_image: .ext_ram.data is {len(data)} bytes, more than the {size} bytes of "
             "tab5_ext_ram_data_image: raise EXT_RAM_DATA_IMAGE_BYTES in components/tab5-emu/src/ext_ram_data.c")
holder = sections[shndx]
if holder[1] == SHT_NOBITS or not (holder[3] <= value and value + size <= holder[3] + holder[5]):
    sys.exit(f"ext_ram_data_image: tab5_ext_ram_data_image is not in a loaded section ({names[shndx]})")
file_off = holder[4] + (value - holder[3])
elf[file_off : file_off + len(data)] = data
struct.pack_into("<I", elf, shoff + di * shentsize + 4, SHT_NOBITS)
open(path, "wb").write(elf)
print(f"ext_ram_data_image: {len(data)} bytes of .ext_ram.data (PSRAM {d[3]:#x}) stored in flash at {value:#x} "
      f"({names[shndx]}, {size - len(data)} bytes to spare)")

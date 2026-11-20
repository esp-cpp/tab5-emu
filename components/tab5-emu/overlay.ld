/* The game cores' code overlay, for builds that execute from flash (no
 * CONFIG_SPIRAM_XIP_FROM_PSRAM): code from flash is slow, and a copy of all
 * of it in PSRAM (what XIP from PSRAM does) costs over 12 MB. Only one game
 * runs at a time, so every core's code is linked at the same PSRAM address
 * and the one a game needs is copied there when it launches
 * (src/overlay.c, tab5_overlay_load()); the PSRAM this costs is the largest
 * core's code.
 *
 * tools/bss_to_psram.py --overlay=<core> renames a core archive's code
 * (.text*) to .tab5_ovl.<core>* and its static constructors (.init_array) to
 * .tab5_ovl_ctors.<core>, which run on the core's first load instead of at
 * boot. Shared template code stays in flash (see the script).
 *
 * Linked bytes: the bootloader can only load an image's flash and internal
 * RAM segments, so tools/overlay_image.py (after linking) copies each
 * overlay member's (and .tab5_res's) linked, relocated bytes into its slot of
 * .flash.tab5_ovl_store (below,
 * part of the image's rodata segment, so the boot-time SHA-256 covers it) and
 * marks the member NOBITS, and checks that nothing outside a core calls into
 * the overlay.
 *
 * NOCROSSREFS: a link error if one core's code references another's. The
 * OVERLAY must be the last allocated output section in ext_ram_seg (a
 * section after it would get its load address moved past the members). */
SECTIONS
{
  /* Code that is not a game core's but needs PSRAM's speed (LVGL and the
   * GUI, which draw in software: from flash the launcher redraws visibly).
   * bss_to_psram.py --psram-code renames it .tab5_res*; it is filled from its
   * flash slot at boot (src/ext_ram_data.c, before the other constructors)
   * and stays. */
  .tab5_res ALIGN(128) :
  {
    _tab5_res_start = ABSOLUTE(.);
    *(.tab5_res .tab5_res.*)
    . = ALIGN(128);
    _tab5_res_end = ABSOLUTE(.);
  } > ext_ram_seg

  OVERLAY ALIGN(128) : NOCROSSREFS
  {
    .tab5_ovl.gba        { *(.tab5_ovl.gba.*)        . = ALIGN(128); }
    .tab5_ovl.snes       { *(.tab5_ovl.snes.*)       . = ALIGN(128); }
    .tab5_ovl.genesis    { *(.tab5_ovl.genesis.*)    . = ALIGN(128); }
    .tab5_ovl.doom       { *(.tab5_ovl.doom.*)       . = ALIGN(128); }
    .tab5_ovl.nes        { *(.tab5_ovl.nes.*)        . = ALIGN(128); }
    .tab5_ovl.gbc        { *(.tab5_ovl.gbc.*)        . = ALIGN(128); }
    .tab5_ovl.sms        { *(.tab5_ovl.sms.*)        . = ALIGN(128); }
    .tab5_ovl.msx        { *(.tab5_ovl.msx.*)        . = ALIGN(128); }
    .tab5_ovl.pico8      { *(.tab5_ovl.pico8.*)      . = ALIGN(128); }
    .tab5_ovl.jk         { *(.tab5_ovl.jk.*)         . = ALIGN(128); }
    .tab5_ovl.darkforces { *(.tab5_ovl.darkforces.*) . = ALIGN(128); }
    .tab5_ovl.mgs        { *(.tab5_ovl.mgs.*)        . = ALIGN(128); }
    .tab5_ovl.scummvm    { *(.tab5_ovl.scummvm.*)    . = ALIGN(128); }
  } > ext_ram_seg
  _tab5_ovl_start = ADDR(.tab5_ovl.gba);
  /* after an OVERLAY, . is its start plus its largest member */
  _tab5_ovl_end = ALIGN(., 128);

  /* Flash: each core's constructor table, then a slot per core for its code
   * (filled after linking). Directly after .flash.tdata, in the same rodata
   * segment of the image (.flash.tbss does not advance the location). The
   * BYTE() makes the section PROGBITS; it must not be aligned itself (the
   * gap would break the assert below). */
  .flash.tab5_ovl_store :
  {
    _tab5_ovl_store_start = ABSOLUTE(.);
    BYTE(0x4f)
    . = ALIGN(16); _tab5_res_lma = ABSOLUTE(.); . += SIZEOF(.tab5_res);
    . = ALIGN(4); _tab5_ovl_ctors_gba_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.gba .tab5_ovl_ctors.gba.*)) _tab5_ovl_ctors_gba_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_snes_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.snes .tab5_ovl_ctors.snes.*)) _tab5_ovl_ctors_snes_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_genesis_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.genesis .tab5_ovl_ctors.genesis.*)) _tab5_ovl_ctors_genesis_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_doom_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.doom .tab5_ovl_ctors.doom.*)) _tab5_ovl_ctors_doom_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_nes_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.nes .tab5_ovl_ctors.nes.*)) _tab5_ovl_ctors_nes_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_gbc_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.gbc .tab5_ovl_ctors.gbc.*)) _tab5_ovl_ctors_gbc_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_sms_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.sms .tab5_ovl_ctors.sms.*)) _tab5_ovl_ctors_sms_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_msx_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.msx .tab5_ovl_ctors.msx.*)) _tab5_ovl_ctors_msx_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_pico8_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.pico8 .tab5_ovl_ctors.pico8.*)) _tab5_ovl_ctors_pico8_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_jk_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.jk .tab5_ovl_ctors.jk.*)) _tab5_ovl_ctors_jk_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_darkforces_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.darkforces .tab5_ovl_ctors.darkforces.*)) _tab5_ovl_ctors_darkforces_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_mgs_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.mgs .tab5_ovl_ctors.mgs.*)) _tab5_ovl_ctors_mgs_end = ABSOLUTE(.);
    . = ALIGN(4); _tab5_ovl_ctors_scummvm_start = ABSOLUTE(.);
    KEEP(*(.tab5_ovl_ctors.scummvm .tab5_ovl_ctors.scummvm.*)) _tab5_ovl_ctors_scummvm_end = ABSOLUTE(.);
    . = ALIGN(16); _tab5_ovl_gba_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.gba);
    . = ALIGN(16); _tab5_ovl_snes_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.snes);
    . = ALIGN(16); _tab5_ovl_genesis_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.genesis);
    . = ALIGN(16); _tab5_ovl_doom_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.doom);
    . = ALIGN(16); _tab5_ovl_nes_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.nes);
    . = ALIGN(16); _tab5_ovl_gbc_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.gbc);
    . = ALIGN(16); _tab5_ovl_sms_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.sms);
    . = ALIGN(16); _tab5_ovl_msx_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.msx);
    . = ALIGN(16); _tab5_ovl_pico8_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.pico8);
    . = ALIGN(16); _tab5_ovl_jk_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.jk);
    . = ALIGN(16); _tab5_ovl_darkforces_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.darkforces);
    . = ALIGN(16); _tab5_ovl_mgs_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.mgs);
    . = ALIGN(16); _tab5_ovl_scummvm_lma = ABSOLUTE(.); . += SIZEOF(.tab5_ovl.scummvm);
    . = ALIGN(16);
    _tab5_ovl_store_end = ABSOLUTE(.);
  } > flash_rodata_seg
  ASSERT(ADDR(.flash.tab5_ovl_store) == ADDR(.flash.tdata) + SIZEOF(.flash.tdata),
         "the overlay store must directly follow .flash.tdata (one rodata segment)")
}
_tab5_ovl_gba_size = SIZEOF(.tab5_ovl.gba);
_tab5_ovl_snes_size = SIZEOF(.tab5_ovl.snes);
_tab5_ovl_genesis_size = SIZEOF(.tab5_ovl.genesis);
_tab5_ovl_doom_size = SIZEOF(.tab5_ovl.doom);
_tab5_ovl_nes_size = SIZEOF(.tab5_ovl.nes);
_tab5_ovl_gbc_size = SIZEOF(.tab5_ovl.gbc);
_tab5_ovl_sms_size = SIZEOF(.tab5_ovl.sms);
_tab5_ovl_msx_size = SIZEOF(.tab5_ovl.msx);
_tab5_ovl_pico8_size = SIZEOF(.tab5_ovl.pico8);
_tab5_ovl_jk_size = SIZEOF(.tab5_ovl.jk);
_tab5_ovl_darkforces_size = SIZEOF(.tab5_ovl.darkforces);
_tab5_ovl_mgs_size = SIZEOF(.tab5_ovl.mgs);
_tab5_ovl_scummvm_size = SIZEOF(.tab5_ovl.scummvm);
/* the MMU driver keeps the virtual addresses up to here (the store's) */
_rodata_reserved_end = _tab5_ovl_store_end;
/* keep [PSRAM bss start, overlay end) out of the PSRAM heap (esp_psram.c; a
 * later assignment wins, see ext_ram_data.ld) */
_ext_ram_noinit_end = _tab5_ovl_end;
ASSERT(_tab5_res_start >= _ext_ram_data_end, ".tab5_res must follow .ext_ram.data in PSRAM")
ASSERT(_tab5_ovl_start >= _tab5_res_end, "the code overlay must follow .tab5_res in PSRAM")

/* Initialised data in PSRAM, for builds that execute from flash (no
 * CONFIG_SPIRAM_XIP_FROM_PSRAM). The cores' .data is renamed .ext_ram.data*
 * (tools/bss_to_psram.py --data-into=ext_ram) so internal RAM does not hold
 * it; with XIP from PSRAM it goes into the (then writable) .rodata instead.
 *
 * The linker resolves every reference to these PSRAM addresses. The section
 * cannot be loaded by the bootloader (the image has no PSRAM segments), so
 * tools/ext_ram_data_image.py copies the linked, relocated bytes into
 * tab5_ext_ram_data_image (.rodata, flash) and marks this section NOBITS;
 * src/ext_ram_data.c copies them in before any constructor runs.
 *
 * It follows the other PSRAM sections in ext_ram_seg (.ext_ram.bss, then
 * .ext_ram_noinit). IDF keeps [bss start, noinit end) out of the PSRAM heap
 * (esp_psram.c, CONFIG_SPIRAM_ALLOW_NOINIT_SEG_EXTERNAL_MEMORY), so the end of
 * the noinit range is moved past this section below (a later assignment in a
 * linker script wins). Only [bss start, bss end) is zeroed at boot; the memory
 * test skips the noinit range. (INSERT AFTER .ext_ram.bss would be neater, but
 * IDF links with --enable-non-contiguous-regions, which INSERT does not
 * support.) */
SECTIONS
{
  .ext_ram.data : ALIGN(16)
  {
    _ext_ram_data_start = ABSOLUTE(.);
    /* Metal Gear Solid resets its statics between launches from a snapshot
     * (components/mgs/src/mgs.cpp) and saves them in save states; its glue
     * (mgs.cpp, mgs_platform.cpp: the config, the SD root) is outside the
     * brackets. Inside a section list EXCLUDE_FILE covers only the pattern
     * right after it, hence twice. */
    _mgs_data_start = ABSOLUTE(.);
    *libmgs.a:(EXCLUDE_FILE(*libmgs.a:mgs.* *libmgs.a:mgs_platform.*) .ext_ram.data.mgs_data
               EXCLUDE_FILE(*libmgs.a:mgs.* *libmgs.a:mgs_platform.*) .ext_ram.data.mgs_data.*)
    _mgs_data_end = ABSOLUTE(.);
    _mgs_sdata_start = ABSOLUTE(.);
    *libmgs.a:(EXCLUDE_FILE(*libmgs.a:mgs.* *libmgs.a:mgs_platform.*) .ext_ram.data.mgs_sdata
               EXCLUDE_FILE(*libmgs.a:mgs.* *libmgs.a:mgs_platform.*) .ext_ram.data.mgs_sdata.*)
    _mgs_sdata_end = ABSOLUTE(.);
    *(.ext_ram.data .ext_ram.data.*)
    . = ALIGN(16);
    _ext_ram_data_end = ABSOLUTE(.);
  } > ext_ram_seg
}
/* (the heap starts after the reserved range: see above) */
_ext_ram_noinit_end = _ext_ram_data_end;
ASSERT(_ext_ram_data_start >= _ext_ram_bss_end, ".ext_ram.data must follow .ext_ram.bss in PSRAM")

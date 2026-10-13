/* The bundle's statics (ld -r, see CMakeLists.txt): every .bss of the
 * recompiled game, the runtime and its psyz pieces in one section that the
 * IDF's linker script sends to PSRAM (.ext_ram.bss*: internal RAM is the
 * Tab5's scarce memory), and every .data in one section, each bracketed so
 * src/tpm_engine.cpp can put them back between launches: the runtime, like
 * the PSX, expects to start from a fresh machine. */
SECTIONS
{
  .ext_ram.bss.tpm (NOLOAD) : ALIGN(16)
  {
    _tpm_bss_start = .;
    *(.bss .bss.* .sbss .sbss.* .ext_ram.bss .ext_ram.bss.* COMMON)
    . = ALIGN(16);
    _tpm_bss_end = .;
  }
  .data.tpm : ALIGN(16)
  {
    _tpm_data_start = .;
    *(.data .data.* .sdata .sdata.*)
    . = ALIGN(16);
    _tpm_data_end = .;
  }
}

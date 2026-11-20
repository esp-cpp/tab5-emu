# NES: nofrendo, from the esp-cpp/nofrendo submodule (the core in
# nofrendo/nofrendo, its glue in nofrendo/port)
set(NF "nofrendo/nofrendo")
idf_component_register(
  INCLUDE_DIRS "nofrendo/port/include"
  SRC_DIRS "nofrendo/port" "${NF}/cpu" "${NF}/libsnss" "${NF}/nes" "${NF}/sndhrdw" "${NF}/mappers" "${NF}"
  PRIV_INCLUDE_DIRS "${NF}/cpu" "${NF}/libsnss" "${NF}/nes" "${NF}/sndhrdw" "${NF}"
  REQUIRES "tab5-emu" "statistics" "shared_memory"
)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-char-subscripts -Wno-attributes -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-variable $<$<COMPILE_LANGUAGE:C>:-std=gnu17> $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers>)
# the core's static state (.bss and .data) lives in PSRAM (see the post-build step; the P4
# linker script has no ldgen placeholder for external RAM); no gp-relative
# small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -msmall-data-limit=0)
# executing from flash, the core's code is part of the PSRAM code overlay
# (components/tab5-emu/overlay.ld)
include(${CMAKE_CURRENT_LIST_DIR}/../../tools/overlay.cmake)
tab5_overlay_args(nes overlay_args)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> ${overlay_args}
          # .data to PSRAM: via .rodata where XIP copies it into PSRAM (writable),
          # else into .ext_ram.data (filled at boot from flash; see the script)
          --data --data-into=$<IF:$<BOOL:${CONFIG_SPIRAM_XIP_FROM_PSRAM}>,rodata,ext_ram>
  COMMENT "Moving the nes core's .bss and .data to PSRAM"
  VERBATIM)

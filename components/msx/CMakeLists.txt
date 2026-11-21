# MSX: fMSX, from the esp-cpp/fmsx submodule (the core in fmsx/fmsx, its
# glue in fmsx/port)
set(FM "fmsx/fmsx")
idf_component_register(
  INCLUDE_DIRS "fmsx/port/include"
  SRC_DIRS "fmsx/port" "${FM}" "${FM}/src/EMULib" "${FM}/src/fMSX" "${FM}/src/Z80"
  PRIV_INCLUDE_DIRS "${FM}" "${FM}/src/EMULib" "${FM}/src/fMSX" "${FM}/src/Z80"
  REQUIRES tab5-emu statistics shared_memory
  )
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-char-subscripts -Wno-attributes -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-variable -Wno-discarded-qualifiers)
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=stringop-truncation
    -Wno-error=format-overflow
    -Wno-implicit-fallthrough
    -Wno-format-overflow
    -Wno-stringop-truncation
    -Wno-unused-but-set-variable
    -Wno-unused-variable
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-register>
    -DBPS16 -DUNIX -DLSB_FIRST -DNARROW -O2
)
file(GLOB_RECURSE c_sources "${CMAKE_CURRENT_SOURCE_DIR}/${FM}/src/*.c")
set_source_files_properties(${c_sources} PROPERTIES COMPILE_FLAGS "-include msxfix.h")
# the core's static state (.bss and .data) lives in PSRAM (see the post-build step; the P4
# linker script has no ldgen placeholder for external RAM); no gp-relative
# small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -msmall-data-limit=0)
# executing from flash, the core's code is part of the PSRAM code overlay
# (components/tab5-emu/overlay.ld)
include(${CMAKE_CURRENT_LIST_DIR}/../../tools/overlay.cmake)
tab5_overlay_args(msx overlay_args)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> ${overlay_args}
          # .data to PSRAM: via .rodata where XIP copies it into PSRAM (writable),
          # else into .ext_ram.data (filled at boot from flash; see the script)
          --data --data-into=$<IF:$<BOOL:${CONFIG_SPIRAM_XIP_FROM_PSRAM}>,rodata,ext_ram>
  COMMENT "Moving the msx core's .bss and .data to PSRAM"
  VERBATIM)
# globals other cores in this firmware define too (the box doesn't build
# those): renamed in this library, its glue included
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --redefine-syms=${CMAKE_CURRENT_LIST_DIR}/symbol_renames.syms $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Renaming the globals another core also defines"
  VERBATIM)

# Jump tables for the core's switch statements (the interpreter's opcode
# dispatch). ESP-IDF turns them off for everything, because a table in flash
# breaks code that must run while the cache is disabled; this core runs from
# its PSRAM overlay with the cache on, and its tables move into the overlay
# with its other read-only data. Measured on the Tab5: 10% less CPU per frame.
target_compile_options(${COMPONENT_LIB} PRIVATE -fjump-tables -ftree-switch-conversion)

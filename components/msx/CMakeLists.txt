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
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> --data
  COMMENT "Moving the msx core's .bss and .data to PSRAM"
  VERBATIM)
# globals other cores in this firmware define too (the box doesn't build
# those): renamed in this library, its glue included
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${CMAKE_OBJCOPY} --redefine-syms=${CMAKE_CURRENT_LIST_DIR}/symbol_renames.syms $<TARGET_FILE:${COMPONENT_LIB}>
  COMMENT "Renaming the globals another core also defines"
  VERBATIM)

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
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> --data
  COMMENT "Moving the nes core's .bss and .data to PSRAM"
  VERBATIM)

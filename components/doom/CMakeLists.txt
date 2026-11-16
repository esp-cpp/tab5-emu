# Doom: PrBoom, from the esp-cpp/prboom submodule (the engine in
# prboom/prboom, its glue in prboom/port)
idf_component_register(
  INCLUDE_DIRS "prboom/port/include"
  SRC_DIRS "prboom/port" "prboom/prboom"
  PRIV_INCLUDE_DIRS "prboom" "prboom/prboom"
  REQUIRES tab5-emu shared_memory
  )
target_compile_options(${COMPONENT_LIB} PRIVATE
    -Wno-error=address
    -Wno-error=sizeof-pointer-memaccess
    -Wno-misleading-indentation
    -Wno-format-overflow
    -Wno-char-subscripts
    $<$<COMPILE_LANGUAGE:C>:-std=gnu17>
    $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers>
    $<$<COMPILE_LANGUAGE:C>:-Wno-error=incompatible-pointer-types>
    $<$<COMPILE_LANGUAGE:C>:-Wno-error=compare-distinct-pointer-types>
    -Wno-missing-field-initializers
    -DHAVE_CONFIG_H
    -O2
)
# the core's static state (.bss and .data) lives in PSRAM (see the post-build step; the P4
# linker script has no ldgen placeholder for external RAM); no gp-relative
# small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -msmall-data-limit=0)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> --data
  COMMENT "Moving the doom core's .bss and .data to PSRAM"
  VERBATIM)

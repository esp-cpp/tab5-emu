# Master System / Game Gear: SMS Plus, from the esp-cpp/smsplus submodule
# (the core in smsplus/smsplus, its glue in smsplus/port)
set(SP "smsplus/smsplus")
idf_component_register(
  INCLUDE_DIRS "smsplus/port/include"
  SRC_DIRS "smsplus/port" "${SP}" "${SP}/cpu" "${SP}/sound"
  PRIV_INCLUDE_DIRS "smsplus" "${SP}" "${SP}/cpu" "${SP}/sound"
  REQUIRES tab5-emu statistics shared_memory
  )
# target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-char-subscripts -Wno-attributes -Wno-implicit-fallthrough -Wno-unused-function -Wno-unused-variable -Wno-discarded-qualifiers)
target_compile_options(${COMPONENT_LIB} PRIVATE -Wno-unused-const-variable -Wno-error=sizeof-pointer-memaccess $<$<COMPILE_LANGUAGE:C>:-std=gnu17> $<$<COMPILE_LANGUAGE:C>:-Wno-discarded-qualifiers> $<$<COMPILE_LANGUAGE:C>:-Wno-error=address-of-packed-member>)
target_compile_definitions(${COMPONENT_LIB} PRIVATE LSB_FIRST=1)
# the core's static state (.bss and .data) lives in PSRAM (see the post-build step; the P4
# linker script has no ldgen placeholder for external RAM); no gp-relative
# small data then
target_compile_options(${COMPONENT_LIB} PRIVATE -msmall-data-limit=0)
add_custom_command(TARGET ${COMPONENT_LIB} POST_BUILD
  COMMAND ${PYTHON} ${CMAKE_CURRENT_LIST_DIR}/../../tools/bss_to_psram.py
          ${CMAKE_OBJDUMP} ${CMAKE_OBJCOPY} $<TARGET_FILE:${COMPONENT_LIB}> --data
  COMMENT "Moving the sms core's .bss and .data to PSRAM"
  VERBATIM)

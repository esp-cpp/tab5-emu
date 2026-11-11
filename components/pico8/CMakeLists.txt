# PICO-8 for tab5-emu: femto8 and the glue shared with esp-box-emu, both in
# the femto8 submodule (femto8/port, see femto8/port/README.md).
# COMPONENT_DIR, not CMAKE_CURRENT_SOURCE_DIR: IDF also evaluates this file in
# its early requirements pass, from the build directory.
set(F8 ${COMPONENT_DIR}/femto8/src)
set(PORT ${COMPONENT_DIR}/femto8/port)

file(GLOB SRC_FEMTO8 ${F8}/*.c ${F8}/lua/*.c ${F8}/lodepng/*.c ${F8}/lexaloffle/*.c)

idf_component_register(
  INCLUDE_DIRS "femto8/port/include"
  SRCS ${PORT}/pico8.cpp ${PORT}/platform/p8_lua_alloc.c ${PORT}/platform/p8_state.c ${SRC_FEMTO8}
  PRIV_INCLUDE_DIRS ${PORT} ${PORT}/platform ${F8} ${F8}/data ${F8}/lua ${F8}/lodepng ${F8}/lexaloffle
  REQUIRES tab5-emu statistics esp_timer heap pthread
  LDFRAGMENTS "linker.lf"
  )

# Where carts keep their cartdata() saves (femto8 defaults to a relative "cdata")
target_compile_definitions(${COMPONENT_LIB} PRIVATE
  FEMTO8_ESPBOX=1
  # PICO8_PROFILE=1: report Lua instructions/s, GC share and the cycle count of
  # every API function with the fps line (see femto8/port/pico8.cpp)
  # PICO8_PROFILE=1
  CARTDATA_PATH="/sdcard/pico8/cdata"
  DEFAULT_CARTS_PATH="/sdcard/pico8")

# -msmall-data-limit=0: no gp-relative .sbss/.sdata, so every static falls
# inside the linker.lf SURROUND bounds pico8.cpp resets between launches
target_compile_options(${COMPONENT_LIB} PRIVATE
  -O2
  -msmall-data-limit=0
  -Wno-error
  -w)

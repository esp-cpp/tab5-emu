idf_component_register(
  INCLUDE_DIRS "include"
  SRC_DIRS "src"
  LDFRAGMENTS "linker.lf"
  REQUIRES
  "driver"
  "heap"
  "fatfs"
  "esp_lcd"
  "esp_psram"
  "esp_driver_ppa"
  "esp_mm"
  "hal"
  "lvgl"
  "codec"
  "display"
  "display_drivers"
  "input_drivers"
  "gt911"
  "event_manager"
  "pool_allocator"
  "task"
  "timer"
  "m5stack-tab5"
  "statistics"
  "usb_host"
  "hid-rp"
  "usb_device"
  "esp_hal_usb"
  "coredump"
  "mbedtls"
  "sdcard"
  )

# LVGL's custom allocator lives in src/lv_mem_core_psram.c; liblvgl is linked
# after this library, so pull the object in explicitly
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u lv_mem_init")

# Executing from flash (no XIP from PSRAM), the cores' initialised data lives in
# the PSRAM section .ext_ram.data: place it (ext_ram_data.ld) and pull in the
# constructor that fills it at boot (src/ext_ram_data.c); the root
# CMakeLists.txt stores its contents in flash after linking
if(NOT CONFIG_SPIRAM_XIP_FROM_PSRAM)
  target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/ext_ram_data.ld")
  # and the game cores' code overlay (overlay.ld, src/overlay.c), after it
  target_linker_script(${COMPONENT_LIB} INTERFACE "${CMAKE_CURRENT_LIST_DIR}/overlay.ld")
  target_link_libraries(${COMPONENT_LIB} INTERFACE "-u tab5_ext_ram_data_init")
endif()

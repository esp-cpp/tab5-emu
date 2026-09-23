idf_component_register(
  INCLUDE_DIRS "include"
  SRC_DIRS "src"
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
  "task"
  "timer"
  "m5stack-tab5"
  "statistics"
  "usb_host"
  "hid-rp"
  "usb_device"
  "esp_hal_usb"
  "espcoredump"
  "esp_partition"
  "mbedtls"
  "sdcard"
  )

# LVGL's custom allocator lives in src/lv_mem_core_psram.c; liblvgl is linked
# after this library, so pull the object in explicitly
target_link_libraries(${COMPONENT_LIB} INTERFACE "-u lv_mem_init")

// The Tab5 Emu updater (the ota_1 slot). The emulator app (ota_0) is 9+ MB
// and the Tab5 has 16 MB of flash, so the usual two-slot OTA cannot hold two
// emulators; instead an update reboots into this small app, which serves the
// espp OTA protocol on the USB-C port's vendor interface, writes the new
// emulator into ota_0 and boots it (the OTA service restarts after FINISH).
//
// Booted with no update to take (e.g. ENTER_UPDATER and then nothing), it
// boots the emulator again after kIdleTimeout. If the emulator image is not
// usable (a failed update, or one that rolled back here) it waits for one.
#include <atomic>
#include <chrono>
#include <thread>

#include <driver/gpio.h>
#include <esp_app_desc.h>
#include <esp_ota_ops.h>
#include <esp_timer.h>
#include <esp_system.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/usb_serial_jtag_ll.h>
#include <hal/usb_wrap_ll.h>
#include <soc/lp_system_struct.h>
#include <soc/usb_wrap_struct.h>

#include "dispatcher_worker.hpp"
#include "logger.hpp"
#include "ota.hpp"
#include "ota_service.hpp"
#include "system_service.hpp"
#include "usb_device.hpp"

using namespace std::chrono_literals;

static constexpr auto kIdleTimeout = 90s;

// The USB-C pins: by default the USB-Serial-JTAG console has them (PHY 0); the
// OTG 1.1 controller takes them while this app runs (see the emulator's
// components/tab5-emu/src/usb_device.cpp for the details).
static void usb_otg11_take_usb_c(bool take) {
  if (take) {
    usb_serial_jtag_ll_phy_enable_pad(false);
    vTaskDelay(pdMS_TO_TICKS(300)); // let the PC see the disconnect
    usb_wrap_ll_phy_select(&USB_WRAP, 0);
    gpio_set_drive_capability(GPIO_NUM_24, GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(GPIO_NUM_25, GPIO_DRIVE_CAP_3);
  } else {
    LP_SYS.usb_ctrl.sw_usb_phy_sel = 0;
    LP_SYS.usb_ctrl.sw_hw_usb_phy_sel = 0;
    usb_serial_jtag_ll_phy_enable_pad(true);
  }
}

static const esp_partition_t *emulator_partition() {
  return esp_partition_find_first(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_OTA_0, nullptr);
}

// Whether ota_0 holds an app that can boot (a valid image that did not fail
// its verification after an update).
static bool emulator_bootable() {
  const auto *p = emulator_partition();
  if (!p) return false;
  esp_app_desc_t desc;
  if (esp_ota_get_partition_description(p, &desc) != ESP_OK) return false;
  esp_ota_img_states_t state;
  if (esp_ota_get_state_partition(p, &state) == ESP_OK &&
      (state == ESP_OTA_IMG_INVALID || state == ESP_OTA_IMG_ABORTED)) {
    return false;
  }
  return true;
}

extern "C" void app_main(void) {
  espp::Logger logger({.tag = "updater", .level = espp::Logger::Verbosity::INFO});
  logger.info("Tab5 Emu updater {}", esp_app_get_description()->version);
  // this app is flashed once, not through an OTA; never leave it pending
  esp_ota_mark_app_valid_cancel_rollback();

  espp::UsbDevice::Config cfg;
  cfg.manufacturer = "esp-cpp";
  cfg.product = "Tab5 Emu updater";
  cfg.serial_number = "tab5-emu";
  cfg.max_power_ma = 500;
  cfg.port = 0; // the full-speed controller, on the USB-C pins
  espp::UsbDevice::CdcFunction cdc;
  cdc.interface_name = "Tab5 Emu updater console";
  cdc.route_console = true;
  cfg.cdc = cdc;
  espp::UsbDevice::VendorFunction vendor;
  vendor.interface_name = "Tab5 Emu link";
  vendor.webusb = true;
  vendor.landing_page_url = "esp-cpp.github.io/espp/apps/ota_console.html";
  cfg.vendor = vendor;

  usb_otg11_take_usb_c(true);
  espp::UsbDevice usb(cfg);

  std::mutex tx_mutex;
  auto send = [&](std::span<const uint8_t> frame) {
    std::lock_guard<std::mutex> lk(tx_mutex);
    usb.write_vendor(frame);
  };

  std::atomic<bool> busy{false}; // an update has started: do not time out
  // the idle timeout counts from the last request (a discovery or status
  // query only delays it; an update, once it writes, stops it)
  std::atomic<int64_t> last_rx_us{esp_timer_get_time()};
  espp::Ota ota({.progress_callback = [&](size_t written, size_t total) { busy = true; }});
  espp::OtaService ota_service(ota, {.send = send, .log_level = espp::Logger::Verbosity::INFO});
  espp::SystemService system_service({.send = send,
                                      .on_reboot_request =
                                          [](espp::SystemService::RebootKind kind) {
                                            if (kind == espp::SystemService::RebootKind::Bootloader) {
                                              usb_otg11_take_usb_c(false);
                                            }
                                            return true;
                                          }});
  espp::DispatcherWorker link({.send = send,
                               .on_overflow = [&]() { ota_service.on_rx_overflow(); },
                               .task_config = {.name = "link", .stack_size_bytes = 8192}});
  link.register_module(ota_service);
  link.register_module(system_service);
  link.serve_discovery("Tab5 Emu updater", esp_app_get_description()->version);
  usb.set_vendor_receive_callback([&](std::span<const uint8_t> data) {
    last_rx_us = esp_timer_get_time();
    link.push(data);
  });

  std::error_code ec;
  if (!usb.initialize(ec)) {
    usb_otg11_take_usb_c(false);
    logger.error("USB device: {}", ec.message());
  }
  logger.info("waiting for an emulator image (espp OTA on the vendor interface)");

  while (true) {
    std::this_thread::sleep_for(1s);
    // nothing asked of us: back to the emulator, if it can boot
    const auto idle = std::chrono::microseconds(esp_timer_get_time() - last_rx_us);
    if (!busy && idle > kIdleTimeout && emulator_bootable()) {
      // (a slot whose header reads fine can still fail the full image check:
      // then stay here, where an update can still fix it)
      if (const esp_err_t err = esp_ota_set_boot_partition(emulator_partition()); err != ESP_OK) {
        logger.error("the emulator image does not verify ({}); waiting for an update", esp_err_to_name(err));
        busy = true;
        continue;
      }
      logger.info("no update; booting the emulator");
      usb_otg11_take_usb_c(false);
      esp_restart();
    }
  }
}

// USB mass storage: the SD card as a USB drive (espp::UsbDevice MSC function
// over the BSP's espp::SdCard). See tab5-emu.hpp for the ownership rules.
#include "tab5-emu.hpp"

#include <driver/gpio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <hal/usb_serial_jtag_ll.h>
#include <hal/usb_wrap_ll.h>
#include <soc/usb_serial_jtag_struct.h>
#include <soc/usb_wrap_struct.h>

// The ESP32-P4 has two full-speed PHYs: PHY 0 on GPIO24/25 and PHY 1 on
// GPIO26/27. By default the USB-Serial-JTAG console owns PHY 0 and the OTG 1.1
// controller (TinyUSB port 0) owns PHY 1. On the Tab5 only PHY 0 reaches a
// connector (USB-C); GPIO26/27 carry I2S. LP_SYS.usb_ctrl can swap the two, so
// while the drive is on the OTG controller takes the USB-C pins and the
// console is parked on the (pad-disabled) other PHY.
static constexpr gpio_num_t usb_c_dm_io = GPIO_NUM_24;
static constexpr gpio_num_t usb_c_dp_io = GPIO_NUM_25;
static constexpr gpio_num_t phy1_dm_io = GPIO_NUM_26;
static constexpr gpio_num_t phy1_dp_io = GPIO_NUM_27;

// A PC only re-enumerates after it has seen a disconnect, so the pull-up has
// to drop for a moment before the other controller takes the pins.
static constexpr int usb_disconnect_ms = 300;

static void usb_otg11_take_usb_c(bool take) {
  if (take) {
    // the console drops off the bus (its pads are also the audio pins once
    // it is moved to PHY 1, so they stay disabled while the drive is on)
    usb_serial_jtag_ll_phy_enable_pad(false);
    vTaskDelay(pdMS_TO_TICKS(usb_disconnect_ms));
    usb_wrap_ll_phy_select(&USB_WRAP, 0);
    // the internal PHY driver only set the drive strength on PHY 1's pins
    gpio_set_drive_capability(usb_c_dm_io, GPIO_DRIVE_CAP_3);
    gpio_set_drive_capability(usb_c_dp_io, GPIO_DRIVE_CAP_3);
  } else {
    // the OTG controller is already torn down here (pull-up gone)
    vTaskDelay(pdMS_TO_TICKS(usb_disconnect_ms));
    LP_SYS.usb_ctrl.sw_usb_phy_sel = 0;
    LP_SYS.usb_ctrl.sw_hw_usb_phy_sel = 0; // hardware default: console on PHY 0
    usb_serial_jtag_ll_phy_enable_pad(true);
    gpio_set_drive_capability(phy1_dm_io, GPIO_DRIVE_CAP_2);
    gpio_set_drive_capability(phy1_dp_io, GPIO_DRIVE_CAP_2);
  }
}

// LP_SYS survives a software reset: make sure the console owns the USB-C
// pins whenever the app starts (e.g. after a crash while the drive was on).
void Tab5Emu::usb_msc_restore_console() {
  if (LP_SYS.usb_ctrl.sw_hw_usb_phy_sel) {
    usb_otg11_take_usb_c(false);
  }
}

bool Tab5Emu::initialize_usb_msc() {
  if (usb_device_) {
    return true;
  }
  auto &bsp = Bsp::get();
  auto *sd = bsp.sdcard_component();
  if (!sd || !bsp.sdcard()) {
    logger_.error("USB drive: no SD card");
    return false;
  }
  // one OTG controller: the host side (keyboard / mouse) has to go
  usb_host_was_enabled_ = is_usb_host_enabled();
  if (usb_host_was_enabled_) {
    deinitialize_usb_host();
  }
  // release the volume; the MSC function mounts it for us while we own it
  std::error_code ec;
  if (sd->is_mounted() && !sd->unmount(ec)) {
    logger_.error("USB drive: could not unmount the SD card: {}", ec.message());
    if (usb_host_was_enabled_) initialize_usb_host();
    return false;
  }
  espp::UsbDevice::Config cfg;
  cfg.manufacturer = "esp-cpp";
  cfg.product = "Tab5 Emu";
  cfg.serial_number = "tab5-emu";
  // The P4's high-speed OTG port is wired to the Tab5's USB-A host jack; the
  // USB-C connector carries the full-speed port (shared with the
  // USB-Serial-JTAG console, which drops out while the drive is on).
  cfg.port = 0; // TINYUSB_PORT_FULL_SPEED_0
  espp::UsbDevice::MscMedium medium;
  medium.type = espp::UsbDevice::MscMedium::Type::SdCard;
  medium.sd_card = bsp.sdcard();
  medium.base_path = mount_point;
  medium.max_files = 8;
  medium.initial_owner = espp::UsbDevice::MscOwner::App;
  espp::UsbDevice::MscFunction msc;
  msc.interface_name = "Tab5 Emu SD card";
  msc.media = {medium};
  msc.auto_handover = true; // the PC takes the card when it mounts the drive
  msc.on_event = [this](size_t lun, espp::UsbDevice::MscEvent event, espp::UsbDevice::MscOwner owner) {
    using E = espp::UsbDevice::MscEvent;
    if (event == E::OwnerChanged) {
      logger_.info("USB drive: SD card now owned by the {}", owner == espp::UsbDevice::MscOwner::Host ? "PC" : "device");
    } else if (event == E::OwnerChangeFailed) {
      logger_.error("USB drive: hand-over failed");
    } else if (event == E::FormatRequired) {
      logger_.error("USB drive: the SD card has no FAT filesystem");
    }
  };
  cfg.msc = msc;
  logger_.info("USB drive: moving the USB-C port from the console to the OTG controller");
  usb_otg11_take_usb_c(true);
  usb_device_ = std::make_unique<espp::UsbDevice>(cfg);
  if (!usb_device_->initialize(ec)) {
    usb_device_.reset();
    usb_otg11_take_usb_c(false);
    logger_.error("USB drive: could not start the USB device: {}", ec.message());
    sd->mount();
    if (usb_host_was_enabled_) initialize_usb_host();
    return false;
  }
  logger_.info("USB drive on: plug the USB-C port into a PC (the serial console is off until the drive is)");
  return true;
}

bool Tab5Emu::usb_msc_host_has_card() const {
  if (!usb_device_) {
    return false;
  }
  const auto owner = usb_device_->msc_owner(0);
  return owner.has_value() && *owner == espp::UsbDevice::MscOwner::Host;
}

void Tab5Emu::deinitialize_usb_msc() {
  if (!usb_device_) {
    return;
  }
  // destroying the device detaches from the PC and releases the card
  // (initialized, unmounted); then it is ours again
  usb_device_.reset();
  usb_otg11_take_usb_c(false);
  auto *sd = Bsp::get().sdcard_component();
  std::error_code ec;
  if (sd && !sd->is_mounted() && !sd->mount(ec)) {
    logger_.error("USB drive: could not re-mount the SD card: {}", ec.message());
  }
  if (usb_host_was_enabled_) {
    initialize_usb_host();
  }
  logger_.info("USB drive off");
}

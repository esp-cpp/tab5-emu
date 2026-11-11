// The composite USB device on the USB-C port (espp::UsbDevice): the console
// on CDC, the framed link (OTA, core dump, system, test control) on a vendor
// interface, and the SD card as a USB drive (MSC over the BSP's espp::SdCard).
// See tab5-emu.hpp for the ownership rules.
#include "tab5-emu.hpp"

#include <cstdio>
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

bool Tab5Emu::initialize_usb_device() {
  if (usb_device_) {
    return true;
  }
  auto &bsp = Bsp::get();
  auto *sd = bsp.sdcard_component();
  const bool have_sd = sd && bsp.sdcard();
  std::error_code ec;
  // the MSC function mounts the card for us while we own it: release our mount
  if (have_sd && sd->is_mounted() && !sd->unmount(ec)) {
    logger_.error("USB: could not unmount the SD card for the drive function: {}", ec.message());
    return false;
  }
  espp::UsbDevice::Config cfg;
  cfg.manufacturer = "esp-cpp";
  cfg.product = "Tab5 Emu";
  cfg.serial_number = "tab5-emu";
  // The P4's high-speed OTG port is wired to the Tab5's USB-A host jack; the
  // USB-C connector carries the full-speed port (shared with the
  // USB-Serial-JTAG console, which is parked while TinyUSB owns the pins).
  cfg.port = 0; // TINYUSB_PORT_FULL_SPEED_0
  // the console: logs on this CDC port (teed to UART0)
  espp::UsbDevice::CdcFunction cdc;
  cdc.interface_name = "Tab5 Emu console";
  cdc.route_console = true;
  cfg.cdc = cdc;
  // the framed services (OTA, core dump, system, test control)
  espp::UsbDevice::VendorFunction vendor;
  vendor.interface_name = "Tab5 Emu link";
  vendor.webusb = true;
  vendor.landing_page_url = "esp-cpp.github.io/espp/apps/device_hub.html";
  vendor.on_receive = [this](std::span<const uint8_t> data) {
    if (vendor_rx_) vendor_rx_(data);
  };
  cfg.vendor = vendor;
  // the SD card as a USB drive: ours until handed over (initialize_usb_msc())
  if (have_sd) {
    espp::UsbDevice::MscMedium medium;
    medium.type = espp::UsbDevice::MscMedium::Type::SdCard;
    medium.sd_card = bsp.sdcard();
    medium.base_path = mount_point;
    medium.max_files = 16;
    medium.initial_owner = espp::UsbDevice::MscOwner::App;
    espp::UsbDevice::MscFunction msc;
    msc.interface_name = "Tab5 Emu SD card";
    msc.media = {medium};
    msc.auto_handover = false; // only when asked (launcher switch / host request)
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
  }
  logger_.info("USB: moving the USB-C port from the serial console to the composite device");
  usb_otg11_take_usb_c(true);
  usb_device_ = std::make_unique<espp::UsbDevice>(cfg);
  if (!usb_device_->initialize(ec)) {
    usb_device_.reset();
    usb_otg11_take_usb_c(false);
    logger_.error("USB: could not start the composite device: {}", ec.message());
    if (have_sd) sd->mount();
    return false;
  }
  logger_.info("USB: composite device up (console, link{})", have_sd ? ", SD card drive" : "");
  // espp routes only stdout to the CDC console; send stderr (the cores' error
  // messages) there too. /dev/usbcons is the console device espp registers.
  if (usb_device_->is_console_routed_to_cdc() && freopen("/dev/usbcons", "w", stderr)) {
    setvbuf(stderr, nullptr, _IONBF, 0);
  }
  return true;
}

bool Tab5Emu::vendor_write(std::span<const uint8_t> frame) {
  std::lock_guard<std::mutex> lk(vendor_tx_mutex_);
  return usb_device_ && usb_device_->write_vendor(frame);
}

void Tab5Emu::usb_prepare_for_rom_download() {
  // LP_SYS survives the reset: give the USB-C pins back to the USB-Serial-JTAG
  // controller so the ROM's download mode shows up on the connector
  LP_SYS.usb_ctrl.sw_usb_phy_sel = 0;
  LP_SYS.usb_ctrl.sw_hw_usb_phy_sel = 0;
  usb_serial_jtag_ll_phy_enable_pad(true);
}

bool Tab5Emu::initialize_usb_msc() {
  if (!usb_device_ || usb_device_->msc_lun_count() == 0) {
    logger_.error("USB drive: no SD card drive function");
    return false;
  }
  // the launcher reads the card (metadata, boxart) under the LVGL lock: hold
  // it so the volume is not unmounted under one of its reads
  std::lock_guard<std::recursive_mutex> lk(lvgl_mutex());
  std::error_code ec;
  if (!usb_device_->set_msc_owner(0, espp::UsbDevice::MscOwner::Host, ec)) {
    logger_.error("USB drive: could not hand the SD card to the PC: {}", ec.message());
    return false;
  }
  logger_.info("USB drive on: the PC has the SD card");
  return true;
}

void Tab5Emu::deinitialize_usb_msc() {
  if (!usb_device_ || usb_device_->msc_lun_count() == 0) {
    return;
  }
  std::lock_guard<std::recursive_mutex> lk(lvgl_mutex()); // see initialize_usb_msc()
  std::error_code ec;
  // The PC stays attached (console, link), so this is taken on trust that it
  // ejected the drive first (the launcher says so; emulink drive off ejects
  // it): one that did not may lose writes it had cached, as on an unplug.
  if (!usb_device_->take_msc_from_ejected_host(0, ec)) {
    logger_.error("USB drive: could not take the SD card back: {}", ec.message());
    return;
  }
  logger_.info("USB drive off: the SD card is ours again");
}

bool Tab5Emu::usb_msc_host_has_card() const {
  if (!usb_device_ || usb_device_->msc_lun_count() == 0) {
    return false;
  }
  const auto owner = usb_device_->msc_owner(0);
  return owner.has_value() && *owner == espp::UsbDevice::MscOwner::Host;
}

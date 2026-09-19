// USB host + HID keyboard / mouse for the Tab5's USB-A port.
//
// The USB host library runs its event loop on its own task, the HID class
// driver on another (both core 1, so the engine core is left alone). New
// HID interfaces are announced on the driver task; opening them is deferred
// to a small worker task (the class driver must not be re-entered from its
// callback). Boot-protocol reports are decoded here into a keyboard usage
// bitmap and a mouse accumulator that the game glue polls.
#include "tab5-emu.hpp"

#include <cstring>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <usb/hid_host.h>
#include <usb/hid_usage_keyboard.h>
#include <usb/hid_usage_mouse.h>
#include <usb/usb_host.h>

struct Tab5Emu::UsbHid {
  Tab5Emu *emu{nullptr};
  QueueHandle_t connect_queue{nullptr};
  TaskHandle_t usb_task{nullptr};
  TaskHandle_t open_task{nullptr};
  espp::Logger logger{{.tag = "UsbHid", .level = espp::Logger::Verbosity::INFO}};

  static void usb_lib_task(void *arg) {
    auto *self = static_cast<UsbHid *>(arg);
    while (true) {
      uint32_t event_flags = 0;
      usb_host_lib_handle_events(portMAX_DELAY, &event_flags);
      if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
        // HID driver deregistered (not expected while running); free devices
        // so a re-install could start clean
        usb_host_device_free_all();
      }
      (void)self;
    }
  }

  // HID driver task context: just hand the new interface to the worker
  static void on_driver_event(hid_host_device_handle_t dev, const hid_host_driver_event_t event, void *arg) {
    auto *self = static_cast<UsbHid *>(arg);
    if (event == HID_HOST_DRIVER_EVENT_CONNECTED && self->connect_queue) {
      xQueueSend(self->connect_queue, &dev, 0);
    }
  }

  static void on_interface_event(hid_host_device_handle_t dev, const hid_host_interface_event_t event, void *arg) {
    auto *self = static_cast<UsbHid *>(arg);
    hid_host_dev_params_t params{};
    hid_host_device_get_params(dev, &params);
    switch (event) {
    case HID_HOST_INTERFACE_EVENT_INPUT_REPORT: {
      uint8_t data[64];
      size_t len = 0;
      if (hid_host_device_get_raw_input_report_data(dev, data, sizeof(data), &len) != ESP_OK) {
        break;
      }
      if (params.sub_class == HID_SUBCLASS_BOOT_INTERFACE) {
        if (params.proto == HID_PROTOCOL_KEYBOARD) {
          self->emu->on_hid_keyboard_report(data, len);
        } else if (params.proto == HID_PROTOCOL_MOUSE) {
          self->emu->on_hid_mouse_report(data, len);
        }
      }
      break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED:
      self->logger.info("HID {} disconnected (addr {} iface {})",
                        params.proto == HID_PROTOCOL_KEYBOARD ? "keyboard"
                        : params.proto == HID_PROTOCOL_MOUSE  ? "mouse"
                                                              : "device",
                        params.addr, params.iface_num);
      if (params.proto == HID_PROTOCOL_KEYBOARD) {
        self->emu->usb_keyboards_--;
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        memset(self->emu->keyboard_.keys, 0, sizeof(self->emu->keyboard_.keys));
      } else if (params.proto == HID_PROTOCOL_MOUSE) {
        self->emu->usb_mice_--;
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        self->emu->mouse_.buttons = 0;
      }
      hid_host_device_close(dev);
      break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR:
      self->logger.warn("HID transfer error (addr {} iface {})", params.addr, params.iface_num);
      break;
    default:
      break;
    }
  }

  static void open_worker(void *arg) {
    auto *self = static_cast<UsbHid *>(arg);
    while (true) {
      hid_host_device_handle_t dev = nullptr;
      if (xQueueReceive(self->connect_queue, &dev, portMAX_DELAY) != pdTRUE) {
        continue;
      }
      hid_host_dev_params_t params{};
      hid_host_device_get_params(dev, &params);
      const bool keyboard = params.proto == HID_PROTOCOL_KEYBOARD;
      const bool mouse = params.proto == HID_PROTOCOL_MOUSE;
      self->logger.info("HID {} connected (addr {} iface {} subclass {})",
                        keyboard ? "keyboard" : mouse ? "mouse" : "device", params.addr, params.iface_num,
                        params.sub_class);
      if (!keyboard && !mouse) {
        // generic HID (gamepads etc.): not handled yet
        continue;
      }
      const hid_host_device_config_t cfg = {.callback = &UsbHid::on_interface_event, .callback_arg = self};
      if (auto err = hid_host_device_open(dev, &cfg); err != ESP_OK) {
        self->logger.error("open failed: {}", esp_err_to_name(err));
        continue;
      }
      if (params.sub_class == HID_SUBCLASS_BOOT_INTERFACE) {
        hid_class_request_set_protocol(dev, HID_REPORT_PROTOCOL_BOOT);
        if (keyboard) {
          hid_class_request_set_idle(dev, 0, 0);
        }
      }
      if (auto err = hid_host_device_start(dev); err != ESP_OK) {
        self->logger.error("start failed: {}", esp_err_to_name(err));
        hid_host_device_close(dev);
        continue;
      }
      if (keyboard) {
        self->emu->usb_keyboards_++;
      } else {
        self->emu->usb_mice_++;
      }
    }
  }
};

bool Tab5Emu::initialize_usb_host() {
  if (usb_hid_) {
    return true;
  }
  auto *hid = new UsbHid();
  hid->emu = this;
  const usb_host_config_t host_config = {
      .skip_phy_setup = false,
      .intr_flags = ESP_INTR_FLAG_LOWMED,
  };
  if (auto err = usb_host_install(&host_config); err != ESP_OK) {
    logger_.error("usb_host_install failed: {}", esp_err_to_name(err));
    delete hid;
    return false;
  }
  if (xTaskCreatePinnedToCore(&UsbHid::usb_lib_task, "usb_events", 4096, hid, 6, &hid->usb_task, 1) != pdPASS) {
    logger_.error("could not create the USB event task");
    usb_host_uninstall();
    delete hid;
    return false;
  }
  hid->connect_queue = xQueueCreate(8, sizeof(hid_host_device_handle_t));
  if (xTaskCreatePinnedToCore(&UsbHid::open_worker, "usb_hid", 4096, hid, 5, &hid->open_task, 1) != pdPASS) {
    logger_.error("could not create the HID worker task");
    return false;
  }
  const hid_host_driver_config_t driver_config = {
      .create_background_task = true,
      .task_priority = 5,
      .stack_size = 4096,
      .core_id = 1,
      .callback = &UsbHid::on_driver_event,
      .callback_arg = hid,
  };
  if (auto err = hid_host_install(&driver_config); err != ESP_OK) {
    logger_.error("hid_host_install failed: {}", esp_err_to_name(err));
    return false;
  }
  usb_hid_ = hid;
  logger_.info("USB host ready (HID keyboard / mouse, hubs supported)");
  return true;
}

// Boot keyboard report: [modifiers][reserved][6 key usages]
void Tab5Emu::on_hid_keyboard_report(const uint8_t *data, size_t len) {
  if (len < 8) {
    return;
  }
  KeyboardState st{};
  const uint8_t mods = data[0];
  for (int i = 0; i < 8; i++) {
    if (mods & (1 << i)) {
      const uint8_t usage = 0xE0 + i;
      st.keys[usage >> 3] |= 1 << (usage & 7);
    }
  }
  for (int i = 2; i < 8; i++) {
    const uint8_t usage = data[i];
    if (usage > HID_KEY_ERROR_UNDEFINED) { // 0..3 are no-key / rollover / fail / undefined
      st.keys[usage >> 3] |= 1 << (usage & 7);
    }
  }
  std::lock_guard<std::mutex> lk(hid_mutex_);
  keyboard_ = st;
}

// Boot mouse report: [buttons][dx][dy][wheel?]
void Tab5Emu::on_hid_mouse_report(const uint8_t *data, size_t len) {
  if (len < 3) {
    return;
  }
  std::lock_guard<std::mutex> lk(hid_mutex_);
  mouse_.buttons = data[0] & 0x07;
  mouse_.dx += static_cast<int8_t>(data[1]);
  mouse_.dy += static_cast<int8_t>(data[2]);
  if (len >= 4) {
    mouse_.wheel += static_cast<int8_t>(data[3]);
  }
}

Tab5Emu::KeyboardState Tab5Emu::keyboard_state() const {
  std::lock_guard<std::mutex> lk(hid_mutex_);
  return keyboard_;
}

Tab5Emu::MouseState Tab5Emu::mouse_state() const {
  std::lock_guard<std::mutex> lk(hid_mutex_);
  return mouse_;
}

Tab5Emu::MouseState Tab5Emu::take_mouse_motion() {
  std::lock_guard<std::mutex> lk(hid_mutex_);
  auto st = mouse_;
  mouse_.dx = mouse_.dy = mouse_.wheel = 0;
  return st;
}

// USB host + HID keyboard / mouse for the Tab5's USB-A port.
//
// The USB host library runs its event loop on its own task, the HID class
// driver on another (both core 1, so the engine core is left alone). New
// HID interfaces are announced on the driver task; opening them is deferred
// to a small worker task (the class driver must not be re-entered from its
// callback). Boot-protocol reports are decoded here into a keyboard usage
// bitmap and a mouse accumulator that the game glue polls.
#include "tab5-emu.hpp"

#include <map>

#include "hid_gamepad.hpp"

#include <atomic>
#include <cstring>
#include <mutex>
#include <vector>

#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/task.h>
#include <esp_heap_caps.h>
#include <usb/hid_host.h>
#include <usb/hid_usage_keyboard.h>
#include <usb/hid_usage_mouse.h>
#include <usb/usb_host.h>

struct Tab5Emu::UsbHid {
  Tab5Emu *emu{nullptr};
  QueueHandle_t connect_queue{nullptr};
  TaskHandle_t usb_task{nullptr};
  TaskHandle_t open_task{nullptr};
  std::atomic<bool> quit{false};
  std::atomic<bool> all_free{false};
  std::mutex open_mutex;
  std::vector<hid_host_device_handle_t> open_devices; // for teardown
  std::map<hid_host_device_handle_t, HidGamepadMap> gamepads; // generic HID devices with a gamepad-like descriptor
  HidGamepadMap::Raw last_raw{};
  std::string last_unknown;
  espp::Logger logger{{.tag = "UsbHid", .level = espp::Logger::Verbosity::INFO}};

  static void usb_lib_task(void *arg) {
    auto *self = static_cast<UsbHid *>(arg);
    while (true) {
      uint32_t event_flags = 0;
      usb_host_lib_handle_events(pdMS_TO_TICKS(100), &event_flags);
      if (event_flags & USB_HOST_LIB_EVENT_FLAGS_NO_CLIENTS) {
        // the HID driver deregistered: free the devices so the library can
        // be uninstalled (teardown) or re-used. With nothing attached this
        // completes at once and no ALL_FREE event follows.
        if (usb_host_device_free_all() == ESP_OK) {
          self->all_free = true;
        }
      }
      if (event_flags & USB_HOST_LIB_EVENT_FLAGS_ALL_FREE) {
        self->all_free = true;
      }
      if (self->quit && self->all_free) {
        break;
      }
    }
    // deinitialize_usb_host() deletes us (a WithCaps task cannot free its
    // own stack)
    vTaskSuspend(nullptr);
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
      if (params.sub_class == HID_SUBCLASS_BOOT_INTERFACE && params.proto == HID_PROTOCOL_KEYBOARD) {
        self->emu->on_hid_keyboard_report(data, len);
      } else if (params.sub_class == HID_SUBCLASS_BOOT_INTERFACE && params.proto == HID_PROTOCOL_MOUSE) {
        self->emu->on_hid_mouse_report(data, len);
      } else {
        const HidGamepadMap *map = nullptr;
        {
          std::lock_guard<std::mutex> lk(self->open_mutex);
          auto it = self->gamepads.find(dev);
          if (it != self->gamepads.end()) map = &it->second;
        }
        GamepadState state;
        bool menu = false;
        HidGamepadMap::Raw raw;
        GamepadAxes axes;
        if (map && map->decode(data, len, state, menu, &raw, &axes)) {
          self->emu->on_hid_gamepad_state(state, menu, axes);
          // mapping diagnostics: log the raw report whenever the pressed
          // buttons / d-pad / consumer controls change (not the sticks)
          if (raw.buttons != self->last_raw.buttons || raw.dpad != self->last_raw.dpad || raw.hat != self->last_raw.hat ||
              memcmp(raw.consumer, self->last_raw.consumer, sizeof(raw.consumer)) != 0) {
            self->last_raw = raw;
            std::string btns, cons;
            for (int n = 1; n < 32; n++) {
              if (raw.buttons & (1u << n)) btns += fmt::format("{} ", n);
            }
            for (auto u : raw.consumer) {
              if (u) cons += fmt::format("{:#x} ", u);
            }
            self->logger.info("gamepad: report {} buttons [{}] dpad {:#x} hat {} axes {} {} {} {} {} {} consumer [{}] -> {:#x}",
                              raw.report_id, btns, raw.dpad, raw.hat, raw.axes[0], raw.axes[1], raw.axes[2],
                              raw.axes[3], raw.axes[4], raw.axes[5], cons, state.buttons);
          }
        } else if (map) {
          // a report this map knows nothing about: dump it once per content
          std::string hex;
          for (size_t i = 0; i < len && i < 16; i++) hex += fmt::format("{:02x} ", data[i]);
          if (hex != self->last_unknown) {
            self->last_unknown = hex;
            self->logger.info("gamepad: unmapped report [{}]", hex);
          }
        }
      }
      break;
    }
    case HID_HOST_INTERFACE_EVENT_DISCONNECTED: {
      bool gamepad = false;
      {
        std::lock_guard<std::mutex> lk(self->open_mutex);
        std::erase(self->open_devices, dev);
        gamepad = self->gamepads.erase(dev) > 0;
      }
      self->logger.info("HID {} disconnected (addr {} iface {})",
                        params.proto == HID_PROTOCOL_KEYBOARD ? "keyboard"
                        : params.proto == HID_PROTOCOL_MOUSE  ? "mouse"
                        : gamepad                             ? "gamepad"
                                                              : "device",
                        params.addr, params.iface_num);
      if (gamepad) {
        self->emu->usb_gamepads_--;
        self->emu->on_hid_gamepad_state(GamepadState{}, false);
      } else if (params.proto == HID_PROTOCOL_KEYBOARD) {
        self->emu->usb_keyboards_--;
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        memset(self->emu->keyboard_.keys, 0, sizeof(self->emu->keyboard_.keys));
      } else if (params.proto == HID_PROTOCOL_MOUSE) {
        self->emu->usb_mice_--;
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        self->emu->mouse_.buttons = 0;
      }
      hid_host_device_close(dev);
    } break;
    case HID_HOST_INTERFACE_EVENT_TRANSFER_ERROR: {
      // the interface stops reporting: drop whatever was held (a stuck
      // button otherwise stays pressed) and try to get the transfers going
      // again; if that fails the user has to re-plug the device
      self->logger.warn("HID transfer error (addr {} iface {}); restarting the interface", params.addr, params.iface_num);
      bool gamepad = false;
      {
        std::lock_guard<std::mutex> lk(self->open_mutex);
        gamepad = self->gamepads.count(dev) > 0;
      }
      if (gamepad) {
        self->emu->on_hid_gamepad_state(GamepadState{}, false);
      } else if (params.proto == HID_PROTOCOL_KEYBOARD) {
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        memset(self->emu->keyboard_.keys, 0, sizeof(self->emu->keyboard_.keys));
      } else if (params.proto == HID_PROTOCOL_MOUSE) {
        std::lock_guard<std::mutex> lk(self->emu->hid_mutex_);
        self->emu->mouse_.buttons = 0;
      }
      hid_host_device_stop(dev);
      if (auto err = hid_host_device_start(dev); err != ESP_OK) {
        self->logger.error("HID interface restart failed: {}", esp_err_to_name(err));
      }
    } break;
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
      self->logger.info("HID {} connected (addr {} iface {} subclass {} proto {})",
                        keyboard ? "keyboard" : mouse ? "mouse" : "device", params.addr, params.iface_num,
                        params.sub_class, params.proto);
      const hid_host_device_config_t cfg = {.callback = &UsbHid::on_interface_event, .callback_arg = self};
      if (auto err = hid_host_device_open(dev, &cfg); err != ESP_OK) {
        self->logger.error("open failed: {}", esp_err_to_name(err));
        continue;
      }
      HidGamepadMap map;
      if (!keyboard && !mouse) {
        // generic HID: gamepads (and anything else) come with a report
        // descriptor; keep the interface only if it looks like a gamepad
        size_t desc_len = 0;
        const uint8_t *desc = hid_host_get_report_descriptor(dev, &desc_len);
        if (desc && desc_len) {
          map = HidGamepadMap::parse(desc, desc_len);
        }
        if (map.buttons.empty() || (map.hats.empty() && map.axes.empty() && map.dpad.empty())) {
          self->logger.info("HID device (addr {} iface {}) is not a gamepad ({} buttons, {} axes, {} hats); ignored",
                            params.addr, params.iface_num, map.buttons.size(), map.axes.size(), map.hats.size());
          hid_host_device_close(dev);
          continue;
        }
        hid_host_dev_info_t info{};
        if (hid_host_get_device_info(dev, &info) == ESP_OK) {
          map.apply_quirks(info.VID, info.PID);
        }
        self->logger.info("HID gamepad {:04x}:{:04x}: {} buttons, {} axes, {} hat(s), {} d-pad usages, {} consumer, "
                          "report ids {}, {} layout",
                          info.VID, info.PID, map.buttons.size(), map.axes.size(), map.hats.size(), map.dpad.size(),
                          map.consumer.size(), map.uses_report_ids ? "yes" : "no",
                          map.layout == HidGamepadMap::Layout::Xbox ? "xbox-style" : "directinput");
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
      } else if (mouse) {
        self->emu->usb_mice_++;
      } else {
        self->emu->usb_gamepads_++;
      }
      std::lock_guard<std::mutex> lk(self->open_mutex);
      self->open_devices.push_back(dev);
      if (!keyboard && !mouse) {
        self->gamepads[dev] = std::move(map);
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
  // both tasks live forever; PSRAM stacks keep internal RAM for DMA users
  if (xTaskCreatePinnedToCoreWithCaps(&UsbHid::usb_lib_task, "usb_events", 4096, hid, 6, &hid->usb_task, 1,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
    logger_.error("could not create the USB event task");
    usb_host_uninstall();
    delete hid;
    return false;
  }
  hid->connect_queue = xQueueCreate(8, sizeof(hid_host_device_handle_t));
  if (xTaskCreatePinnedToCoreWithCaps(&UsbHid::open_worker, "usb_hid", 4096, hid, 5, &hid->open_task, 1,
                                      MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT) != pdPASS) {
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

void Tab5Emu::deinitialize_usb_host() {
  auto *hid = usb_hid_;
  if (!hid) {
    return;
  }
  logger_.info("stopping the USB host");
  // close what we opened, then the class driver, then the library
  {
    std::lock_guard<std::mutex> lk(hid->open_mutex);
    for (auto dev : hid->open_devices) {
      hid_host_device_stop(dev);
      hid_host_device_close(dev);
    }
    hid->open_devices.clear();
  }
  usb_keyboards_ = 0;
  usb_gamepads_ = 0;
  usb_gamepad_ = GamepadState{};
  usb_mice_ = 0;
  {
    std::lock_guard<std::mutex> lk(hid_mutex_);
    memset(keyboard_.keys, 0, sizeof(keyboard_.keys));
    mouse_ = {};
  }
  hid->quit = true;
  if (auto err = hid_host_uninstall(); err != ESP_OK) {
    logger_.warn("hid_host_uninstall: {}", esp_err_to_name(err));
  }
  // the event task sees NO_CLIENTS -> frees devices -> ALL_FREE -> parks
  for (int i = 0; i < 300 && eTaskGetState(hid->usb_task) != eSuspended; i++) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  if (eTaskGetState(hid->usb_task) != eSuspended) {
    // last resort: if the library still holds devices the uninstall below
    // fails and we leave the host installed rather than corrupt it
    logger_.warn("USB event task did not park");
  }
  vTaskDeleteWithCaps(hid->usb_task);
  vTaskDeleteWithCaps(hid->open_task); // blocked on its queue
  vQueueDelete(hid->connect_queue);
  if (auto err = usb_host_uninstall(); err != ESP_OK) {
    logger_.warn("usb_host_uninstall: {}", esp_err_to_name(err));
  }
  delete hid;
  usb_hid_ = nullptr;
  logger_.info("USB host stopped");
}

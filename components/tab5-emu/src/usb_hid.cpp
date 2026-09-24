// USB host for HID input on the Tab5's USB-A jack: espp::UsbHost drives the
// USB Host library and the HID class driver; each device's report descriptor
// is read with the hid-rp runtime report map and its reports are decoded
// into keyboard / mouse / gamepad state for the HAL.
//
// The host runs the root port in full-speed-only mode so devices behind a
// hub work (IDF has no transaction translator), and its tasks' stacks live
// in PSRAM (internal RAM is what the emulators and DMA compete for).
#include "tab5-emu.hpp"

#include <map>
#include <variant>

#include "hid-rp-report-map.hpp"
#include "usb_host.hpp"

struct Tab5Emu::UsbHid {
  Tab5Emu *emu{nullptr};
  std::unique_ptr<espp::UsbHost> host;
  espp::Logger logger{{.tag = "UsbHid", .level = espp::Logger::Verbosity::DEBUG}}; // DEBUG while validating the hid-rp decoders

  // one decoder per opened device (keyed by the HidDevice pointer)
  using Decoder = std::variant<std::monostate, espp::hid_rp::GamepadDecoder, espp::hid_rp::KeyboardDecoder,
                               espp::hid_rp::MouseDecoder>;
  std::mutex mutex;
  std::map<const espp::UsbHost::HidDevice *, Decoder> decoders;
  // a keyboard can be several HID interfaces (boot + NKRO / media); each keeps
  // its own last report and the HAL sees the union
  std::map<const espp::UsbHost::HidDevice *, espp::hid_rp::KeyboardReport> keyboard_states;
  espp::hid_rp::GamepadReport last_gamepad{}; // for the mapping log
  espp::hid_rp::KeyboardReport last_keyboard{};

  void publish_keyboard() {
    espp::hid_rp::KeyboardReport merged{};
    for (const auto &[dev, st] : keyboard_states) {
      for (size_t i = 0; i < sizeof(merged.keys); i++) merged.keys[i] |= st.keys[i];
    }
    if (memcmp(merged.keys, last_keyboard.keys, sizeof(merged.keys)) != 0) {
      std::string usages;
      for (int u = 0; u < 256; u++) {
        if (merged.pressed(static_cast<uint8_t>(u))) usages += fmt::format("{:#x} ", u);
      }
      logger.debug("keyboard: [{}]", usages);
      last_keyboard = merged;
    }
    emu->on_hid_keyboard(merged);
  }

  void on_connected(const std::shared_ptr<espp::UsbHost::HidDevice> &device) {
    const auto &info = device->info();
    const auto &params = device->params();
    auto map = espp::hid_rp::ReportMap::parse(device->report_descriptor());
    if (!map) {
      logger.info("HID {:04x}:{:04x} iface {}: no usable report descriptor; ignored", info.vid, info.pid,
                  params.interface_number);
      std::error_code ec;
      device->stop(ec);
      return;
    }
    Decoder decoder;
    const char *kind = "device";
    if (espp::hid_rp::KeyboardDecoder::looks_like_keyboard(*map)) {
      for (const auto &f : map->fields()) { // before the move below
        logger.debug("  field: report {} bits {}+{} page {:#x} usage {:#x}..{:#x} logical {}..{} {}", f.report_id,
                     f.bit_offset, f.bit_size, f.usage_page, f.usage, f.usage_max, f.logical_min, f.logical_max,
                     f.array ? "array" : "variable");
      }
      decoder = espp::hid_rp::KeyboardDecoder(std::move(*map));
      kind = "keyboard";
      emu->usb_keyboards_++;
    } else if (espp::hid_rp::MouseDecoder::looks_like_mouse(*map)) {
      decoder = espp::hid_rp::MouseDecoder(std::move(*map));
      kind = "mouse";
      emu->usb_mice_++;
    } else if (espp::hid_rp::GamepadDecoder::looks_like_gamepad(*map)) {
      espp::hid_rp::GamepadDecoder pad(std::move(*map));
      pad.apply_quirks(info.vid, info.pid);
      logger.info("HID gamepad {:04x}:{:04x} '{}': {} fields, report ids {}, {} layout", info.vid, info.pid,
                  info.product, pad.map().fields().size(), pad.map().uses_report_ids() ? "yes" : "no",
                  pad.quirks().layout == espp::hid_rp::GamepadDecoder::Layout::Xbox ? "xbox-style" : "directinput");
      decoder = std::move(pad);
      kind = "gamepad";
      emu->usb_gamepads_++;
    } else {
      logger.info("HID {:04x}:{:04x} iface {} is neither keyboard, mouse nor gamepad; ignored", info.vid, info.pid,
                  params.interface_number);
      std::error_code ec;
      device->stop(ec);
      return;
    }
    logger.info("HID {} connected ({:04x}:{:04x} '{}' addr {} iface {})", kind, info.vid, info.pid, info.product,
                params.address, params.interface_number);
    {
      std::lock_guard<std::mutex> lk(mutex);
      decoders[device.get()] = std::move(decoder);
    }
    auto *dev = device.get();
    device->set_input_callback([this, dev](std::span<const uint8_t> data) { on_input(dev, data); });
  }

  void on_disconnected(const std::shared_ptr<espp::UsbHost::HidDevice> &device) {
    Decoder decoder;
    {
      std::lock_guard<std::mutex> lk(mutex);
      auto it = decoders.find(device.get());
      if (it == decoders.end()) {
        return; // one we ignored
      }
      decoder = std::move(it->second);
      decoders.erase(it);
    }
    const char *kind = "device";
    if (std::holds_alternative<espp::hid_rp::KeyboardDecoder>(decoder)) {
      kind = "keyboard";
      emu->usb_keyboards_--;
      std::lock_guard<std::mutex> lk(mutex);
      keyboard_states.erase(device.get());
      publish_keyboard();
    } else if (std::holds_alternative<espp::hid_rp::MouseDecoder>(decoder)) {
      kind = "mouse";
      emu->usb_mice_--;
    } else if (std::holds_alternative<espp::hid_rp::GamepadDecoder>(decoder)) {
      kind = "gamepad";
      emu->usb_gamepads_--;
    }
    logger.info("HID {} disconnected", kind);
    clear_state_of(decoder);
  }

  void clear_state_of(const Decoder &decoder) {
    if (std::holds_alternative<espp::hid_rp::KeyboardDecoder>(decoder)) {
      // handled per device (keyboard_states) by the callers
    } else if (std::holds_alternative<espp::hid_rp::MouseDecoder>(decoder)) {
      std::lock_guard<std::mutex> lk(emu->hid_mutex_);
      emu->mouse_.buttons = 0;
    } else if (std::holds_alternative<espp::hid_rp::GamepadDecoder>(decoder)) {
      emu->on_hid_gamepad_state(GamepadState{}, false, GamepadAxes{});
    }
  }

  // dispatch task: decode and hand the state to the HAL
  void on_input(const espp::UsbHost::HidDevice *dev, std::span<const uint8_t> data) {
    std::lock_guard<std::mutex> lk(mutex);
    auto it = decoders.find(dev);
    if (it == decoders.end()) {
      return;
    }
    auto &decoder = it->second;
    if (auto *kb = std::get_if<espp::hid_rp::KeyboardDecoder>(&decoder)) {
      {
        std::string hex;
        for (size_t i = 0; i < data.size() && i < 24; i++) hex += fmt::format("{:02x} ", data[i]);
        logger.debug("keyboard iface {} raw [{}]", dev->params().interface_number, hex);
      }
      espp::hid_rp::KeyboardReport report;
      if (kb->decode(data, report)) {
        keyboard_states[dev] = report;
        publish_keyboard();
      }
    } else if (auto *mouse = std::get_if<espp::hid_rp::MouseDecoder>(&decoder)) {
      espp::hid_rp::MouseReport report;
      if (mouse->decode(data, report)) {
        emu->on_hid_mouse(report);
      }
    } else if (auto *pad = std::get_if<espp::hid_rp::GamepadDecoder>(&decoder)) {
      espp::hid_rp::GamepadReport report;
      if (pad->decode(data, report)) {
        // mapping diagnostics: the raw buttons / consumer controls on change
        if (report.buttons != last_gamepad.buttons ||
            memcmp(report.consumer, last_gamepad.consumer, sizeof(report.consumer)) != 0 ||
            report.hat != last_gamepad.hat) {
          std::string btns, cons;
          for (int n = 1; n < 64; n++) {
            if (report.buttons & (uint64_t{1} << n)) btns += fmt::format("{} ", n);
          }
          for (auto u : report.consumer) {
            if (u) cons += fmt::format("{:#x} ", u);
          }
          logger.debug("gamepad: buttons [{}] hat {} consumer [{}] sticks {} {} {} {}", btns, report.hat, cons,
                       report.lx, report.ly, report.rx, report.ry);
        }
        last_gamepad = report;
        // Nintendo-style positions for the emulators: A = east, B = south, X =
        // north, Y = west; shoulders and triggers both count as L / R
        GamepadState state{};
        state.a = report.east;
        state.b = report.south;
        state.x = report.north;
        state.y = report.west;
        state.l = report.l1 || report.l2;
        state.r = report.r1 || report.r2;
        state.select = report.select;
        state.start = report.start;
        state.up = report.up;
        state.down = report.down;
        state.left = report.left;
        state.right = report.right;
        const bool menu = report.home || (report.select && report.start);
        emu->on_hid_gamepad_state(state, menu, GamepadAxes{report.lx, report.ly, report.rx, report.ry});
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
  hid->host = std::make_unique<espp::UsbHost>(espp::UsbHost::Config{
      .on_device_connected = [hid](const auto &device) { hid->on_connected(device); },
      .on_device_disconnected = [hid](const auto &device) { hid->on_disconnected(device); },
      .auto_start = true,
      .task_priority = 5,
      .dispatch_task_stack_size = 8 * 1024,
      .full_speed_only = true, // devices behind a hub: IDF has no transaction translator
      .task_stack_alloc_caps = MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT,
      .log_level = espp::Logger::Verbosity::WARN,
  });
  std::error_code ec;
  if (!hid->host->initialize(ec)) {
    logger_.error("USB host: {}", ec.message());
    delete hid;
    return false;
  }
  usb_hid_ = hid;
  logger_.info("USB host ready (HID keyboard / mouse / gamepad, hubs at full speed)");
  return true;
}

void Tab5Emu::deinitialize_usb_host() {
  auto *hid = usb_hid_;
  if (!hid) {
    return;
  }
  logger_.info("stopping the USB host");
  std::error_code ec;
  if (!hid->host->deinitialize(ec)) {
    logger_.error("USB host teardown failed: {}; keeping it", ec.message());
    return;
  }
  delete hid;
  usb_hid_ = nullptr;
  usb_keyboards_ = 0;
  usb_mice_ = 0;
  usb_gamepads_ = 0;
  {
    std::lock_guard<std::mutex> lk(hid_mutex_);
    memset(keyboard_.keys, 0, sizeof(keyboard_.keys));
    mouse_.buttons = 0;
    usb_gamepad_ = GamepadState{};
    usb_gamepad_axes_ = GamepadAxes{};
  }
  logger_.info("USB host stopped");
}

void Tab5Emu::on_hid_keyboard(const espp::hid_rp::KeyboardReport &report) {
  static_assert(sizeof(report.keys) == sizeof(KeyboardState::keys));
  std::lock_guard<std::mutex> lk(hid_mutex_);
  memcpy(keyboard_.keys, report.keys, sizeof(keyboard_.keys));
}

void Tab5Emu::on_hid_mouse(const espp::hid_rp::MouseReport &report) {
  std::lock_guard<std::mutex> lk(hid_mutex_);
  mouse_.buttons = static_cast<uint8_t>(report.buttons & 0x07);
  mouse_.dx += report.dx;
  mouse_.dy += report.dy;
  mouse_.wheel += report.wheel;
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

#include "gui.hpp"

#include "emu-ui/saves.hpp"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include <esp_app_desc.h>
#include <esp_ota_ops.h>
#include <esp_heap_caps.h>

#include "driver/jpeg_decode.h"

#include "lvgl_private.h" // (lv_image_decoder_dsc_t)

using namespace std::chrono_literals;

namespace {

// the Console of a console key (rom_info's ConsoleInfo::key)
std::optional<Console> console_of_key(const std::string &key) {
  for (int c = 0; c < static_cast<int>(Console::UNKNOWN); c++) {
    if (key == console_info(static_cast<Console>(c)).key) return static_cast<Console>(c);
  }
  return std::nullopt;
}

std::string version_text() {
  const auto *app = esp_app_get_description();
  const auto *part = esp_ota_get_running_partition();
  return fmt::format("{}  {}", app ? app->version : "", part ? part->label : "");
}

struct SettingLimit {
  const char *name;
  int min, max;
};
// (the same ranges as the launcher's own controls)
constexpr SettingLimit kSettingLimits[] = {
    {"volume", 0, 100}, {"muted", 0, 1}, {"brightness", 5, 100}, {"video", 0, 2}, {"resume", 0, 1}};

} // namespace

Gui::Gui(const Config &config)
    : metadata_filename_(config.metadata_filename)
    , save_dir_(config.save_dir)
    , logger_({.tag = "Gui", .level = config.log_level}) {
  {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    launcher_ = std::make_unique<emu_ui::Launcher>(emu_ui::Launcher::Config{
        .metrics = emu_ui::Metrics::large_1280x720(),
        .brand = "tab5-emu",
        .keypad = Tab5Emu::get().keypad_indev(),
        .cover = [this](size_t i) { return cover(i); },
        .console_art = [this](const std::string &key) { return console_art(key); },
        .on_play =
            [this](size_t i) {
              focused_rom_ = i;
              ready_to_play_ = true;
            },
        .on_select = [this](size_t i) { focused_rom_ = i; },
        .settings = settings_rows(),
        .version = version_text(),
    });
    update_rom_list();
    launcher_->show();
  }
  task_.periodic(16 * 1000);
  using namespace std::placeholders;
  espp::EventManager::get().add_subscriber(volume_changed_topic, "gui", std::bind(&Gui::on_volume, this, _1), 4 * 1024);
  espp::EventManager::get().add_subscriber(battery_topic, "gui", std::bind(&Gui::on_battery, this, _1), 5 * 1024);
}

Gui::~Gui() {
  espp::EventManager::get().remove_subscriber(volume_changed_topic, "gui");
  espp::EventManager::get().remove_subscriber(battery_topic, "gui");
  task_.stop();
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  launcher_.reset();
  if (boxart_buf_) lv_draw_buf_destroy(boxart_buf_);
  for (auto &[key, buf] : console_art_) {
    if (buf) lv_draw_buf_destroy(buf);
  }
}

void Gui::pause() {
  paused_ = true;
  task_.stop();
  // wait out a timer callback already in lv_timer_handler() (stop() does not)
  std::lock_guard<std::recursive_mutex> lk(mutex_);
}

void Gui::resume() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  // (a game started from the PC, emulink launch, may have left the picker open)
  launcher_->close_picker();
  update_rom_list();
  launcher_->refresh_settings();
  launcher_->show();
  lv_obj_invalidate(launcher_->screen());
  task_.periodic(16 * 1000);
  paused_ = false;
}

void Gui::update() {
  if (paused_) return;
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  // the USB link can start or end the drive mode too (emulink drive on /
  // off; an eject alone does not, the hand-over is never automatic): follow
  // it, and pick up what the PC copied over
  const bool drive_on = Tab5Emu::get().usb_msc_host_has_card();
  if (drive_on != usb_drive_shown_) {
    usb_drive_shown_ = drive_on;
    usb_note_ = drive_on ? "The PC has the SD card. Eject it there, then switch this off." : "";
    launcher_->set_play_enabled(!drive_on);
    launcher_->refresh_settings();
    if (!drive_on) reload_rom_list();
  }
  if (battery_dirty_.exchange(false)) {
    const auto status = Tab5Emu::get().battery_status();
    // no battery in the bay (running from USB): "--%", not an empty battery
    launcher_->set_battery(status.is_present ? (int)std::lround(status.charge_percent) : -1,
                           status.is_present && status.is_charging);
  }
  lv_timer_handler();
}

std::vector<emu_ui::Row> Gui::settings_rows() {
  using K = emu_ui::Row::Kind;
  auto &emu = Tab5Emu::get();
  // (saved to the SD card by the main loop once the changes stop)
  std::vector<emu_ui::Row> rows;
  rows.push_back({.label = "Volume", .kind = K::SLIDER, .step = 5,
                  .get = [&emu] { return (int)std::lround(emu.volume()); },
                  .set = [&emu](int v) {
                    emu.volume(v);
                    AppSettings::get().modify([v](Settings &s) { s.volume = v; });
                  },
                  .symbol = LV_SYMBOL_VOLUME_MAX});
  rows.push_back({.label = "Mute", .kind = K::TOGGLE, .get = [&emu] { return emu.is_muted() ? 1 : 0; },
                  .set = [&emu](int v) {
                    emu.mute(v != 0);
                    AppSettings::get().modify([v](Settings &s) { s.muted = v != 0; });
                  },
                  .symbol = LV_SYMBOL_MUTE});
  rows.push_back({.label = "Brightness", .kind = K::SLIDER, .min = 5, .step = 5,
                  .get = [&emu] { return (int)std::lround(emu.brightness()); },
                  .set = [&emu](int v) {
                    emu.brightness(v);
                    AppSettings::get().modify([v](Settings &s) { s.brightness = v; });
                  },
                  .symbol = LV_SYMBOL_EYE_OPEN});
  rows.push_back({.label = "Video scaling", .kind = K::CHOICE, .choices = {"Original", "Fit", "Fill"},
                  .get = [&emu] { return (int)emu.video_setting(); },
                  .set = [&emu](int v) {
                    emu.video_setting(static_cast<VideoSetting>(v));
                    AppSettings::get().modify([v](Settings &s) { s.video_setting = v; });
                  },
                  .symbol = LV_SYMBOL_IMAGE});
  rows.push_back({.label = "Resume last save", .kind = K::TOGGLE,
                  .get = [] { return AppSettings::get().get_settings().resume_last_save ? 1 : 0; },
                  .set = [](int v) { AppSettings::get().modify([v](Settings &s) { s.resume_last_save = v != 0; }); },
                  .symbol = LV_SYMBOL_REFRESH});
  rows.push_back({.label = "USB drive", .kind = K::TOGGLE,
                  .get = [&emu] { return emu.usb_msc_host_has_card() ? 1 : 0; },
                  .set = [this](int v) { set_usb_drive(v != 0); },
                  .note =
                      [this] {
                        return usb_note_.empty() ? std::string("The SD card as a USB drive on the USB-C port") : usb_note_;
                      },
                  .symbol = LV_SYMBOL_USB});
  return rows;
}

// USB drive mode: hand the SD card to a PC. Nothing may touch the card
// meanwhile, so Play is off; when it ends the list is reloaded so anything
// copied over shows up.
void Gui::set_usb_drive(bool on) {
  auto &emu = Tab5Emu::get();
  if (on) {
    if (!emu.initialize_usb_msc()) {
      usb_note_ = "The USB drive could not start (no SD card?)";
      return;
    }
    usb_drive_shown_ = true;
    usb_note_ = "Plug the USB-C port into a PC. Eject there, then switch this off.";
    launcher_->set_play_enabled(false);
  } else {
    emu.deinitialize_usb_msc();
    usb_drive_shown_ = false;
    usb_note_.clear();
    launcher_->set_play_enabled(true);
    reload_rom_list();
  }
}

void Gui::on_volume(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  launcher_->refresh_settings();
}

void Gui::on_battery(const std::vector<uint8_t> &) { battery_dirty_ = true; }

void Gui::update_rom_list() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  std::error_code ec;
  auto path = std::string(Tab5Emu::mount_point) + "/" + metadata_filename_;
  auto mtime = std::filesystem::last_write_time(path, ec);
  // (the saves are counted each time: a game played since may have saved)
  auto saves = emu_ui::count_saves(save_dir_);
  if (!ec && mtime == metadata_last_modified_ && !rom_infos_.empty()) {
    if (saves != save_counts_) {
      save_counts_ = std::move(saves);
      const int f = focused_rom_;
      show_games(f >= 0 && f < (int)rom_infos_.size() ? rom_infos_[f].rom_path : std::string());
    }
    return;
  }
  save_counts_ = std::move(saves);
  metadata_last_modified_ = mtime;
  // keep the selected game selected across a reload (its index may change);
  // at boot that is the last game played
  std::string selected_path;
  if (focused_rom_ >= 0 && focused_rom_ < (int)rom_infos_.size()) {
    selected_path = rom_infos_[focused_rom_].rom_path;
  } else {
    selected_path = AppSettings::get().get_settings().last_rom;
  }
  rom_infos_ = parse_metadata(metadata_filename_);
  // grouped by console, in Console order; metadata.csv order within a console
  std::stable_sort(rom_infos_.begin(), rom_infos_.end(), [](const RomInfo &a, const RomInfo &b) {
    return console_of(a.platform) < console_of(b.platform);
  });
  show_games(selected_path);
}

void Gui::show_games(const std::string &selected_path) {
  int selected = -1;
  std::vector<emu_ui::Game> games;
  games.reserve(rom_infos_.size());
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    const auto &rom = rom_infos_[i];
    const auto &info = console_info(console_of(rom.platform));
    const auto it = save_counts_.find(std::filesystem::path(rom.rom_path).stem().string());
    games.push_back({.title = rom.name, .console_key = info.key, .console_name = info.name,
                     .console_short = info.short_name, .saves = it == save_counts_.end() ? 0 : it->second});
    if (selected < 0 && rom.rom_path == selected_path) selected = i;
  }
  focused_rom_ = -1;
  launcher_->set_games(std::move(games), selected);
  focused_rom_ = launcher_->selected();
}

std::vector<Console> Gui::consoles() const {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  std::vector<Console> out;
  for (const auto &key : launcher_->consoles()) {
    if (auto c = console_of_key(key)) out.push_back(*c);
  }
  return out;
}

std::optional<Console> Gui::console_filter() const {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto f = launcher_->filter();
  return f ? console_of_key(*f) : std::nullopt;
}

bool Gui::set_console_filter(std::optional<Console> console) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  return launcher_->set_filter(console ? std::optional<std::string>(console_info(*console).key) : std::nullopt);
}

bool Gui::select_rom_matching(std::string_view text) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  // an exact path or name first (one ROM's path can contain another's), then
  // the first game containing the text
  for (const bool exact : {true, false}) {
    for (size_t i = 0; i < rom_infos_.size(); i++) {
      const auto &rom = rom_infos_[i];
      if (exact ? (rom.rom_path == text || rom.name == text)
                : (rom.name.find(text) != std::string::npos || rom.rom_path.find(text) != std::string::npos)) {
        // (a game started from the link must not leave the picker over the list)
        launcher_->close_picker();
        launcher_->select(i); // (lifts a filter that hides it)
        focused_rom_ = i;
        return true;
      }
    }
  }
  return false;
}

std::string Gui::settings_json() const {
  auto &emu = Tab5Emu::get();
  const auto s = AppSettings::get().get_settings();
  std::string limits;
  for (const auto &l : kSettingLimits) {
    limits += fmt::format("{}\"{}\":[{},{}]", limits.empty() ? "" : ",", l.name, l.min, l.max);
  }
  return fmt::format("{{\"settings\":{{\"volume\":{},\"muted\":{},\"brightness\":{},\"video\":{},\"resume\":{}}},"
                     "\"limits\":{{{}}}}}",
                     (int)std::lround(emu.volume()), emu.is_muted() ? 1 : 0, (int)std::lround(emu.brightness()),
                     (int)emu.video_setting(), s.resume_last_save ? 1 : 0, limits);
}

bool Gui::set_setting(const std::string &name, int value) {
  const auto *limit = std::find_if(std::begin(kSettingLimits), std::end(kSettingLimits),
                                   [&](const SettingLimit &l) { return name == l.name; });
  if (limit == std::end(kSettingLimits) || value < limit->min || value > limit->max) {
    return false;
  }
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  auto &emu = Tab5Emu::get();
  // (saved to the SD card by the main loop once the changes stop, as from the launcher)
  AppSettings::get().modify([&](Settings &s) {
    if (name == "volume") s.volume = value;
    else if (name == "muted") s.muted = value != 0;
    else if (name == "brightness") s.brightness = value;
    else if (name == "video") s.video_setting = value;
    else if (name == "resume") s.resume_last_save = value != 0;
  });
  if (name == "volume") emu.volume(value);
  else if (name == "muted") emu.mute(value != 0);
  else if (name == "brightness") emu.brightness(value);
  else if (name == "video") emu.video_setting(static_cast<VideoSetting>(value));
  launcher_->refresh_settings(); // the launcher shows it
  return true;
}

Gui::Snapshot Gui::snapshot() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  Snapshot shot;
  auto *screen = lv_screen_active();
  if (!screen) {
    return shot;
  }
  lv_obj_update_layout(screen);
  const int32_t w = lv_obj_get_width(screen), h = lv_obj_get_height(screen);
  const uint32_t stride = lv_draw_buf_width_to_stride(w, LV_COLOR_FORMAT_RGB565);
  shot.pixels.resize(stride * h);
  lv_draw_buf_t buf;
  if (lv_draw_buf_init(&buf, w, h, LV_COLOR_FORMAT_RGB565, stride, shot.pixels.data(), shot.pixels.size()) !=
          LV_RESULT_OK ||
      lv_snapshot_take_to_draw_buf(screen, LV_COLOR_FORMAT_RGB565, &buf) != LV_RESULT_OK) {
    shot.pixels.clear();
    return shot;
  }
  // (rows packed: the reader expects w * 2 bytes per row)
  if (stride != static_cast<uint32_t>(w) * 2) {
    for (int32_t y = 1; y < h; y++) {
      memmove(shot.pixels.data() + y * w * 2, shot.pixels.data() + y * stride, w * 2);
    }
    shot.pixels.resize(w * h * 2);
  }
  shot.width = w;
  shot.height = h;
  return shot;
}

// emu-ui's launcher shows the cover in a 360 px square (LV_IMAGE_ALIGN_CONTAIN).
// Scaled to fit it once, here, LVGL draws the cover as it is rather than
// scaling it in software on every redraw.
constexpr int kCoverBox = 360;

// The selected game's boxart, decoded once per selection (the launcher has
// already let go of the previous one).
const void *Gui::cover(size_t index) {
  if (boxart_buf_) {
    lv_draw_buf_destroy(boxart_buf_);
    boxart_buf_ = nullptr;
  }
  if (index < rom_infos_.size() && !rom_infos_[index].boxart_path.empty()) {
    boxart_buf_ = decode_image(rom_infos_[index].boxart_path, kCoverBox);
  }
  return boxart_buf_;
}

// boxart/consoles/<key>.jpg on the card, if there is one (kept: the launcher
// asks again on every selection)
const void *Gui::console_art(const std::string &key) {
  if (auto it = console_art_.find(key); it != console_art_.end()) return it->second;
  lv_draw_buf_t *buf = nullptr;
  const std::string path = std::string(Tab5Emu::mount_point) + "/boxart/consoles/" + key + ".jpg";
  std::error_code ec;
  if (std::filesystem::exists(path, ec)) buf = decode_image(path);
  console_art_[key] = buf;
  return buf;
}

namespace {

// The P4's JPEG decoder (made on first use; nullptr if it cannot be)
jpeg_decoder_handle_t jpeg_engine() {
  static jpeg_decoder_handle_t engine = nullptr;
  static bool tried = false;
  if (!tried) {
    tried = true;
    jpeg_decode_engine_cfg_t cfg{};
    cfg.timeout_ms = 500;
    if (jpeg_new_decoder_engine(&cfg, &engine) != ESP_OK) engine = nullptr;
  }
  return engine;
}

// The frame header of a baseline JPEG in memory: the offset of its height
// (then width) field and its MCU size; -1 if there is none (progressive...)
int find_frame_header(const uint8_t *d, size_t n, int &mcu_w, int &mcu_h) {
  size_t i = 2; // (after SOI)
  while (i + 4 <= n) {
    if (d[i] != 0xFF) return -1;
    const uint8_t m = d[i + 1];
    if (m == 0xFF) { // fill byte
      i++;
      continue;
    }
    if (m == 0x01 || (m >= 0xD0 && m <= 0xD7)) {
      i += 2;
      continue;
    }
    const size_t len = (d[i + 2] << 8) | d[i + 3];
    if (m == 0xC0 || m == 0xC1) {
      // P Y(2) X(2) Nf, then per component: id, Hi:Vi, Tq; the decoder's MCU
      // is the first component's sampling factors x 8
      if (len < 11 || i + 2 + len > n) return -1;
      const int h = d[i + 11] >> 4, v = d[i + 11] & 0x0f;
      if (!h || !v) return -1;
      mcu_w = 8 * h;
      mcu_h = 8 * v;
      return static_cast<int>(i + 5);
    }
    if (m == 0xDA || m == 0xD9 || (m >= 0xC2 && m <= 0xCF && m != 0xC4 && m != 0xC8 && m != 0xCC)) {
      return -1; // a scan first, or not baseline
    }
    i += 2 + len;
  }
  return -1;
}

struct FreeDeleter {
  void operator()(void *p) const { free(p); }
};

} // namespace

// A new RGB565 draw buffer of `src` (rows `stride` bytes apart), scaled
// (bilinear) to fit a box x box square with its longer side box long, or as
// it is (box 0)
lv_draw_buf_t *Gui::fit_image(const uint8_t *src, int w, int h, int stride, int box) {
  int dw = w, dh = h;
  if (box > 0) {
    if (w >= h) {
      dw = box;
      dh = std::max(1, (h * box + w / 2) / w);
    } else {
      dh = box;
      dw = std::max(1, (w * box + h / 2) / h);
    }
  }
  auto *out = lv_draw_buf_create(dw, dh, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  if (!out) return nullptr;
  if (dw == w && dh == h) {
    for (int y = 0; y < h; y++) memcpy(out->data + y * out->header.stride, src + y * stride, w * 2);
    return out;
  }
  // Separable bilinear: each source row is interpolated across once (into a
  // two-row cache in internal RAM), then output rows blend two of those. The
  // pixels spread as 0x07E0F81F (green, red, blue, with room above each) so
  // one multiply weighs all three, with 5 bit weights.
  const int32_t step_x = (w << 16) / dw, step_y = (h << 16) / dh;
  auto src_pos = [](int i, int32_t step, int n) {
    return std::clamp<int32_t>(i * step + step / 2 - 0x8000, 0, (n - 1) << 16);
  };
  constexpr uint32_t kMask = 0x07E0F81Fu;
  auto spread = [](uint32_t c) { return (c | (c << 16)) & kMask; };
  // per output column: the two source columns and the weight of the second
  const size_t bytes = static_cast<size_t>(dw) * (2 * sizeof(uint32_t) + 2 * sizeof(uint16_t) + 1);
  std::unique_ptr<uint8_t, FreeDeleter> scratch(
      static_cast<uint8_t *>(heap_caps_malloc(bytes, MALLOC_CAP_INTERNAL | MALLOC_CAP_8BIT)));
  if (!scratch) scratch.reset(static_cast<uint8_t *>(malloc(bytes)));
  if (!scratch) {
    lv_draw_buf_destroy(out);
    return nullptr;
  }
  auto *row_a = reinterpret_cast<uint32_t *>(scratch.get());
  auto *row_b = row_a + dw;
  auto *xa = reinterpret_cast<uint16_t *>(row_b + dw);
  auto *xb = xa + dw;
  auto *wx = reinterpret_cast<uint8_t *>(xb + dw);
  for (int x = 0; x < dw; x++) {
    const int32_t sx = src_pos(x, step_x, w);
    xa[x] = sx >> 16;
    xb[x] = std::min(xa[x] + 1, w - 1);
    wx[x] = (sx >> 11) & 31;
  }
  auto across = [&](int sy, uint32_t *dst) {
    const auto *r = reinterpret_cast<const uint16_t *>(src + sy * stride);
    for (int x = 0; x < dw; x++) {
      const uint32_t f = wx[x];
      dst[x] = ((spread(r[xa[x]]) * (32 - f) + spread(r[xb[x]]) * f) >> 5) & kMask;
    }
  };
  int have_a = -1, have_b = -1; // the source rows in row_a / row_b
  for (int y = 0; y < dh; y++) {
    const int32_t sy = src_pos(y, step_y, h);
    const int y0 = sy >> 16, y1 = std::min(y0 + 1, h - 1);
    if (have_a != y0) {
      if (have_b == y0) {
        std::swap(row_a, row_b);
        std::swap(have_a, have_b);
      } else {
        across(y0, row_a);
        have_a = y0;
      }
    }
    if (have_b != y1) {
      across(y1, row_b);
      have_b = y1;
    }
    const uint32_t f = (sy >> 11) & 31;
    auto *row = reinterpret_cast<uint16_t *>(out->data + y * out->header.stride);
    for (int x = 0; x < dw; x++) {
      const uint32_t c = ((row_a[x] * (32 - f) + row_b[x] * f) >> 5) & kMask;
      row[x] = static_cast<uint16_t>(c | (c >> 16));
    }
  }
  return out;
}

// A baseline JPEG through the P4's JPEG decoder, straight to RGB565: a cover
// decoded and scaled takes ~30 ms, where LVGL's software decoder took ~75 ms
// for the decode alone. nullptr if it cannot (decode_image() then falls back
// to LVGL's).
lv_draw_buf_t *Gui::decode_jpeg_hw(const std::string &path, int box) {
  auto *engine = jpeg_engine();
  if (!engine) return nullptr;
  FILE *f = fopen(path.c_str(), "rb");
  if (!f) return nullptr;
  fseek(f, 0, SEEK_END);
  const long size = ftell(f);
  fseek(f, 0, SEEK_SET);
  if (size < 4) {
    fclose(f);
    return nullptr;
  }
  jpeg_decode_memory_alloc_cfg_t in_cfg{.buffer_direction = JPEG_DEC_ALLOC_INPUT_BUFFER};
  size_t in_capacity = 0;
  std::unique_ptr<uint8_t, FreeDeleter> in(static_cast<uint8_t *>(jpeg_alloc_decoder_mem(size, &in_cfg, &in_capacity)));
  const bool read = in && fread(in.get(), 1, size, f) == static_cast<size_t>(size);
  fclose(f);
  if (!read) return nullptr;
  // The decoder refuses a picture whose width x height is not a multiple of
  // 8 (two in five of the covers here), though it decodes whole MCUs anyway:
  // give the frame header the MCU-rounded size (the same MCUs, so the same
  // scan) and crop the result back.
  int mcu_w = 8, mcu_h = 8;
  const int sof = find_frame_header(in.get(), size, mcu_w, mcu_h);
  if (sof < 0) return nullptr;
  uint8_t *dims = in.get() + sof;
  const int h = (dims[0] << 8) | dims[1], w = (dims[2] << 8) | dims[3];
  if (!w || !h) return nullptr;
  const int pw = (w + mcu_w - 1) / mcu_w * mcu_w, ph = (h + mcu_h - 1) / mcu_h * mcu_h;
  dims[0] = ph >> 8;
  dims[1] = ph & 0xff;
  dims[2] = pw >> 8;
  dims[3] = pw & 0xff;
  jpeg_decode_memory_alloc_cfg_t out_cfg{.buffer_direction = JPEG_DEC_ALLOC_OUTPUT_BUFFER};
  size_t out_capacity = 0;
  std::unique_ptr<uint8_t, FreeDeleter> out(
      static_cast<uint8_t *>(jpeg_alloc_decoder_mem(static_cast<size_t>(pw) * ph * 2, &out_cfg, &out_capacity)));
  if (!out) return nullptr;
  jpeg_decode_cfg_t cfg{};
  cfg.output_format = JPEG_DECODE_OUT_FORMAT_RGB565;
  cfg.rgb_order = JPEG_DEC_RGB_ELEMENT_ORDER_BGR; // (little-endian RGB565, as LVGL's)
  cfg.conv_std = JPEG_YUV_RGB_CONV_STD_BT601;
  uint32_t out_size = 0;
  if (jpeg_decoder_process(engine, &cfg, in.get(), size, out.get(), out_capacity, &out_size) != ESP_OK ||
      out_size != static_cast<uint32_t>(pw) * ph * 2) {
    return nullptr;
  }
  return fit_image(out.get(), w, h, pw * 2, box);
}

// The P4's decoder first; else LVGL's JPEG decoder (TJPGD), which only yields
// the image block by block (and LVGL cannot scale a block-decoded image), so
// it is decoded into a full RGB565 buffer here. The POSIX fs driver is
// registered as 'S:' and passes the rest of the path through unchanged.
lv_draw_buf_t *Gui::decode_image(const std::string &path, int box) {
  if (auto *hw = decode_jpeg_hw(path, box)) return hw;
  const std::string src = "S:" + path;
  lv_image_decoder_dsc_t dsc{};
  if (lv_image_decoder_open(&dsc, src.c_str(), nullptr) != LV_RESULT_OK) {
    logger_.warn("image '{}': cannot decode", path);
    return nullptr;
  }
  const uint32_t w = dsc.header.w, h = dsc.header.h;
  auto *out = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  if (!out) {
    lv_image_decoder_close(&dsc);
    return nullptr;
  }
  lv_area_t full = {0, 0, static_cast<int32_t>(w) - 1, static_cast<int32_t>(h) - 1};
  lv_area_t area{};
  area.y1 = LV_COORD_MIN;
  while (lv_image_decoder_get_area(&dsc, &full, &area) == LV_RESULT_OK) {
    if (area.y1 >= static_cast<int32_t>(h)) {
      break;
    }
    const lv_draw_buf_t *blk = dsc.decoded;
    if (!blk || !blk->data) {
      break;
    }
    const int bw = lv_area_get_width(&area), bh = lv_area_get_height(&area);
    for (int y = 0; y < bh; y++) {
      const uint8_t *srow = blk->data + y * blk->header.stride; // RGB888
      uint16_t *drow = reinterpret_cast<uint16_t *>(out->data + (area.y1 + y) * out->header.stride) + area.x1;
      for (int x = 0; x < bw; x++) {
        const uint8_t r = srow[3 * x + 2], g = srow[3 * x + 1], b = srow[3 * x + 0];
        drow[x] = static_cast<uint16_t>(((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3));
      }
    }
    // the last block of the image
    if (area.y2 >= static_cast<int32_t>(h) - 1 && area.x2 >= static_cast<int32_t>(w) - 1) {
      break;
    }
  }
  lv_image_decoder_close(&dsc);
  logger_.info("image '{}': {}x{} (software decoder)", path, w, h);
  if (box > 0) {
    auto *fitted = fit_image(out->data, w, h, out->header.stride, box);
    lv_draw_buf_destroy(out);
    out = fitted;
  }
  return out;
}

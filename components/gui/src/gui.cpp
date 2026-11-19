#include "gui.hpp"

#include <algorithm>
#include <cmath>

#include <esp_app_desc.h>
#include <esp_ota_ops.h>

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

void Gui::on_battery(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto status = Tab5Emu::get().battery_status();
  launcher_->set_battery((int)std::lround(status.charge_percent), status.is_charging);
}

void Gui::update_rom_list() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  std::error_code ec;
  auto path = std::string(Tab5Emu::mount_point) + "/" + metadata_filename_;
  auto mtime = std::filesystem::last_write_time(path, ec);
  if (!ec && mtime == metadata_last_modified_ && !rom_infos_.empty()) {
    return;
  }
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
  int selected = -1;
  std::vector<emu_ui::Game> games;
  games.reserve(rom_infos_.size());
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    const auto &rom = rom_infos_[i];
    const auto &info = console_info(console_of(rom.platform));
    games.push_back({.title = rom.name, .console_key = info.key, .console_name = info.name,
                     .console_short = info.short_name});
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

// The selected game's boxart, decoded once per selection (the launcher has
// already let go of the previous one).
const void *Gui::cover(size_t index) {
  if (boxart_buf_) {
    lv_draw_buf_destroy(boxart_buf_);
    boxart_buf_ = nullptr;
  }
  if (index < rom_infos_.size() && !rom_infos_[index].boxart_path.empty()) {
    boxart_buf_ = decode_image(rom_infos_[index].boxart_path);
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

// LVGL's JPEG decoder (TJPGD) only yields the image block by block, and LVGL
// cannot scale a block-decoded image, so decode it into a full RGB565 buffer
// here and show that. The POSIX fs driver is registered as 'S:' and passes
// the rest of the path through unchanged.
lv_draw_buf_t *Gui::decode_image(const std::string &path) {
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
  logger_.info("image '{}': {}x{}", path, w, h);
  return out;
}

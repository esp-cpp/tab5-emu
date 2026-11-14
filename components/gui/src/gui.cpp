#include "gui.hpp"

#include <cmath>

#include "lvgl_private.h"

#include <algorithm>


using namespace std::chrono_literals;

Gui::Gui(const Config &config)
    : metadata_filename_(config.metadata_filename)
    , logger_({.tag = "Gui", .level = config.log_level}) {
  init_ui();
  update_rom_list();
  update_shared_state();
  // gamepad / keyboard navigation from the first screen on (resume() does
  // the same after a game)
  if (auto *keypad = Tab5Emu::get().keypad_indev(); keypad && group_) {
    lv_indev_set_group(keypad, group_);
    if (focused_rom_ >= 0 && focused_rom_ < (int)rom_buttons_.size() && rom_buttons_[focused_rom_]) {
      lv_group_focus_obj(rom_buttons_[focused_rom_]);
    }
    logger_.info("keypad group attached: {} objects, focused rom {}, focused obj {}", lv_group_get_obj_count(group_),
                 focused_rom_.load(), fmt::ptr(lv_group_get_focused(group_)));
  } else {
    logger_.warn("no keypad input device / group at startup");
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
  deinit_ui();
}

void Gui::pause() {
  paused_ = true;
  task_.stop();
  // wait out a timer callback already in lv_timer_handler() (stop() does not)
  std::lock_guard<std::recursive_mutex> lk(mutex_);
}

void Gui::resume() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  // a game started from the PC (emulink launch) leaves the console picker open
  // behind it: back from the game, the list has the keypad, so close it
  close_console_picker();
  update_rom_list();
  update_shared_state();
  lv_screen_load(screen_);
  lv_obj_invalidate(screen_);
  if (auto *keypad = Tab5Emu::get().keypad_indev(); keypad && group_) {
    lv_indev_set_group(keypad, group_);
    if (focused_rom_ >= 0 && focused_rom_ < (int)rom_buttons_.size() && rom_buttons_[focused_rom_]) {
      lv_group_focus_obj(rom_buttons_[focused_rom_]);
    }
    logger_.info("keypad group attached: {} objects, focused rom {}, focused obj {}", lv_group_get_obj_count(group_),
                 focused_rom_.load(), fmt::ptr(lv_group_get_focused(group_)));
  } else {
    logger_.warn("no keypad input device / group at startup");
  }
  task_.periodic(16 * 1000);
  paused_ = false;
}

void Gui::update() {
  if (!paused_) {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    // the USB link can start or end the drive mode too (emulink drive on /
    // off; an eject alone does not, the hand-over is never automatic): follow
    // it with the switch, and pick up what the PC copied over
    const bool drive_on = Tab5Emu::get().usb_msc_host_has_card();
    if (usb_switch_ && drive_on != lv_obj_has_state(usb_switch_, LV_STATE_CHECKED)) {
      if (drive_on) {
        lv_obj_add_state(usb_switch_, LV_STATE_CHECKED);
        lv_obj_add_state(play_button_, LV_STATE_DISABLED);
        lv_label_set_text(usb_label_, "The PC has the SD card. Eject it there, then switch off.");
      } else {
        lv_obj_remove_state(usb_switch_, LV_STATE_CHECKED);
        lv_obj_remove_state(play_button_, LV_STATE_DISABLED);
        lv_label_set_text(usb_label_, "SD card as a USB drive on the USB-C port");
        reload_rom_list();
      }
    }
    lv_timer_handler();
  }
}

void Gui::update_shared_state() {
  auto &emu = Tab5Emu::get();
  lv_slider_set_value(volume_slider_, (int)emu.volume(), LV_ANIM_OFF);
  lv_slider_set_value(brightness_slider_, (int)emu.brightness(), LV_ANIM_OFF);
  if (emu.is_muted())
    lv_obj_add_state(mute_switch_, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(mute_switch_, LV_STATE_CHECKED);
  lv_dropdown_set_selected(video_dropdown_, (int)emu.video_setting());
  if (AppSettings::get().get_settings().resume_last_save)
    lv_obj_add_state(resume_switch_, LV_STATE_CHECKED);
  else
    lv_obj_remove_state(resume_switch_, LV_STATE_CHECKED);
}

void Gui::on_volume(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  update_shared_state();
}

void Gui::on_battery(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  auto status = Tab5Emu::get().battery_status();
  lv_label_set_text_fmt(battery_label_, "%s %.0f%%  %.2fV", status.is_charging ? LV_SYMBOL_CHARGE : LV_SYMBOL_BATTERY_3,
                        status.charge_percent, status.voltage_v);
}

void Gui::init_ui() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  screen_ = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen_, lv_color_hex(0x101418), 0);
  lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);

  const int W = Tab5Emu::lcd_width(), H = Tab5Emu::lcd_height();

  // header
  auto header = lv_label_create(screen_);
  lv_label_set_text(header, "tab5-emu");
  lv_obj_set_style_text_font(header, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(header, lv_color_white(), 0);
  lv_obj_align(header, LV_ALIGN_TOP_LEFT, 24, 16);

  battery_label_ = lv_label_create(screen_);
  lv_label_set_text(battery_label_, LV_SYMBOL_BATTERY_EMPTY " --%");
  lv_obj_set_style_text_font(battery_label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(battery_label_, lv_color_white(), 0);
  lv_obj_align(battery_label_, LV_ALIGN_TOP_RIGHT, -24, 22);

  // rom list (left)
  rom_list_ = lv_list_create(screen_);
  lv_obj_set_size(rom_list_, W / 3, H - 100);
  lv_obj_align(rom_list_, LV_ALIGN_TOP_LEFT, 24, 76);
  lv_obj_set_style_text_font(rom_list_, &lv_font_montserrat_24, 0);

  // console of the selected game (center top): its SD card art or a badge
  console_area_ = lv_obj_create(screen_);
  lv_obj_remove_style_all(console_area_);
  lv_obj_remove_flag(console_area_, LV_OBJ_FLAG_SCROLLABLE);
  // a tap on it opens the console picker (so does X on a gamepad, F on a keyboard)
  lv_obj_add_flag(console_area_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_flag(console_area_, LV_OBJ_FLAG_OVERFLOW_VISIBLE); // (the filter outline)
  lv_obj_add_event_cb(console_area_, [](lv_event_t *e) {
    static_cast<Gui *>(lv_event_get_user_data(e))->open_console_picker();
  }, LV_EVENT_CLICKED, this);

  lv_obj_set_size(console_area_, 400, 56);
  lv_obj_align(console_area_, LV_ALIGN_TOP_MID, 40, 76);
  lv_obj_set_flex_flow(console_area_, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(console_area_, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);

  // boxart + title (center)
  boxart_ = lv_image_create(screen_);
  lv_obj_set_size(boxart_, 360, 360);
  lv_image_set_inner_align(boxart_, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_align(boxart_, LV_ALIGN_TOP_MID, 40, 144);

  title_label_ = lv_label_create(screen_);
  lv_label_set_text(title_label_, "");
  lv_obj_set_style_text_font(title_label_, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(title_label_, lv_color_white(), 0);
  lv_label_set_long_mode(title_label_, LV_LABEL_LONG_WRAP);
  lv_obj_set_width(title_label_, 400);
  lv_obj_set_style_text_align(title_label_, LV_TEXT_ALIGN_CENTER, 0);
  lv_obj_align_to(title_label_, boxart_, LV_ALIGN_OUT_BOTTOM_MID, 0, 16);

  play_button_ = lv_button_create(screen_);
  lv_obj_set_size(play_button_, 240, 72);
  lv_obj_align(play_button_, LV_ALIGN_BOTTOM_MID, 40, -32);
  auto play_label = lv_label_create(play_button_);
  lv_label_set_text(play_label, LV_SYMBOL_PLAY "  Play");
  lv_obj_set_style_text_font(play_label, &lv_font_montserrat_24, 0);
  lv_obj_center(play_label);
  lv_obj_add_event_cb(play_button_, event_callback, LV_EVENT_CLICKED, this);

  // settings (right)
  auto settings = lv_obj_create(screen_);
  lv_obj_set_size(settings, W / 4, H - 100);
  lv_obj_align(settings, LV_ALIGN_TOP_RIGHT, -24, 76);
  lv_obj_set_flex_flow(settings, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(settings, 18, 0);

  auto make_label = [&](const char *text) {
    auto l = lv_label_create(settings);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_20, 0);
    return l;
  };

  make_label(LV_SYMBOL_VOLUME_MAX "  Volume");
  volume_slider_ = lv_slider_create(settings);
  lv_obj_set_width(volume_slider_, lv_pct(90));
  lv_slider_set_range(volume_slider_, 0, 100);
  lv_obj_add_event_cb(volume_slider_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  make_label(LV_SYMBOL_MUTE "  Mute");
  mute_switch_ = lv_switch_create(settings);
  lv_obj_add_event_cb(mute_switch_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  make_label(LV_SYMBOL_EYE_OPEN "  Brightness");
  brightness_slider_ = lv_slider_create(settings);
  lv_obj_set_width(brightness_slider_, lv_pct(90));
  lv_slider_set_range(brightness_slider_, 5, 100);
  lv_obj_add_event_cb(brightness_slider_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  make_label(LV_SYMBOL_USB "  USB drive");
  usb_switch_ = lv_switch_create(settings);
  lv_obj_add_event_cb(usb_switch_, event_callback, LV_EVENT_VALUE_CHANGED, this);
  usb_label_ = lv_label_create(settings);
  lv_label_set_text(usb_label_, "SD card as a USB drive on the USB-C port");
  lv_obj_set_width(usb_label_, lv_pct(95));
  lv_label_set_long_mode(usb_label_, LV_LABEL_LONG_WRAP);

  make_label(LV_SYMBOL_IMAGE "  Video scaling");
  video_dropdown_ = lv_dropdown_create(settings);
  lv_dropdown_set_options(video_dropdown_, "Original\nFit\nFill");
  lv_obj_set_width(video_dropdown_, lv_pct(90));
  lv_obj_add_event_cb(video_dropdown_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  make_label(LV_SYMBOL_REFRESH "  Resume last save");
  resume_switch_ = lv_switch_create(settings);
  lv_obj_add_event_cb(resume_switch_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  // gamepad / keyboard focus order (see Tab5Emu::keypad_indev()): the ROM
  // buttons come first (added by update_rom_list), then the controls
  group_ = lv_group_create();
  lv_group_set_wrap(group_, true);

  lv_screen_load(screen_);
}

void Gui::deinit_ui() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (screen_) {
    lv_obj_delete(screen_);
    screen_ = nullptr;
  }
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
  // (an open picker's consoles and counts are the old list's)
  close_console_picker();
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
  focused_rom_ = -1;
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    if (rom_infos_[i].rom_path == selected_path) {
      focused_rom_ = i;
      break;
    }
  }
  build_list(focused_rom_);
}

// The game list: the games that pass the console filter, under a header per
// console; `keep_rom` stays selected if it is shown, else the first game.
void Gui::build_list(int keep_rom) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  // a filter for a console that is no longer on the card shows them all
  if (filter_ >= 0 && std::none_of(rom_infos_.begin(), rom_infos_.end(), [this](const RomInfo &r) {
        return static_cast<int>(console_of(r.platform)) == filter_;
      })) {
    filter_ = -1;
  }
  auto shown = [this](size_t i) {
    return filter_ < 0 || static_cast<int>(console_of(rom_infos_[i].platform)) == filter_;
  };
  if (keep_rom < 0 || keep_rom >= (int)rom_infos_.size() || !shown(keep_rom)) {
    keep_rom = -1;
    for (size_t i = 0; i < rom_infos_.size() && keep_rom < 0; i++) {
      if (shown(i)) keep_rom = i;
    }
  }
  // (adding the first button to the empty group focuses it, and its FOCUSED
  // event selects that game: keep_rom is selected at the end)
  lv_obj_clean(rom_list_);
  rom_buttons_.assign(rom_infos_.size(), nullptr);
  if (group_) {
    lv_group_remove_all_objs(group_);
  }
  int last_console = -1;
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    if (!shown(i)) continue;
    const Console console = console_of(rom_infos_[i].platform);
    if (static_cast<int>(console) != last_console) {
      last_console = static_cast<int>(console);
      add_console_header(console);
    }
    auto btn = lv_list_add_button(rom_list_, nullptr, rom_infos_[i].name.c_str());
    lv_obj_set_user_data(btn, reinterpret_cast<void *>(static_cast<intptr_t>(i)));
    lv_obj_add_event_cb(btn, event_callback, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn, event_callback, LV_EVENT_FOCUSED, this);
    lv_obj_add_event_cb(btn, event_callback, LV_EVENT_KEY, this);
    // left / right come to the game as keys (not focus moves): they change
    // the console (see Tab5Emu::keypad_read())
    lv_obj_add_flag(btn, LV_OBJ_FLAG_USER_1);
    // the selected game (the theme has no checked look for list buttons)
    lv_obj_set_style_bg_color(btn, lv_palette_main(LV_PALETTE_BLUE), LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(btn, LV_OPA_COVER, LV_STATE_CHECKED);
    rom_buttons_[i] = btn;
    if (group_) lv_group_add_obj(group_, btn);
  }
  if (group_) {
    for (auto *o : {play_button_, volume_slider_, mute_switch_, brightness_slider_, usb_switch_, video_dropdown_,
                    resume_switch_}) {
      if (o) lv_group_add_obj(group_, o);
    }
  }
  update_filter_marker();
  if (keep_rom < 0) {
    lv_list_add_text(rom_list_, "No games found.\nAdd metadata.csv to the SD card.");
    focused_rom_ = -1;
    lv_label_set_text(title_label_, "");
    lv_image_set_src(boxart_, nullptr);
    lv_obj_clean(console_area_);
    shown_console_ = -1;
  } else {
    select_rom(keep_rom);
  }
}

std::vector<Console> Gui::consoles() const {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  std::vector<Console> out;
  for (const auto &r : rom_infos_) {
    const Console c = console_of(r.platform);
    if (out.empty() || out.back() != c) out.push_back(c); // (grouped by console)
  }
  return out;
}

std::optional<Console> Gui::console_filter() const {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (filter_ < 0) return std::nullopt;
  return static_cast<Console>(filter_);
}

bool Gui::set_console_filter(std::optional<Console> console) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto present = consoles();
  if (console && std::find(present.begin(), present.end(), *console) == present.end()) {
    return false; // no games for it
  }
  filter_ = console ? static_cast<int>(*console) : -1;
  build_list(focused_rom_); // (the selected game stays if the filter shows it)
  return true;
}

// Left / right: the previous / next console's first game; with a filter on,
// the filter moves to that console.
void Gui::step_console(int dir) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto present = consoles();
  if (present.size() < 2 || focused_rom_ < 0 || focused_rom_ >= (int)rom_infos_.size()) {
    return;
  }
  const Console current = console_of(rom_infos_[focused_rom_].platform);
  const int n = present.size();
  const int at = std::find(present.begin(), present.end(), current) - present.begin();
  const Console next = present[(at + (dir < 0 ? n - 1 : 1)) % n];
  if (filter_ >= 0) {
    set_console_filter(next);
    return;
  }
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    if (console_of(rom_infos_[i].platform) == next) {
      select_rom(i);
      if (rom_buttons_[i]) lv_obj_scroll_to_view(rom_buttons_[i], LV_ANIM_ON);
      return;
    }
  }
}

// With a filter on, the console above the boxart is outlined (the area is also the
// button that opens the console picker).
void Gui::update_filter_marker() {
  if (!console_area_) return;
  // (on the badge / art itself, which is narrower than the area)
  if (auto *shown = lv_obj_get_child(console_area_, 0)) {
    lv_obj_set_style_outline_color(shown, lv_color_white(), 0);
    lv_obj_set_style_outline_pad(shown, 3, 0);
    lv_obj_set_style_outline_width(shown, filter_ >= 0 ? 3 : 0, 0);
  }
}

// The console picker: "All consoles" and each console on the card with its
// number of games, over the launcher. A tap / A picks one, B or a tap beside
// the list closes it.
void Gui::open_console_picker() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (picker_) {
    return;
  }
  const auto present = consoles();
  picker_ = lv_obj_create(screen_);
  lv_obj_remove_style_all(picker_);
  lv_obj_set_size(picker_, lv_pct(100), lv_pct(100));
  lv_obj_set_style_bg_color(picker_, lv_color_black(), 0);
  lv_obj_set_style_bg_opa(picker_, LV_OPA_60, 0);
  lv_obj_add_flag(picker_, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_add_event_cb(picker_, picker_event, LV_EVENT_CLICKED, this);

  auto *list = lv_list_create(picker_);
  lv_obj_set_size(list, 480, LV_SIZE_CONTENT);
  lv_obj_set_style_max_height(list, Tab5Emu::lcd_height() - 120, 0);
  lv_obj_center(list);
  lv_obj_set_style_text_font(list, &lv_font_montserrat_24, 0);
  lv_list_add_text(list, "Show games for");

  picker_group_ = lv_group_create();
  lv_group_set_wrap(picker_group_, true);
  picker_consoles_.clear();
  lv_obj_t *focus = nullptr;
  auto add = [&](int console, const std::string &text) {
    auto *btn = lv_list_add_button(list, console < 0 ? LV_SYMBOL_LIST : nullptr, text.c_str());
    lv_obj_set_height(btn, 64);
    if (console >= 0) {
      auto *icon = console_icon_create(btn, console_info(static_cast<Console>(console)), 32);
      lv_obj_move_to_index(icon, 0);
    }
    if (console == filter_) lv_obj_add_state(btn, LV_STATE_CHECKED);
    lv_obj_set_user_data(btn, reinterpret_cast<void *>(static_cast<intptr_t>(picker_consoles_.size())));
    lv_obj_add_event_cb(btn, picker_event, LV_EVENT_CLICKED, this);
    lv_obj_add_event_cb(btn, picker_event, LV_EVENT_KEY, this);
    lv_group_add_obj(picker_group_, btn);
    picker_consoles_.push_back(console);
    if (console == filter_) focus = btn;
  };
  add(-1, fmt::format("All consoles  ({})", rom_infos_.size()));
  for (const Console c : present) {
    const auto n = std::count_if(rom_infos_.begin(), rom_infos_.end(),
                                 [c](const RomInfo &r) { return console_of(r.platform) == c; });
    add(static_cast<int>(c), fmt::format("{}  ({})", console_info(c).name, n));
  }
  if (focus) lv_group_focus_obj(focus);
  if (auto *keypad = Tab5Emu::get().keypad_indev()) lv_indev_set_group(keypad, picker_group_);
}

void Gui::close_console_picker() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (!picker_) {
    return;
  }
  // (out of the group before the group goes; the objects go after this event)
  lv_group_remove_all_objs(picker_group_);
  lv_group_delete(picker_group_);
  picker_group_ = nullptr;
  lv_obj_delete_async(picker_);
  picker_ = nullptr;
  if (auto *keypad = Tab5Emu::get().keypad_indev(); keypad && group_) {
    lv_indev_set_group(keypad, group_);
    if (focused_rom_ >= 0 && focused_rom_ < (int)rom_buttons_.size() && rom_buttons_[focused_rom_]) {
      lv_group_focus_obj(rom_buttons_[focused_rom_]);
    }
  }
}

void Gui::picker_event(lv_event_t *e) {
  auto *gui = static_cast<Gui *>(lv_event_get_user_data(e));
  if (!gui || !gui->picker_) {
    return;
  }
  auto *target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  const auto code = lv_event_get_code(e);
  if (code == LV_EVENT_KEY) {
    const auto key = lv_indev_get_key(lv_indev_active());
    if (key == LV_KEY_ESC || key == LV_KEY_HOME) gui->close_console_picker();
    return;
  }
  if (target == gui->picker_) {
    gui->close_console_picker(); // a tap beside the list
    return;
  }
  const auto index = reinterpret_cast<intptr_t>(lv_obj_get_user_data(target));
  if (index >= 0 && index < (intptr_t)gui->picker_consoles_.size()) {
    const int console = gui->picker_consoles_[index];
    gui->close_console_picker();
    gui->set_console_filter(console < 0 ? std::nullopt : std::optional<Console>(static_cast<Console>(console)));
  }
}

namespace {
struct SettingLimit {
  const char *name;
  int min, max;
};
// (the same ranges as the launcher's own sliders)
constexpr SettingLimit kSettingLimits[] = {
    {"volume", 0, 100}, {"muted", 0, 1}, {"brightness", 5, 100}, {"video", 0, 2}, {"resume", 0, 1}};
} // namespace

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
                     (int)emu.video_setting(),
                     s.resume_last_save ? 1 : 0, limits);
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
  update_shared_state(); // the launcher's controls show it
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

// A non-selectable row in the game list starting a console's group: the
// console's icon and name on its accent color.
void Gui::add_console_header(Console console) {
  const auto &info = console_info(console);
  auto *hdr = lv_obj_create(rom_list_);
  lv_obj_remove_style_all(hdr);
  lv_obj_remove_flag(hdr, LV_OBJ_FLAG_SCROLLABLE);
  lv_obj_remove_flag(hdr, LV_OBJ_FLAG_CLICKABLE);
  lv_obj_set_size(hdr, lv_pct(100), 44);
  lv_obj_set_style_bg_color(hdr, lv_color_hex(info.color), 0);
  lv_obj_set_style_bg_opa(hdr, LV_OPA_COVER, 0);
  lv_obj_set_style_pad_hor(hdr, 12, 0);
  lv_obj_set_style_pad_column(hdr, 12, 0);
  lv_obj_set_flex_flow(hdr, LV_FLEX_FLOW_ROW);
  lv_obj_set_flex_align(hdr, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
  console_icon_create(hdr, info, 26);
  auto *label = lv_label_create(hdr);
  lv_label_set_text(label, info.name);
  lv_obj_set_style_text_color(label, lv_color_white(), 0);
  lv_obj_set_style_text_font(label, &lv_font_montserrat_20, 0);
}

// Above the boxart: the SD card art of the selected game's console
// (boxart/consoles/<key>.jpg) if there is one, else the firmware's badge.
// Only on this screen: in a game the screen is the game's.
void Gui::show_console(Console console) {
  if (shown_console_ == static_cast<int>(console)) {
    return;
  }
  shown_console_ = static_cast<int>(console);
  lv_obj_clean(console_area_);
  if (console_art_buf_) {
    lv_draw_buf_destroy(console_art_buf_);
    console_art_buf_ = nullptr;
  }
  const auto &info = console_info(console);
  if (const auto art = console_art_path(Tab5Emu::mount_point, info); !art.empty()) {
    console_art_buf_ = decode_image(art);
    if (console_art_buf_) {
      auto *img = lv_image_create(console_area_);
      lv_obj_set_size(img, lv_pct(100), lv_pct(100));
      lv_image_set_inner_align(img, LV_IMAGE_ALIGN_CONTAIN);
      lv_image_set_src(img, console_art_buf_);
      update_filter_marker();
      return;
    }
  }
  console_badge_create(console_area_, info, 48, &lv_font_montserrat_24);
  update_filter_marker();
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
        // a game the console filter hides: show them all (build_list()
        // selects it)
        if (filter_ >= 0 && static_cast<int>(console_of(rom.platform)) != filter_) {
          filter_ = -1;
          build_list(i);
        } else {
          select_rom(i);
        }
        return true;
      }
    }
  }
  return false;
}

void Gui::select_rom(int index) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (index < 0 || index >= (int)rom_infos_.size()) {
    return;
  }
  focused_rom_ = index;
  for (size_t i = 0; i < rom_buttons_.size(); i++) {
    if (!rom_buttons_[i]) continue; // (hidden by the console filter)
    if ((int)i == index)
      lv_obj_add_state(rom_buttons_[i], LV_STATE_CHECKED);
    else
      lv_obj_remove_state(rom_buttons_[i], LV_STATE_CHECKED);
  }
  // keep the keypad focus (and its highlight) on the selected game too
  if (group_ && index < (int)rom_buttons_.size() && rom_buttons_[index] &&
      lv_group_get_focused(group_) != rom_buttons_[index]) {
    lv_group_focus_obj(rom_buttons_[index]);
  }
  const auto &rom = rom_infos_[index];
  lv_label_set_text(title_label_, rom.name.c_str());
  show_console(console_of(rom.platform));
  load_boxart(rom.boxart_path);
}

// LVGL's JPEG decoder (TJPGD) only yields the image block by block, and LVGL
// cannot scale a block-decoded image, so decode the boxart into a full RGB565
// buffer here (once per selection) and show that. The POSIX fs driver is
// registered as 'S:' and passes the rest of the path through unchanged.
void Gui::load_boxart(const std::string &path) {
  lv_image_set_src(boxart_, nullptr);
  if (boxart_buf_) {
    lv_draw_buf_destroy(boxart_buf_);
    boxart_buf_ = nullptr;
  }
  if (path.empty()) {
    return;
  }
  boxart_buf_ = decode_image(path);
  if (boxart_buf_) {
    lv_image_set_src(boxart_, boxart_buf_);
  }
}

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

void Gui::event_callback(lv_event_t *e) {
  auto gui = static_cast<Gui *>(lv_event_get_user_data(e));
  if (!gui) {
    return;
  }
  auto target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto code = lv_event_get_code(e);
  auto &emu = Tab5Emu::get();
  if (code == LV_EVENT_KEY) {
    // on a game: left / right change the console, X (home) opens the picker
    const auto key = lv_indev_get_key(lv_indev_active());
    if (key == LV_KEY_LEFT || key == LV_KEY_RIGHT) {
      gui->step_console(key == LV_KEY_LEFT ? -1 : 1);
    } else if (key == LV_KEY_HOME) {
      gui->open_console_picker();
    }
    return;
  }
  if (code == LV_EVENT_FOCUSED) {
    // gamepad / keyboard: moving the focus through the list selects the game
    for (size_t i = 0; i < gui->rom_buttons_.size(); i++) {
      if (gui->rom_buttons_[i] == target) {
        if (gui->focused_rom_ != (int)i) gui->select_rom(i);
        return;
      }
    }
  } else if (code == LV_EVENT_CLICKED) {
    if (target == gui->play_button_) {
      if (gui->focused_rom_ >= 0) {
        gui->ready_to_play_ = true;
      }
      return;
    }
    for (size_t i = 0; i < gui->rom_buttons_.size(); i++) {
      if (gui->rom_buttons_[i] == target) {
        // a tap selects; enter on the already selected game (gamepad A /
        // keyboard enter) starts it
        const bool from_keypad = lv_indev_get_type(lv_indev_active()) == LV_INDEV_TYPE_KEYPAD;
        if (from_keypad && gui->focused_rom_ == (int)i) {
          gui->ready_to_play_ = true;
        } else {
          gui->select_rom(i);
        }
        return;
      }
    }
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    // (saved to the SD card by the main loop once the changes stop)
    auto &settings = AppSettings::get();
    if (target == gui->volume_slider_) {
      const int v = lv_slider_get_value(gui->volume_slider_);
      emu.volume(v);
      settings.modify([v](Settings &s) { s.volume = v; });
    } else if (target == gui->brightness_slider_) {
      const int v = lv_slider_get_value(gui->brightness_slider_);
      emu.brightness(v);
      settings.modify([v](Settings &s) { s.brightness = v; });
    } else if (target == gui->mute_switch_) {
      const bool v = lv_obj_has_state(gui->mute_switch_, LV_STATE_CHECKED);
      emu.mute(v);
      settings.modify([v](Settings &s) { s.muted = v; });
    } else if (target == gui->usb_switch_) {
      gui->set_usb_drive(lv_obj_has_state(gui->usb_switch_, LV_STATE_CHECKED));
    } else if (target == gui->video_dropdown_) {
      const int v = lv_dropdown_get_selected(gui->video_dropdown_);
      emu.video_setting(static_cast<VideoSetting>(v));
      settings.modify([v](Settings &s) { s.video_setting = v; });
    } else if (target == gui->resume_switch_) {
      const bool v = lv_obj_has_state(gui->resume_switch_, LV_STATE_CHECKED);
      settings.modify([v](Settings &s) { s.resume_last_save = v; });
    }
  }
}

// USB drive mode: hand the SD card to a PC. Nothing may touch the card
// meanwhile, so Play is disabled; when it ends the ROM list is reloaded so
// anything copied over shows up.
void Gui::set_usb_drive(bool on) {
  auto &emu = Tab5Emu::get();
  if (on) {
    if (!emu.initialize_usb_msc()) {
      lv_obj_remove_state(usb_switch_, LV_STATE_CHECKED);
      lv_label_set_text(usb_label_, "USB drive could not start (no SD card?)");
      return;
    }
    lv_obj_add_state(play_button_, LV_STATE_DISABLED);
    lv_label_set_text(usb_label_, "Plug the USB-C port into a PC. Eject there, then switch off.");
  } else {
    emu.deinitialize_usb_msc();
    lv_obj_remove_state(play_button_, LV_STATE_DISABLED);
    lv_label_set_text(usb_label_, "SD card as a USB drive on the USB-C port");
    update_rom_list();
    update_shared_state();
  }
}

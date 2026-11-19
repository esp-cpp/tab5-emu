#include "menu.hpp"

#include <cmath>

Menu::Menu(const Config &config)
    : paused_image_path_(config.paused_image_path)
    , action_callback_(config.action_callback)
    , slot_image_callback_(config.slot_image_callback)
    , logger_({.tag = "Menu", .level = config.log_level}) {
  // the screenshot buffers are made now, while the heap still has room: a
  // running game (JK) can leave PSRAM without a 600 KB block, and a failed
  // allocation on pause would throw std::bad_alloc (no way to recover there)
  pause_image_data_.resize(SCREENSHOT_MAX_BYTES);
  slot_image_data_.resize(SCREENSHOT_MAX_BYTES);

  using K = emu_ui::Row::Kind;
  auto &emu = Tab5Emu::get();
  std::vector<emu_ui::Row> settings;
  settings.push_back({.label = "Volume", .kind = K::SLIDER, .step = 5,
                      .get = [&emu] { return (int)std::lround(emu.volume()); },
                      .set = [&emu](int v) { emu.volume(v); },
                      .symbol = LV_SYMBOL_VOLUME_MAX});
  settings.push_back({.label = "Mute", .kind = K::TOGGLE, .get = [&emu] { return emu.is_muted() ? 1 : 0; },
                      .set = [&emu](int v) { emu.mute(v != 0); },
                      .symbol = LV_SYMBOL_MUTE});
  settings.push_back({.label = "Brightness", .kind = K::SLIDER, .min = 5, .step = 5,
                      .get = [&emu] { return (int)std::lround(emu.brightness()); },
                      .set = [&emu](int v) { emu.brightness(v); },
                      .symbol = LV_SYMBOL_EYE_OPEN});
  settings.push_back({.label = "Video scaling", .kind = K::CHOICE, .choices = {"Original", "Fit", "Fill"},
                      .get = [&emu] { return (int)emu.video_setting(); },
                      .set = [&emu](int v) { emu.video_setting(static_cast<VideoSetting>(v)); },
                      .symbol = LV_SYMBOL_IMAGE});
  {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    view_ = std::make_unique<emu_ui::PauseMenu>(emu_ui::PauseMenu::Config{
        .metrics = emu_ui::Metrics::large_1280x720(),
        .keypad = emu.keypad_indev(),
        .slots = static_cast<int>(MAX_SLOT) + 1,
        .on_action = [this](emu_ui::PauseMenu::Action a) { on_action(a); },
        .on_slot =
            [this](int slot) {
              selected_slot_ = slot;
              update_slot_display();
            },
        .settings = std::move(settings),
    });
    view_->set_game(config.game_title, config.console_key);
  }
  using namespace std::placeholders;
  espp::EventManager::get().add_subscriber(volume_changed_topic, "menu", std::bind(&Menu::on_volume, this, _1), 4 * 1024);
  logger_.info("Menu created");
}

Menu::~Menu() {
  espp::EventManager::get().remove_subscriber(volume_changed_topic, "menu");
  task_.stop();
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  view_.reset();
}

void Menu::select_slot(int slot) {
  if (slot < 0) slot = MAX_SLOT;
  if (slot > (int)MAX_SLOT) slot = 0;
  selected_slot_ = slot;
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  view_->set_slot(slot); // (calls on_slot when it changes)
  update_slot_display();
}

void Menu::pause() {
  paused_ = true;
  task_.stop();
  // wait out a timer callback already in lv_timer_handler() (stop() does not)
  std::lock_guard<std::recursive_mutex> lk(mutex_);
}

void Menu::resume() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  update_pause_image();
  update_slot_display();
  view_->set_fps(get_fps());
  const auto battery = Tab5Emu::get().battery_status();
  view_->set_battery((int)std::lround(battery.charge_percent), battery.is_charging);
  view_->show();
  lv_obj_invalidate(view_->screen());
  task_.periodic(16 * 1000);
  paused_ = false;
}

void Menu::click(Action action) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  view_->click(static_cast<emu_ui::PauseMenu::Action>(action));
}

void Menu::on_action(emu_ui::PauseMenu::Action a) {
  // (the same order: RESUME, RESET, SAVE, LOAD, QUIT)
  const auto action = static_cast<Action>(a);
  action_callback_(action);
  if (action == Action::SAVE) {
    update_slot_display(); // the new screenshot
  } else if (action == Action::LOAD) {
    update_pause_image(); // the loaded state
  }
}

void Menu::update() {
  if (!paused_) {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    lv_timer_handler();
  }
}

void Menu::on_volume(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  view_->refresh_settings();
}

void Menu::update_pause_image() {
  const bool ok = emu_ui::load_screenshot(paused_image_path_.c_str(), pause_image_data_, pause_image_desc_);
  view_->set_pause_image(ok ? &pause_image_desc_ : nullptr);
}

void Menu::update_slot_display() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  const auto path = slot_image_callback_ ? slot_image_callback_() : std::string{};
  const bool ok = !path.empty() && emu_ui::load_screenshot(path.c_str(), slot_image_data_, slot_image_desc_);
  view_->set_slot_image(ok ? &slot_image_desc_ : nullptr);
}

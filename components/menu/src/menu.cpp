#include "menu.hpp"

#include <fstream>

Menu::Menu(const Config &config)
    : paused_image_path_(config.paused_image_path)
    , action_callback_(config.action_callback)
    , slot_image_callback_(config.slot_image_callback)
    , logger_({.tag = "Menu", .level = config.log_level}) {
  init_ui();
  using namespace std::placeholders;
  espp::EventManager::get().add_subscriber(volume_changed_topic, "menu", std::bind(&Menu::on_volume, this, _1), 4 * 1024);
  logger_.info("Menu created");
}

Menu::~Menu() {
  espp::EventManager::get().remove_subscriber(volume_changed_topic, "menu");
  task_.stop();
  deinit_ui();
}

void Menu::select_slot(int slot) {
  if (slot < 0)
    slot = MAX_SLOT;
  if (slot > (int)MAX_SLOT)
    slot = 0;
  selected_slot_ = slot;
  update_slot_display();
}

void Menu::pause() {
  paused_ = true;
  task_.stop();
}

void Menu::resume() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  update_shared_state();
  update_pause_image();
  update_slot_display();
  lv_label_set_text_fmt(fps_label_, "%.1f FPS", get_fps());
  lv_screen_load(screen_);
  lv_obj_invalidate(screen_);
  if (auto *keypad = Tab5Emu::get().keypad_indev(); keypad && group_) {
    lv_indev_set_group(keypad, group_);
    lv_group_focus_obj(btn_resume_);
  }
  task_.periodic(16 * 1000);
  paused_ = false;
}

void Menu::update() {
  if (!paused_) {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
    lv_timer_handler();
  }
}

void Menu::on_volume(const std::vector<uint8_t> &) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  update_shared_state();
}

void Menu::update_shared_state() {
  auto &emu = Tab5Emu::get();
  lv_slider_set_value(volume_slider_, (int)emu.volume(), LV_ANIM_OFF);
  lv_slider_set_value(brightness_slider_, (int)emu.brightness(), LV_ANIM_OFF);
  lv_dropdown_set_selected(video_dropdown_, (int)emu.video_setting());
}

bool Menu::load_screenshot(const std::string &path, lv_obj_t *image, std::vector<uint8_t> &storage,
                           lv_image_dsc_t &desc) {
  std::ifstream file(path, std::ios::binary);
  if (!file.is_open()) {
    lv_image_set_src(image, nullptr);
    return false;
  }
  uint8_t header[4];
  file.read((char *)header, 4);
  uint16_t w = (header[0] << 8) | header[1];
  uint16_t h = (header[2] << 8) | header[3];
  if (w == 0 || h == 0 || w > 2048 || h > 2048) {
    lv_image_set_src(image, nullptr);
    return false;
  }
  storage.resize((size_t)w * h * 2);
  file.read((char *)storage.data(), storage.size());
  memset(&desc, 0, sizeof(desc));
  desc.header.magic = LV_IMAGE_HEADER_MAGIC;
  desc.header.cf = LV_COLOR_FORMAT_RGB565;
  desc.header.w = w;
  desc.header.h = h;
  desc.header.stride = w * 2;
  desc.data_size = storage.size();
  desc.data = storage.data();
  lv_image_set_src(image, &desc);
  return true;
}

void Menu::update_pause_image() {
  load_screenshot(paused_image_path_, pause_image_, pause_image_data_, pause_image_desc_);
}

void Menu::update_slot_display() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  lv_label_set_text_fmt(slot_label_, "Slot %d", (int)selected_slot_);
  auto path = slot_image_callback_ ? slot_image_callback_() : "";
  if (path.empty() || !load_screenshot(path, slot_image_, slot_image_data_, slot_image_desc_)) {
    lv_image_set_src(slot_image_, nullptr);
  }
}

void Menu::init_ui() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  screen_ = lv_obj_create(nullptr);
  lv_obj_set_style_bg_color(screen_, lv_color_hex(0x101418), 0);
  lv_obj_remove_flag(screen_, LV_OBJ_FLAG_SCROLLABLE);
  const int W = Tab5Emu::lcd_width(), H = Tab5Emu::lcd_height();

  auto title = lv_label_create(screen_);
  lv_label_set_text(title, "Paused");
  lv_obj_set_style_text_font(title, &lv_font_montserrat_32, 0);
  lv_obj_set_style_text_color(title, lv_color_white(), 0);
  lv_obj_align(title, LV_ALIGN_TOP_LEFT, 24, 16);

  fps_label_ = lv_label_create(screen_);
  lv_label_set_text(fps_label_, "");
  lv_obj_set_style_text_font(fps_label_, &lv_font_montserrat_20, 0);
  lv_obj_set_style_text_color(fps_label_, lv_color_white(), 0);
  lv_obj_align(fps_label_, LV_ALIGN_TOP_RIGHT, -24, 22);

  // paused game image (left)
  pause_image_ = lv_image_create(screen_);
  lv_obj_set_size(pause_image_, 480, 360);
  lv_image_set_inner_align(pause_image_, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_align(pause_image_, LV_ALIGN_TOP_LEFT, 24, 80);

  // save slot (left, below)
  btn_slot_prev_ = lv_button_create(screen_);
  lv_obj_set_size(btn_slot_prev_, 64, 48);
  lv_obj_align(btn_slot_prev_, LV_ALIGN_TOP_LEFT, 24, 460);
  lv_label_set_text(lv_label_create(btn_slot_prev_), LV_SYMBOL_LEFT);
  lv_obj_center(lv_obj_get_child(btn_slot_prev_, 0));
  lv_obj_add_event_cb(btn_slot_prev_, event_callback, LV_EVENT_CLICKED, this);

  slot_label_ = lv_label_create(screen_);
  lv_label_set_text(slot_label_, "Slot 0");
  lv_obj_set_style_text_font(slot_label_, &lv_font_montserrat_24, 0);
  lv_obj_set_style_text_color(slot_label_, lv_color_white(), 0);
  lv_obj_align(slot_label_, LV_ALIGN_TOP_LEFT, 110, 470);

  btn_slot_next_ = lv_button_create(screen_);
  lv_obj_set_size(btn_slot_next_, 64, 48);
  lv_obj_align(btn_slot_next_, LV_ALIGN_TOP_LEFT, 220, 460);
  lv_label_set_text(lv_label_create(btn_slot_next_), LV_SYMBOL_RIGHT);
  lv_obj_center(lv_obj_get_child(btn_slot_next_, 0));
  lv_obj_add_event_cb(btn_slot_next_, event_callback, LV_EVENT_CLICKED, this);

  slot_image_ = lv_image_create(screen_);
  lv_obj_set_size(slot_image_, 240, 180);
  lv_image_set_inner_align(slot_image_, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_align(slot_image_, LV_ALIGN_TOP_LEFT, 300, 460);

  // actions (center)
  auto actions = lv_obj_create(screen_);
  lv_obj_set_size(actions, 300, H - 100);
  lv_obj_align(actions, LV_ALIGN_TOP_MID, 100, 76);
  lv_obj_set_flex_flow(actions, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_row(actions, 16, 0);
  auto make_button = [&](const char *text) {
    auto b = lv_button_create(actions);
    lv_obj_set_size(b, lv_pct(100), 64);
    auto l = lv_label_create(b);
    lv_label_set_text(l, text);
    lv_obj_set_style_text_font(l, &lv_font_montserrat_24, 0);
    lv_obj_center(l);
    lv_obj_add_event_cb(b, event_callback, LV_EVENT_CLICKED, this);
    return b;
  };
  btn_resume_ = make_button(LV_SYMBOL_PLAY "  Resume");
  btn_save_ = make_button(LV_SYMBOL_SAVE "  Save");
  btn_load_ = make_button(LV_SYMBOL_UPLOAD "  Load");
  btn_reset_ = make_button(LV_SYMBOL_REFRESH "  Reset");
  btn_quit_ = make_button(LV_SYMBOL_CLOSE "  Quit");

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
  make_label(LV_SYMBOL_EYE_OPEN "  Brightness");
  brightness_slider_ = lv_slider_create(settings);
  lv_obj_set_width(brightness_slider_, lv_pct(90));
  lv_slider_set_range(brightness_slider_, 5, 100);
  lv_obj_add_event_cb(brightness_slider_, event_callback, LV_EVENT_VALUE_CHANGED, this);
  make_label(LV_SYMBOL_IMAGE "  Video scaling");
  video_dropdown_ = lv_dropdown_create(settings);
  lv_dropdown_set_options(video_dropdown_, "Original\nFit\nFill");
  lv_obj_set_width(video_dropdown_, lv_pct(90));
  lv_obj_add_event_cb(video_dropdown_, event_callback, LV_EVENT_VALUE_CHANGED, this);

  // gamepad / keyboard focus order (see Tab5Emu::keypad_indev())
  group_ = lv_group_create();
  for (auto *o : {btn_resume_, btn_save_, btn_load_, btn_reset_, btn_quit_, btn_slot_prev_, btn_slot_next_, volume_slider_,
                  brightness_slider_, video_dropdown_}) {
    lv_group_add_obj(group_, o);
  }
  lv_group_set_wrap(group_, true);
}

void Menu::deinit_ui() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (group_) {
    if (auto *keypad = Tab5Emu::get().keypad_indev(); keypad && lv_indev_get_group(keypad) == group_) {
      lv_indev_set_group(keypad, nullptr);
    }
    lv_group_delete(group_);
    group_ = nullptr;
  }
  if (screen_) {
    lv_obj_delete(screen_);
    screen_ = nullptr;
  }
}

void Menu::event_callback(lv_event_t *e) {
  auto menu = static_cast<Menu *>(lv_event_get_user_data(e));
  if (!menu) {
    return;
  }
  auto target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto code = lv_event_get_code(e);
  auto &emu = Tab5Emu::get();
  if (code == LV_EVENT_CLICKED) {
    if (target == menu->btn_resume_)
      menu->action_callback_(Action::RESUME);
    else if (target == menu->btn_reset_)
      menu->action_callback_(Action::RESET);
    else if (target == menu->btn_save_) {
      menu->action_callback_(Action::SAVE);
      menu->update_slot_display();
    } else if (target == menu->btn_load_)
      menu->action_callback_(Action::LOAD);
    else if (target == menu->btn_quit_)
      menu->action_callback_(Action::QUIT);
    else if (target == menu->btn_slot_prev_)
      menu->select_slot(menu->selected_slot_ - 1);
    else if (target == menu->btn_slot_next_)
      menu->select_slot(menu->selected_slot_ + 1);
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (target == menu->volume_slider_)
      emu.volume(lv_slider_get_value(menu->volume_slider_));
    else if (target == menu->brightness_slider_)
      emu.brightness(lv_slider_get_value(menu->brightness_slider_));
    else if (target == menu->video_dropdown_)
      emu.video_setting(static_cast<VideoSetting>(lv_dropdown_get_selected(menu->video_dropdown_)));
  }
}

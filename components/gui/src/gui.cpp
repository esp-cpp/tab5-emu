#include "gui.hpp"

#include "lvgl_private.h"


using namespace std::chrono_literals;

Gui::Gui(const Config &config)
    : metadata_filename_(config.metadata_filename)
    , logger_({.tag = "Gui", .level = config.log_level}) {
  init_ui();
  update_rom_list();
  update_shared_state();
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
}

void Gui::resume() {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  update_rom_list();
  update_shared_state();
  lv_screen_load(screen_);
  lv_obj_invalidate(screen_);
  task_.periodic(16 * 1000);
  paused_ = false;
}

void Gui::update() {
  if (!paused_) {
    std::lock_guard<std::recursive_mutex> lk(mutex_);
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

  // boxart + title (center)
  boxart_ = lv_image_create(screen_);
  lv_obj_set_size(boxart_, 400, 400);
  lv_image_set_inner_align(boxart_, LV_IMAGE_ALIGN_CONTAIN);
  lv_obj_align(boxart_, LV_ALIGN_TOP_MID, 40, 90);

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

  make_label(LV_SYMBOL_IMAGE "  Video scaling");
  video_dropdown_ = lv_dropdown_create(settings);
  lv_dropdown_set_options(video_dropdown_, "Original\nFit\nFill");
  lv_obj_set_width(video_dropdown_, lv_pct(90));
  lv_obj_add_event_cb(video_dropdown_, event_callback, LV_EVENT_VALUE_CHANGED, this);

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
  rom_infos_ = parse_metadata(metadata_filename_);
  lv_obj_clean(rom_list_);
  rom_buttons_.clear();
  for (size_t i = 0; i < rom_infos_.size(); i++) {
    auto btn = lv_list_add_button(rom_list_, LV_SYMBOL_FILE, rom_infos_[i].name.c_str());
    lv_obj_set_user_data(btn, reinterpret_cast<void *>(i));
    lv_obj_add_event_cb(btn, event_callback, LV_EVENT_CLICKED, this);
    rom_buttons_.push_back(btn);
  }
  if (rom_infos_.empty()) {
    lv_list_add_text(rom_list_, "No games found.\nAdd metadata.csv to the SD card.");
    focused_rom_ = -1;
    lv_label_set_text(title_label_, "");
    lv_image_set_src(boxart_, nullptr);
  } else {
    select_rom(std::clamp<int>(focused_rom_, 0, rom_infos_.size() - 1));
  }
}

void Gui::select_rom(int index) {
  std::lock_guard<std::recursive_mutex> lk(mutex_);
  if (index < 0 || index >= (int)rom_infos_.size()) {
    return;
  }
  focused_rom_ = index;
  for (size_t i = 0; i < rom_buttons_.size(); i++) {
    if ((int)i == index)
      lv_obj_add_state(rom_buttons_[i], LV_STATE_CHECKED);
    else
      lv_obj_remove_state(rom_buttons_[i], LV_STATE_CHECKED);
  }
  const auto &rom = rom_infos_[index];
  lv_label_set_text(title_label_, rom.name.c_str());
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
  const std::string src = "S:" + path;
  lv_image_decoder_dsc_t dsc{};
  if (lv_image_decoder_open(&dsc, src.c_str(), nullptr) != LV_RESULT_OK) {
    logger_.warn("boxart '{}': cannot decode", path);
    return;
  }
  const uint32_t w = dsc.header.w, h = dsc.header.h;
  if (dsc.decoded && dsc.decoded->data) {
    // fully decoded formats: LVGL scales those itself, use the file directly
    lv_image_decoder_close(&dsc);
    static std::string file_src;
    file_src = src;
    lv_image_set_src(boxart_, file_src.c_str());
    return;
  }
  auto *out = lv_draw_buf_create(w, h, LV_COLOR_FORMAT_RGB565, LV_STRIDE_AUTO);
  if (!out) {
    lv_image_decoder_close(&dsc);
    return;
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
  boxart_buf_ = out;
  lv_image_set_src(boxart_, boxart_buf_);
  logger_.info("boxart '{}': {}x{}", path, w, h);
}

void Gui::event_callback(lv_event_t *e) {
  auto gui = static_cast<Gui *>(lv_event_get_user_data(e));
  if (!gui) {
    return;
  }
  auto target = static_cast<lv_obj_t *>(lv_event_get_target(e));
  auto code = lv_event_get_code(e);
  auto &emu = Tab5Emu::get();
  if (code == LV_EVENT_CLICKED) {
    if (target == gui->play_button_) {
      if (gui->focused_rom_ >= 0) {
        gui->ready_to_play_ = true;
      }
      return;
    }
    for (size_t i = 0; i < gui->rom_buttons_.size(); i++) {
      if (gui->rom_buttons_[i] == target) {
        gui->select_rom(i);
        return;
      }
    }
  } else if (code == LV_EVENT_VALUE_CHANGED) {
    if (target == gui->volume_slider_) {
      emu.volume(lv_slider_get_value(gui->volume_slider_));
    } else if (target == gui->brightness_slider_) {
      emu.brightness(lv_slider_get_value(gui->brightness_slider_));
    } else if (target == gui->mute_switch_) {
      emu.mute(lv_obj_has_state(gui->mute_switch_, LV_STATE_CHECKED));
    } else if (target == gui->video_dropdown_) {
      emu.video_setting(static_cast<VideoSetting>(lv_dropdown_get_selected(gui->video_dropdown_)));
    }
  }
}

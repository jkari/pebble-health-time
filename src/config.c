#include <pebble.h>
#include "ui.h"
#include "weather.h"
#include "config.h"

static void _update_settings(int use_celcius, char* theme, int show_activity_current,
                            int show_activity_progress, int show_activity, int show_sleep,
                            int show_weather, int show_pins_sun, int show_pins_achievement) {
  persist_write_int(MESSAGE_KEY_USE_CELCIUS, use_celcius);
  persist_write_string(MESSAGE_KEY_THEME, theme);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY_CURRENT, show_activity_current);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY_PROGRESS, show_activity_progress);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY, show_activity);
  persist_write_int(MESSAGE_KEY_SHOW_SLEEP, show_sleep);
  persist_write_int(MESSAGE_KEY_SHOW_WEATHER, show_weather);
  persist_write_int(MESSAGE_KEY_SHOW_PINS_SUN, show_pins_sun);
  persist_write_int(MESSAGE_KEY_SHOW_PINS_ACHIEVEMENT, show_pins_achievement);
  
  ui_update_config();
  weather_update();
}


void config_received_callback(DictionaryIterator* iterator) {
  Tuple *use_celcius = dict_find(iterator, MESSAGE_KEY_USE_CELCIUS);
  Tuple *theme = dict_find(iterator, MESSAGE_KEY_THEME);
  Tuple *show_activity_current = dict_find(iterator, MESSAGE_KEY_SHOW_ACTIVITY_CURRENT);
  Tuple *show_activity_progress = dict_find(iterator, MESSAGE_KEY_SHOW_ACTIVITY_PROGRESS);
  Tuple *show_activity = dict_find(iterator, MESSAGE_KEY_SHOW_ACTIVITY);
  Tuple *show_sleep = dict_find(iterator, MESSAGE_KEY_SHOW_SLEEP);
  Tuple *show_weather = dict_find(iterator, MESSAGE_KEY_SHOW_WEATHER);
  Tuple *show_pins_sun = dict_find(iterator, MESSAGE_KEY_SHOW_PINS_SUN);
  Tuple *show_pins_achievement = dict_find(iterator, MESSAGE_KEY_SHOW_PINS_ACHIEVEMENT);
  
  LOG(
    "Received use_celcius=%s, theme=%s, show_activity_current=%d, show_activity_progress=%d, show_activity=%d, show_sleep=%d, show_weather=%d, show_pins_sun=%d, show_pins_achievement=%d",
    use_celcius->value->cstring,
    theme->value->cstring,
    (int)show_activity_current->value->int32,
    (int)show_activity_progress->value->int32,
    (int)show_activity->value->int32,
    (int)show_sleep->value->int32,
    (int)show_weather->value->int32,
    (int)show_pins_sun->value->int32,
    (int)show_pins_achievement->value->int32
  );
  
  _update_settings(
    strcmp(use_celcius->value->cstring, "1") == 0 ? 1 : 0,
    theme->value->cstring,
    show_activity_current->value->int32,
    show_activity_progress->value->int32,
    show_activity->value->int32,
    show_sleep->value->int32,
    show_weather->value->int32,
    show_pins_sun->value->int32,
    show_pins_achievement->value->int32
  );
}

int _get_theme() {
  char theme[16];
  
  if (!persist_exists(MESSAGE_KEY_THEME)) {
    return THEME_LIGHT;
  }
  
  persist_read_string(MESSAGE_KEY_THEME, theme, 16);
  
  if (strcmp(theme, "dark") == 0) {
    return THEME_DARK;
  }
  
  return THEME_LIGHT;
}

bool config_show_sleep() {
  return persist_exists(MESSAGE_KEY_SHOW_SLEEP) ? persist_read_int(MESSAGE_KEY_SHOW_SLEEP) > 0 : true; 
}

bool config_show_activity() {
  return persist_exists(MESSAGE_KEY_SHOW_ACTIVITY) ? persist_read_int(MESSAGE_KEY_SHOW_ACTIVITY) > 0 : true;
}

bool config_show_activity_progress() {
  return persist_exists(MESSAGE_KEY_SHOW_ACTIVITY_PROGRESS) ? persist_read_int(MESSAGE_KEY_SHOW_ACTIVITY_PROGRESS) > 0 : true;
}

bool config_show_activity_current() {
  return persist_exists(MESSAGE_KEY_SHOW_ACTIVITY_CURRENT) ? persist_read_int(MESSAGE_KEY_SHOW_ACTIVITY_CURRENT) > 0 : false;
}

bool config_show_weather() {
  return persist_exists(MESSAGE_KEY_SHOW_WEATHER) ? persist_read_int(MESSAGE_KEY_SHOW_WEATHER) > 0 : true;
}

bool config_show_pins_sun() {
  return persist_exists(MESSAGE_KEY_SHOW_PINS_SUN) ? persist_read_int(MESSAGE_KEY_SHOW_PINS_SUN) > 0 : true;
}

bool config_show_pins_achievement() {
  return persist_exists(MESSAGE_KEY_SHOW_PINS_ACHIEVEMENT) ? persist_read_int(MESSAGE_KEY_SHOW_PINS_ACHIEVEMENT) > 0 : true;
}

GColor config_get_color_bg() {
  return _get_theme() == THEME_LIGHT ? GColorWhite : GColorBlack;
}

GColor config_get_color_text() {
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
}

GColor config_get_color_text2() {
#ifdef PBL_APLITE
  return GColorLightGray;
#else
  return _get_theme() == THEME_LIGHT ? GColorLightGray : GColorDarkGray;
#endif
}

GColor config_get_color_marker() {
  return config_get_color_text();
}

GColor config_get_color_hour() {
#ifdef PBL_APLITE
  return GColorLightGray;
#else
  return _get_theme() == THEME_LIGHT ? GColorBulgarianRose : GColorElectricBlue;
#endif
}

GColor config_get_color_minute() {
#ifdef PBL_APLITE
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
#else
  return _get_theme() == THEME_LIGHT ? GColorOxfordBlue : GColorMelon;
#endif
}

GColor config_get_color_sleep(int level) {
#ifdef PBL_APLITE
  if (level == 1) {
    return GColorLightGray;
  } else {
    return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
  }
#else
  return level == 1 ? GColorPictonBlue : GColorBlue;
#endif
}

GColor config_get_color_activity(int level) {
#ifdef PBL_APLITE
  if (level == 1) {
    return GColorLightGray;
  } else if (level == 2) {
    return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
  }
#else
  if (level == 1) {
    return GColorOrange;
  } else if (level == 2) {
    return GColorRed;
  }
  
  return GColorDarkCandyAppleRed;
#endif
}

GColor config_get_color_current_activity_neutral() {
#ifdef PBL_APLITE
  return GColorLightGray;
#else
  return GColorGreen;
#endif
}

GColor config_get_color_current_activity_minus() {
#ifdef PBL_APLITE
  return _get_theme() == THEME_LIGHT ? GColorWhite : GColorBlack;
#else
  return GColorRed;
#endif
}

GColor config_get_color_current_activity_plus() {
#ifdef PBL_APLITE
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
#else
  return GColorDarkGreen;
#endif
}

GColor config_get_color_weekday_bg() {
#ifdef PBL_APLITE
  return GColorLightGray;
#else
  return _get_theme() == THEME_LIGHT ? GColorDarkGray : GColorLightGray;
#endif
}

GColor config_get_color_battery(int percentage) {
#ifdef PBL_APLITE
  if (percentage <= BATTERY_LOW_MAX) {
    return GColorLightGray;
  } else {
    return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
#else
  if (percentage <= BATTERY_LOW_MAX) {
    return GColorRed;  
  } else if (percentage >= BATTERY_HIGH_MIN) {
    return GColorGreen;
  } else {
    return GColorOrange;
  }
#endif
}

bool config_get_use_celcius() {
  return persist_exists(MESSAGE_KEY_USE_CELCIUS) ? persist_read_int(MESSAGE_KEY_USE_CELCIUS) > 0 : true;
}
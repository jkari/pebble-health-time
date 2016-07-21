#include <pebble.h>
#include "ui.h"
#include "weather.h"
#include "config.h"

static void _update_settings(int use_celcius, char* theme, int show_activity_current,
                            int show_activity_progress, int show_activity, int show_sleep,
                            int show_weather, int show_pins_sun, int show_pins_achievement,
                            char* weather_provider, char* apikey_fio, char* apikey_owm, char* apikey_wu) {
  persist_write_int(MESSAGE_KEY_USE_CELCIUS, use_celcius);
  persist_write_string(MESSAGE_KEY_THEME, theme);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY_CURRENT, show_activity_current);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY_PROGRESS, show_activity_progress);
  persist_write_int(MESSAGE_KEY_SHOW_ACTIVITY, show_activity);
  persist_write_int(MESSAGE_KEY_SHOW_SLEEP, show_sleep);
  persist_write_int(MESSAGE_KEY_SHOW_WEATHER, show_weather);
  persist_write_int(MESSAGE_KEY_SHOW_PINS_SUN, show_pins_sun);
  persist_write_int(MESSAGE_KEY_SHOW_PINS_ACHIEVEMENT, show_pins_achievement);
  persist_write_string(MESSAGE_KEY_WEATHER_PROVIDER, weather_provider);
  persist_write_string(MESSAGE_KEY_APIKEY_FORECASTIO, apikey_fio);
  persist_write_string(MESSAGE_KEY_APIKEY_OPENWEATHERMAP, apikey_owm);
  persist_write_string(MESSAGE_KEY_APIKEY_WEATHERUNDERGROUND, apikey_wu);
  
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
  Tuple *weather_provider = dict_find(iterator, MESSAGE_KEY_WEATHER_PROVIDER);
  Tuple *apikey_fio = dict_find(iterator, MESSAGE_KEY_APIKEY_FORECASTIO);
  Tuple *apikey_owm = dict_find(iterator, MESSAGE_KEY_APIKEY_OPENWEATHERMAP);
  Tuple *apikey_wu = dict_find(iterator, MESSAGE_KEY_APIKEY_WEATHERUNDERGROUND);
  
  LOG(
    "provider=%s, apikey_fio=%s, apikey_owm=%s, apikey_wu=%s",
    weather_provider->value->cstring,
    apikey_fio->value->cstring,
    apikey_owm->value->cstring,
    apikey_wu->value->cstring
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
    show_pins_achievement->value->int32,
    weather_provider->value->cstring,
    apikey_fio->value->cstring,
    apikey_owm->value->cstring,
    apikey_wu->value->cstring
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
#ifdef PBL_BW
  return GColorLightGray;
#else
  return _get_theme() == THEME_LIGHT ? GColorLightGray : GColorDarkGray;
#endif
}

GColor config_get_color_marker() {
  return config_get_color_text();
}

GColor config_get_color_hour() {
#ifdef PBL_BW
  return config_get_color_minute();
#else
  return _get_theme() == THEME_LIGHT ? GColorBulgarianRose : GColorElectricBlue;
#endif
}

GColor config_get_color_minute() {
#ifdef PBL_BW
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
#else
  return _get_theme() == THEME_LIGHT ? GColorOxfordBlue : GColorMelon;
#endif
}

GColor config_get_color_sleep(int level) {
#ifdef PBL_BW
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
#ifdef PBL_BW
  if (level == 1) {
    return GColorLightGray;
  }
  
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
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
#ifdef PBL_BW
  return GColorLightGray;
#else
  return GColorGreen;
#endif
}

GColor config_get_color_current_activity_minus() {
#ifdef PBL_BW
  return GColorLightGray;
#else
  return GColorRed;
#endif
}

GColor config_get_color_current_activity_plus() {
#ifdef PBL_BW
  return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
#else
  return GColorDarkGreen;
#endif
}

GColor config_get_color_weekday_bg() {
#ifdef PBL_BW
  return GColorLightGray;
#else
  return _get_theme() == THEME_LIGHT ? GColorDarkGray : GColorLightGray;
#endif
}

GColor config_get_color_battery(int percentage) {
#ifdef PBL_BW
  if (percentage <= BATTERY_LOW_MAX) {
    return GColorLightGray;
  } else {
    return _get_theme() == THEME_LIGHT ? GColorBlack : GColorWhite;
  }
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

GColor config_get_color_pin_line() {
#ifdef PBL_BW
  return GColorBlack;
#else
  return config_get_color_weekday_bg();
#endif
}

bool config_get_use_celcius() {
  return persist_exists(MESSAGE_KEY_USE_CELCIUS) ? persist_read_int(MESSAGE_KEY_USE_CELCIUS) > 0 : true;
}

WeatherProvider config_get_weather_provider() {
  char provider[8];
  
  persist_read_string(MESSAGE_KEY_WEATHER_PROVIDER, provider, 8);
  
  if (strcmp(provider, "fio") == 0) {
    return WEATHER_PROVIDER_FORECASTIO;
  } else if (strcmp(provider, "owm") == 0) {
    return WEATHER_PROVIDER_OPENWEATHERMAP;
  } else if (strcmp(provider, "wu")) {
    return WEATHER_PROVIDER_WEATHERUNDERGROUND;
  }
  
  return WEATHER_PROVIDER_UNKNOWN;
}

char* config_get_weather_api_key() {
  static char key[64];
  
  switch (config_get_weather_provider()) {
    case WEATHER_PROVIDER_FORECASTIO: persist_read_string(MESSAGE_KEY_APIKEY_FORECASTIO, key, 64); break;
    case WEATHER_PROVIDER_OPENWEATHERMAP: persist_read_string(MESSAGE_KEY_APIKEY_OPENWEATHERMAP, key, 64); break;
    case WEATHER_PROVIDER_WEATHERUNDERGROUND: persist_read_string(MESSAGE_KEY_APIKEY_WEATHERUNDERGROUND, key, 64); break;
    case WEATHER_PROVIDER_UNKNOWN: break;
  }
  
  return key;
}
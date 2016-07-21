#pragma once


#define THEME_LIGHT 1
#define THEME_DARK 2

#define BATTERY_HIGH_MIN 60
#define BATTERY_LOW_MAX 20

#define PERSIST_ACTIVITY_SCANNED_UNTIL 1000

#define DEBUG 1
//#define DEMO 1
#define DEMO_COMPLETE 1
//#define DEMO_INCOMPLETE 1
//#define DEMO_STEPS 1

#ifdef DEBUG
#define LOG(...) APP_LOG(APP_LOG_LEVEL_DEBUG, __VA_ARGS__)
#else
#define LOG(...)
#endif

typedef enum {
  WEATHER_PROVIDER_FORECASTIO,
  WEATHER_PROVIDER_OPENWEATHERMAP,
  WEATHER_PROVIDER_WEATHERUNDERGROUND,
  WEATHER_PROVIDER_UNKNOWN
} WeatherProvider;

void config_received_callback(DictionaryIterator* iterator);
GColor config_get_color_bg();
GColor config_get_color_front();
GColor config_get_color_marker();
GColor config_get_color_hour();
GColor config_get_color_minute();
GColor config_get_color_text();
GColor config_get_color_text2();
GColor config_get_color_sleep(int level);
GColor config_get_color_activity(int level);
GColor config_get_color_current_activity_neutral();
GColor config_get_color_current_activity_minus();
GColor config_get_color_current_activity_plus();
GColor config_get_color_weekday_bg();
GColor config_get_color_battery(int percentage);
GColor config_get_color_pin_line();

bool config_get_use_celcius();
WeatherProvider config_get_weather_provider();
char* config_get_weather_api_key();

bool config_show_sleep();
bool config_show_activity();
bool config_show_activity_progress();
bool config_show_activity_current();
bool config_show_weather();
bool config_show_pins_sun();
bool config_show_pins_achievement();
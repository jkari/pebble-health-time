#include <pebble.h>
#include <pebble-generic-weather/pebble-generic-weather.h>
#include "weather.h"
#include "config.h"
#include "communication.h"
#include "ui.h"

static GenericWeatherInfo _weather;
static bool _is_available = false;

void _set_config() {
  generic_weather_init();

  switch (config_get_weather_provider()) {
    case WEATHER_PROVIDER_FORECASTIO: generic_weather_set_provider(GenericWeatherProviderForecastIo); break;
    case WEATHER_PROVIDER_OPENWEATHERMAP: generic_weather_set_provider(GenericWeatherProviderOpenWeatherMap); break;
    case WEATHER_PROVIDER_WEATHERUNDERGROUND: generic_weather_set_provider(GenericWeatherProviderWeatherUnderground); break;
    case WEATHER_PROVIDER_UNKNOWN:break;
  }
    
  generic_weather_set_api_key(config_get_weather_api_key());
}

void weather_init(void) {
  if (config_show_weather()) {
    _set_config();
  }
    
  if (!persist_exists(MESSAGE_KEY_TEMPERATURE) && config_show_weather()) {
    weather_update();
  }
}

int weather_get_resource_id() {
  switch (_weather.condition) {
    case GenericWeatherConditionClearSky: return RESOURCE_ID_IMAGE_CLEAR;
    case GenericWeatherConditionFewClouds: return RESOURCE_ID_IMAGE_CLOUDS_LIGHT;
    case GenericWeatherConditionScatteredClouds: return RESOURCE_ID_IMAGE_CLOUDS_MEDIUM;
    case GenericWeatherConditionBrokenClouds: return RESOURCE_ID_IMAGE_CLOUDS_HEAVY;
    case GenericWeatherConditionShowerRain: return RESOURCE_ID_IMAGE_RAIN_LIGHT;
    case GenericWeatherConditionRain: return RESOURCE_ID_IMAGE_THUNDER_HEAVY;
    case GenericWeatherConditionThunderstorm: return RESOURCE_ID_IMAGE_THUNDER_HEAVY;
    case GenericWeatherConditionSnow: return RESOURCE_ID_IMAGE_SNOW_HEAVY;
    case GenericWeatherConditionMist: return RESOURCE_ID_IMAGE_MIST;
    case GenericWeatherConditionUnknown: return 0; 
  }
  
  return 0;
}

static void _weather_data_callback(GenericWeatherInfo *info, GenericWeatherStatus status) {
  if (status == GenericWeatherStatusAvailable) {
    _weather = *info;
    ui_update_weather();
    _is_available = true;
  }
}

void weather_update(void) {
  if (config_show_weather()) {
    _set_config();
    generic_weather_fetch(_weather_data_callback);
  }
}

int weather_get_sunrise_hour() {
  struct tm *tick_time = localtime(&_weather.timesunrise);
  return tick_time->tm_hour;
}

int weather_get_sunset_hour() {
  struct tm *tick_time = localtime(&_weather.timesunset);
  return tick_time->tm_hour;
}

int weather_get_sunrise_minute() {
  struct tm *tick_time = localtime(&_weather.timesunrise);
  return tick_time->tm_min;
}

int weather_get_sunset_minute() {
  struct tm *tick_time = localtime(&_weather.timesunset);
  return tick_time->tm_min;
}

int weather_get_condition() {
  return persist_read_int(MESSAGE_KEY_CONDITIONS);
}

int weather_get_temperature() {
  return config_get_use_celcius() ? _weather.temp_c : _weather.temp_f;
}

bool weather_is_available() {
  return _is_available;
}
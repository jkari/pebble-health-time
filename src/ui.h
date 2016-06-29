#pragma once

#define UI_UPDATE_ACTIVITY_MS 15000
#define STEPS_X 0
#define STEPS_Y -40
#define STEPS_WIDTH 14
#define STEPS_HEIGHT 14
#define STEPS_CIRCLE_WIDTH 3
#define STEPS_CIRCLE_RADIUS 12
#define BATTERY_X PBL_IF_ROUND_ELSE(10, 10)
#define BATTERY_Y PBL_IF_ROUND_ELSE(-12, -12)
#define BATTERY_WIDTH 60
#define BATTERY_HIGH_MIN 60
#define BATTERY_LOW_MAX 20
#define BATTERY_BLOCK_WIDTH 3
#define BATTERY_BLOCK_HEIGHT 4
#define BATTERY_BLOCK_SPACING 1
#define BATTERY_COLOR_HIGH GColorGreen
#define BATTERY_COLOR_MEDIUM GColorChromeYellow 
#define BATTERY_COLOR_LOW GColorRed
#define WIDTH_CIRCLE 5.f
#define WIDTH_EVENT_POINTER 1.5f
#define ARC_RADIUS_MARKER PBL_IF_ROUND_ELSE(85, 56)
#define ACTIVITY_PROGRESS_RADIUS PBL_IF_ROUND_ELSE(88, 59)
#define ACTIVITY_PROGRESS_WIDTH 6.f
#define ARC_RADIUS_ACTIVITY PBL_IF_ROUND_ELSE(69, 60)
#define ARC_RADIUS_SLEEP PBL_IF_ROUND_ELSE(76, 55)
#define ARC_RADIUS_HAND PBL_IF_ROUND_ELSE(45, 36)
#define HAND_LENGTH_MINUTE PBL_IF_ROUND_ELSE(60, 45)
#define HAND_LENGTH_HOUR PBL_IF_ROUND_ELSE(45, 35)
#define WEATHER_TEMPERATURE_Y 38
#define WEATHER_TEMPERATURE_X 7
#define WEATHER_TEMPERATURE_WIDTH 24
#define WEATHER_TEMPERATURE_HEIGHT 20
#define WEATHER_TEMPERATURE_BG_RADIUS 9
#define WEATHER_ICON_Y PBL_IF_ROUND_ELSE(15, 15)
#define WEATHER_ICON_WIDTH 32
#define WEATHER_ICON_BG_RADIUS 20
#define BLUETOOTH_OFFSET_X PBL_IF_ROUND_ELSE(40, 32)
#define WEEKDAY_X PBL_IF_ROUND_ELSE(10, 10)
#define WEEKDAY_Y PBL_IF_ROUND_ELSE(-6, -6)
#define WEEKDAY_FONT_Y -5
#define WEEKDAY_BG_WIDTH 45
#define WEEKDAY_BG_HEIGHT 12
#define DATE_X PBL_IF_ROUND_ELSE(40, 40)
#define PIN_RADIUS 11
#define PIN_ARC_RADIUS PBL_IF_ROUND_ELSE(57, 60)
#define PIN_ICON_WIDTH 14

#define ANGLE_POINT(cx,cy,angle,radius) {.x=(int16_t)(sin_lookup((angle)/360.f*TRIG_MAX_ANGLE)*(radius)/TRIG_MAX_RATIO)+cx,.y=(int16_t)(-cos_lookup((angle)/360.f*TRIG_MAX_ANGLE)*(radius)/TRIG_MAX_RATIO)+cy}

void ui_bluetooth_set_available(bool is_available);
void ui_load(Window *window);
void ui_unload(void);
void ui_hide();
void ui_show();
void ui_update_colors();
void ui_update_time(void);
void ui_update_date(void);
void ui_update_time_and_activity();
void ui_update_weather(void);
void ui_battery_charge_start(void);
void ui_battery_charge_stop(void);
void ui_set_temperature(int temperature);
void ui_set_weather_icon(uint32_t resource_id);
void ui_poll_activity(void* data);
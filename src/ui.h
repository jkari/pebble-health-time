#pragma once

#define UI_UPDATE_ACTIVITY_SECS 10
#define CURRENT_ACTIVITY_SHOW_LIMIT_SPM 20

#define STEPS_X PBL_IF_ROUND_ELSE(-20, -22)
#define STEPS_Y PBL_IF_ROUND_ELSE(-40, -17)
#define STEPS_TEXT_WIDTH 40
#define STEPS_TEXT_HEIGHT 20
#define STEPS_TEXT_OFFSET_X 5
#define STEPS_TEXT_OFFSET_Y -5
#define STEPS_WIDTH 14
#define STEPS_HEIGHT 14
#define BATTERY_X PBL_IF_ROUND_ELSE(10, -21)
#define BATTERY_Y PBL_IF_ROUND_ELSE(-12, -38)
#define BATTERY_WIDTH 60
#define BATTERY_BLOCK_WIDTH 3
#define BATTERY_BLOCK_HEIGHT 4
#define BATTERY_BLOCK_SPACING 1
#define BATTERY_COLOR_HIGH GColorGreen
#define BATTERY_COLOR_MEDIUM GColorChromeYellow 
#define BATTERY_COLOR_LOW GColorRed
#define WIDTH_EVENT_POINTER 1.5f
#define MARKER_SIZE_X PBL_IF_ROUND_ELSE(174, 146)
#define MARKER_SIZE_Y PBL_IF_ROUND_ELSE(174, 170)
#define DATA_SIZE_X PBL_IF_ROUND_ELSE(154, 126)
#define DATA_SIZE_Y PBL_IF_ROUND_ELSE(154, 150)
#define DATA_WIDTH 4
#define DATA_SPACING 2
#define SLEEP_SIZE_X PBL_IF_ROUND_ELSE(152, 120)
#define SLEEP_SIZE_Y PBL_IF_ROUND_ELSE(152, 144)
#define ARC_RADIUS_HAND PBL_IF_ROUND_ELSE(45, 36)
#define HAND_LENGTH_MINUTE PBL_IF_ROUND_ELSE(56, 45)
#define HAND_LENGTH_HOUR PBL_IF_ROUND_ELSE(42, 32)
#define WEATHER_TEMPERATURE_Y PBL_IF_ROUND_ELSE(21, 25)
#define WEATHER_TEMPERATURE_X 7
#define WEATHER_TEMPERATURE_WIDTH 24
#define WEATHER_TEMPERATURE_HEIGHT 20
#define WEATHER_TEMPERATURE_BG_RADIUS 9
#define WEATHER_ICON_Y PBL_IF_ROUND_ELSE(3, 7)
#define WEATHER_ICON_WIDTH 32
#define WEATHER_ICON_BG_RADIUS 20
#define BLUETOOTH_X PBL_IF_ROUND_ELSE(9, -21)
#define BLUETOOTH_Y PBL_IF_ROUND_ELSE(-25, -51)
#define BLUETOOTH_WIDTH 7
#define BLUETOOTH_HEIGHT 11
#define WEEKDAY_X PBL_IF_ROUND_ELSE(10, -21)
#define WEEKDAY_Y PBL_IF_ROUND_ELSE(-6, -32)
#define WEEKDAY_FONT_Y -5
#define WEEKDAY_BG_WIDTH 45
#define WEEKDAY_BG_HEIGHT 12
#define DATE_X PBL_IF_ROUND_ELSE(40, 9)
#define PIN_RADIUS 7
#define PIN_ICON_WIDTH 14

#define ANGLE_POINT(cx,cy,angle,radius) {.x=(int16_t)(sin_lookup((angle)/360.f*TRIG_MAX_ANGLE)*(radius)/TRIG_MAX_RATIO)+cx,.y=(int16_t)(-cos_lookup((angle)/360.f*TRIG_MAX_ANGLE)*(radius)/TRIG_MAX_RATIO)+cy}

void ui_bluetooth_set_available(bool is_available);
void ui_load(Window *window);
void ui_unload(void);
void ui_hide();
void ui_show();
void ui_update_config();
void ui_update_time(void);
void ui_update_date(void);
void ui_update_time_and_activity();
void ui_update_weather(void);
void ui_battery_charge_start(void);
void ui_battery_charge_stop(void);
void ui_set_temperature(int temperature);
void ui_set_weather_icon(uint32_t resource_id);
void ui_poll_activity(void* data);
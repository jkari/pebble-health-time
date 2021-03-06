#include <pebble.h>
#include "config.h"
#include "weather.h"
#include "ui.h"
#include "draw.h"
#include "health.h"
#include "gbitmap_color_palette_manipulator.h"

static TextLayer *s_layer_temperature;
static TextLayer *s_layer_day_of_month;
static TextLayer *s_layer_weekday;
static TextLayer *s_layer_steps;
static Layer *s_layer_canvas;
static Layer *s_layer_hands;
static Layer *s_layer_battery;
static GBitmap *s_bitmap_weather = 0;
static GBitmap *s_bitmap_bluetooth = 0;
static GBitmap *s_bitmap_sunrise = 0;
static GBitmap *s_bitmap_sunset = 0;
static GBitmap *s_bitmap_steps1 = 0;
static GBitmap *s_bitmap_steps2 = 0;
static GBitmap *s_bitmap_achievement = 0;
static BitmapLayer *s_layer_bluetooth;
static BitmapLayer *s_layer_weather;
static GFont s_font;
static bool s_is_battery_animation_active = false;
static int s_battery_animation_percent = 0;

static int get_hour_angle() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  return TRIG_MAX_ANGLE * ((tick_time->tm_hour % 12) / 12.f + tick_time->tm_min / (12.f * 60));
}

static void _ui_set_temperature() {
  layer_set_hidden((Layer*)s_layer_temperature, !(config_show_weather() && weather_is_available()));
  
  if (!weather_is_available()) {
    return;
  }
  
  static char temperature_buffer[8];
  
  int temperature = weather_get_temperature();
  
  if (temperature >= 0) {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "+%d", temperature);
  } else {
    snprintf(temperature_buffer, sizeof(temperature_buffer), "%d", temperature);
  }
  
  text_layer_set_text(s_layer_temperature, temperature_buffer);
}

static void _ui_reload_bitmap(GBitmap **image, uint32_t resource_id, GColor color) {
  if (*image == 0) {
    gbitmap_destroy(*image);
  }
  
  *image = gbitmap_create_with_resource(resource_id);
  
//#ifdef PBL_COLOR
  replace_gbitmap_color(GColorWhite, color, *image, NULL);
//#endif
}

static void _ui_set_weather_icon() {
  layer_set_hidden((Layer*)s_layer_weather, !(config_show_weather() && weather_is_available()));
  
  if (!weather_is_available()) {
    return;
  }
  
  int32_t resource_id = weather_get_resource_id(weather_get_condition());
  
  _ui_reload_bitmap(&s_bitmap_weather, resource_id, config_get_color_text());
  bitmap_layer_set_bitmap(s_layer_weather, s_bitmap_weather);
  
  layer_set_hidden((Layer *)s_layer_weather, false);
}

static void _generate_bitmaps() {
  LOG("Generating bitmaps");
  _ui_set_weather_icon();
  
  _ui_reload_bitmap(&s_bitmap_steps1, RESOURCE_ID_IMAGE_STEPS, config_get_color_text());
  _ui_reload_bitmap(&s_bitmap_steps2, RESOURCE_ID_IMAGE_STEPS, config_get_color_text2());
  
  _ui_reload_bitmap(&s_bitmap_achievement, RESOURCE_ID_IMAGE_ACHIEVEMENT, config_get_color_text());
  _ui_reload_bitmap(&s_bitmap_sunrise, RESOURCE_ID_IMAGE_SUN, config_get_color_text());
  _ui_reload_bitmap(&s_bitmap_sunset, RESOURCE_ID_IMAGE_MOON, config_get_color_text());
}

static void _charge_animation_callback(void *data) {
  s_battery_animation_percent = (s_battery_animation_percent + 2) % 100;
  
  if (s_is_battery_animation_active) {
    app_timer_register(30, _charge_animation_callback, NULL);
  }
  
  layer_mark_dirty(s_layer_battery);
}

static float _get_sunrise_ui_angle(GPoint offset) {
  float hour_sunrise = weather_get_sunrise_hour() + weather_get_sunrise_minute() / 60.f;
  hour_sunrise = (hour_sunrise > 12.f) ? hour_sunrise - 12.f : hour_sunrise;
  return 360.f * hour_sunrise / 12.f;
}

static float _get_sunset_ui_angle(GPoint offset) {
  float hour_sunset = weather_get_sunset_hour() + weather_get_sunset_minute() / 60.f;
  hour_sunset = (hour_sunset > 12.f) ? hour_sunset - 12.f : hour_sunset;
  return 360.f * hour_sunset / 12.f;
}

static void _draw_pin(GContext* ctx, GPoint offset, float angle, GBitmap* bitmap) {
  int size_x = DATA_SIZE_X - 2 * PIN_RADIUS - (2 * DATA_WIDTH + 2 * DATA_SPACING)
    * ((config_show_activity_progress() ? 1 : 0) + (config_show_sleep() ? 1 : 0) + (config_show_activity() ? 1 : 0));
  int size_y = DATA_SIZE_Y - 2 * PIN_RADIUS - (2 * DATA_WIDTH + 2 * DATA_SPACING)
    * ((config_show_activity_progress() ? 1 : 0) + (config_show_sleep() ? 1 : 0) + (config_show_activity() ? 1 : 0));
  GPoint icon_point = draw_get_arc_point(angle, GSize(size_x, size_y), offset);
  
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bitmap, GRect(icon_point.x - PIN_ICON_WIDTH/2, icon_point.y - PIN_ICON_WIDTH/2, PIN_ICON_WIDTH, PIN_ICON_WIDTH));
  
  graphics_context_set_stroke_color(ctx, config_get_color_pin_line());
  graphics_context_set_stroke_width(ctx, WIDTH_EVENT_POINTER);
  
  GPoint marker_point = draw_get_arc_point(angle, GSize(MARKER_SIZE_X, MARKER_SIZE_Y), offset);
  GPoint pin_point = draw_get_arc_point(angle, GSize(size_x + 2 * PIN_RADIUS, size_y + 2 * PIN_RADIUS), offset);
  
  graphics_draw_line(ctx, marker_point, pin_point);
}

static void _draw_pins_sun(GContext *ctx, GPoint offset) {
  _draw_pin(ctx, offset, _get_sunrise_ui_angle(offset), s_bitmap_sunrise);
  _draw_pin(ctx, offset, _get_sunset_ui_angle(offset), s_bitmap_sunset);
}

static void _draw_pins_achievement(GContext *ctx, GPoint offset) {
  if (health_is_activity_goal_achieved()) {
    _draw_pin(ctx, offset, health_get_activity_goal_angle(), s_bitmap_achievement);
  } 
}

static int _get_activity_level(int activity) {
  if (activity >= ACTIVITY_SPM_HIGH) {
    return 3;
  } else if (activity >= ACTIVITY_SPM_MEDIUM) {
    return 2;
  } else if (activity >= ACTIVITY_SPM_LOW) {
    return 1;
  }
  
  return 0;
}

static void _draw_activity_cycle(GContext *ctx, GPoint offset) {
  uint8_t* activity_data = health_get_activity();
  
  int current_index = health_get_index_for_time(time(NULL), true);
  int current_level_start_index = current_index;
  int current_activity_level = _get_activity_level(health_get_activity_value(activity_data, current_index));
  int size_x = DATA_SIZE_X 
    - (config_show_sleep() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0)
    - (config_show_activity_progress() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0);
  int size_y = DATA_SIZE_Y 
    - (config_show_sleep() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0)
    - (config_show_activity_progress() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0);
    
  for (int i = current_index; i < current_index + 12 * 12; i++) {
    int activity_level = _get_activity_level(health_get_activity_value(activity_data, i % (12 * 12)));
    
    if (activity_level != current_activity_level || i == current_index + 12 * 12 - 1) {
      if (current_activity_level > 0) {
        draw_arc(
          ctx, offset, GSize(size_x, size_y), DATA_WIDTH,
          current_level_start_index * (360.f / (12.f * 12.f)),
          i * (360.f / (12.f * 12.f)) + 0.1f,
          config_get_color_activity(current_activity_level)
        );
      }
      
      current_level_start_index = i;
      current_activity_level = activity_level;
    }
  }
}

void _draw_sleep_cycle(GContext *ctx, GPoint offset) {
  uint8_t* sleep_data = health_get_sleep();
  
  int current_index = health_get_index_for_time(time(NULL), false);
  int current_level_start_index = current_index;
  int current_sleep_level = health_get_sleep_value(sleep_data, current_index);
  int size_x = DATA_SIZE_X - (config_show_activity_progress() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0);
  int size_y = DATA_SIZE_Y - (config_show_activity_progress() ? (2 * DATA_WIDTH + 2 * DATA_SPACING) : 0);
  
  for (int i = current_index; i < current_index + 24 * 12; i++) {
    int sleep_level = (int)health_get_sleep_value(sleep_data, i % (12 * 24));
   
    if (sleep_level != current_sleep_level || i == current_index + 24 * 12 - 1) {
      if (current_sleep_level != 0) {
        draw_arc(
          ctx, offset, GSize(size_x, size_y), DATA_WIDTH,
          current_level_start_index * (360.f / (12.f * 12.f)),
          i * (360.f / (12.f * 12.f)) + 0.1f,
          config_get_color_sleep(current_sleep_level)
        );
      }
      
      current_level_start_index = i;
      current_sleep_level = sleep_level;
    }
  }
}

static void _activity_animation_callback(void *data) {
  layer_mark_dirty(s_layer_canvas);
}

static void _draw_activity_current(GContext* ctx, GPoint offset) {
  int current_activity = health_get_current_steps_per_minute();
  
  if (current_activity < CURRENT_ACTIVITY_SHOW_LIMIT_SPM) {
    layer_set_hidden((Layer *)s_layer_steps, true);
    return;
  }
  
  GBitmap *bitmap = 0;
  if (time(NULL) % (2 * UI_UPDATE_ACTIVITY_SECS) < UI_UPDATE_ACTIVITY_SECS) {
    bitmap = s_bitmap_steps1;
  } else {
    bitmap = s_bitmap_steps2;
  }
  
  graphics_draw_bitmap_in_rect(ctx, bitmap, GRect(offset.x + STEPS_X, offset.y + STEPS_Y, STEPS_WIDTH, STEPS_HEIGHT));
  
  static char value_buffer[5];
  snprintf(value_buffer, sizeof(value_buffer), "%d", current_activity);
  
  text_layer_set_text(s_layer_steps, value_buffer);
  layer_set_hidden((Layer *)s_layer_steps, false);
}

static void _draw_text_bg(GContext *ctx, GPoint offset) {
  graphics_context_set_fill_color(ctx, config_get_color_weekday_bg());
  
  graphics_fill_rect(ctx, GRect(offset.x + WEEKDAY_X, offset.y + WEEKDAY_Y, WEEKDAY_BG_WIDTH, WEEKDAY_BG_HEIGHT), 0, GCornerNone);
}

static void _draw_bg(GContext *ctx, GPoint offset, GRect area) {
  graphics_context_set_fill_color(ctx, config_get_color_bg());
  graphics_fill_rect(ctx, area, 0, GCornerNone);
}

static void _draw_activity_progress(GContext* ctx, GPoint offset) {
  float current_score = health_get_current_score();
  float avg_score = health_get_avg_score();
  
  float min_progress = current_score > avg_score ? avg_score : current_score;
  float max_progress = current_score > avg_score ? current_score : avg_score;
  
  draw_arc(
    ctx,
    offset,
    GSize(DATA_SIZE_X, DATA_SIZE_Y),
    DATA_WIDTH,
    0,
    min_progress * 360.f,
    config_get_color_current_activity_neutral()
  );
  
  GColor color = current_score > avg_score ? config_get_color_current_activity_plus() : config_get_color_current_activity_minus();
  
  draw_arc(
    ctx,
    offset,
    GSize(DATA_SIZE_X, DATA_SIZE_Y),
    DATA_WIDTH,
    min_progress * 360.f,
    max_progress * 360.f,
    color
  );
}

static void _draw_markers(GContext *ctx, GPoint offset) {
  LOG("%s", __func__);
  
  graphics_context_set_stroke_color(ctx, config_get_color_marker());
    
  for (int i = 0; i < 60; i++) {
    int length = i % 5 == 0 ? 12 : 8;
    float angle = 360.f * (i / 60.f);
    
    GPoint from = draw_get_arc_point(angle, GSize(MARKER_SIZE_X, MARKER_SIZE_Y), offset);
    GPoint to = draw_get_arc_point(angle, GSize(MARKER_SIZE_X - length, MARKER_SIZE_Y - length), offset);
    
    graphics_context_set_stroke_width(ctx, i % 5 == 0 ? 3 : 1);
    graphics_draw_line(ctx, from, to);
  }
}

static void _draw_hands(GContext *ctx, GPoint offset) {
  LOG("%s", __func__);

  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  int32_t minute_angle = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  int32_t hour_angle = get_hour_angle();
  
#ifdef DEMO
  hour_angle = TRIG_MAX_ANGLE * (7.333f / 12.f);
  minute_angle = TRIG_MAX_ANGLE * (20.f / 60.f);
#endif
  
  GPoint minute_to = {
    .x = (int16_t)(sin_lookup(minute_angle) * (int32_t)(HAND_LENGTH_MINUTE) / TRIG_MAX_RATIO) + offset.x,
    .y = (int16_t)(-cos_lookup(minute_angle) * (int32_t)(HAND_LENGTH_MINUTE) / TRIG_MAX_RATIO) + offset.y,
  };
  GPoint hour_to = {
    .x = (int16_t)(sin_lookup(hour_angle) * (int32_t)(HAND_LENGTH_HOUR) / TRIG_MAX_RATIO) + offset.x,
    .y = (int16_t)(-cos_lookup(hour_angle) * (int32_t)(HAND_LENGTH_HOUR) / TRIG_MAX_RATIO) + offset.y,
  };
  
  graphics_context_set_stroke_width(ctx, 5);
  graphics_context_set_stroke_color(ctx, config_get_color_minute());
  graphics_draw_line(ctx, offset, minute_to);
  graphics_context_set_stroke_color(ctx, config_get_color_hour());
  graphics_draw_line(ctx, offset, hour_to);
}

static void _layer_hands_update_callback(Layer *layer, GContext *ctx) {
  GRect bounds = layer_get_bounds(layer);
  
  _draw_hands(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2));
}

static void _layer_canvas_update_callback(Layer *layer, GContext *ctx) {
  LOG("%s", __func__);

  GRect bounds = layer_get_bounds(layer);
  GPoint offset = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  
  _draw_bg(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), bounds);
  
  if (config_show_activity_progress()) {
    _draw_activity_progress(ctx, offset);
  }
  
  if (config_show_pins_sun() && weather_is_available()) {
    _draw_pins_sun(ctx, offset);
  }
  
  if (config_show_pins_achievement()) {
    _draw_pins_achievement(ctx, offset);
  }
  
  if (config_show_activity_current()) {
    _draw_activity_current(ctx, offset);
  }
  
  if (config_show_sleep()) {
    _draw_sleep_cycle(ctx, offset);
  }
  
  if (config_show_activity()) {
    _draw_activity_cycle(ctx, offset);
  }
  
  _draw_text_bg(ctx, offset);
  _draw_markers(ctx, offset);
}

static void _layer_battery_update_callback(Layer *layer, GContext *ctx) {  
  BatteryChargeState charge_state = battery_state_service_peek();
  int current_charge = s_is_battery_animation_active ? s_battery_animation_percent : charge_state.charge_percent;
  
  for (int i = 0; i < current_charge; i+= 10) {
    graphics_context_set_fill_color(ctx, config_get_color_battery(i));
    graphics_fill_rect(ctx, GRect(i/10 * (BATTERY_BLOCK_WIDTH + BATTERY_BLOCK_SPACING), 0, BATTERY_BLOCK_WIDTH, BATTERY_BLOCK_HEIGHT), 0, GCornerNone); 
  }
}

void ui_update_date(void) {
  LOG("%s", __func__);

  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_day_of_month_buffer[6];
  strftime(s_day_of_month_buffer, sizeof(s_day_of_month_buffer), "%d", tick_time);
  
  static char** week_days;
  
  static char* en_week_days[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  week_days = en_week_days;
  
#ifdef DEMO
  strcpy(week_days[tick_time->tm_wday], "TUE");
  strcpy(s_day_of_month_buffer, "12");
#endif
  
  text_layer_set_text(s_layer_day_of_month, s_day_of_month_buffer);
  text_layer_set_text(s_layer_weekday, week_days[tick_time->tm_wday]);
}

void ui_load(Window *window) {
                              
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  s_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_16));
  
  s_bitmap_bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  s_bitmap_sunrise = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN);
  s_bitmap_sunset = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON);
  s_bitmap_steps1 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STEPS);
  s_bitmap_steps2 = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STEPS);
  s_bitmap_achievement = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACHIEVEMENT);
  
  s_layer_day_of_month = text_layer_create(GRect(center.x + DATE_X, center.y + WEEKDAY_Y + WEEKDAY_FONT_Y, 16, 16));
  text_layer_set_background_color(s_layer_day_of_month, GColorClear);
  text_layer_set_font(s_layer_day_of_month, s_font);
  text_layer_set_text_alignment(s_layer_day_of_month, GTextAlignmentRight);
  
  s_layer_weekday = text_layer_create(GRect(center.x + WEEKDAY_X + 1, center.y + WEEKDAY_Y + WEEKDAY_FONT_Y, 40, 16));
  text_layer_set_background_color(s_layer_weekday, GColorClear);
  text_layer_set_font(s_layer_weekday, s_font);
  text_layer_set_text_alignment(s_layer_weekday, GTextAlignmentLeft);

  s_layer_temperature = text_layer_create(GRect(center.x + WEATHER_TEMPERATURE_X, center.y + WEATHER_TEMPERATURE_Y, WEATHER_TEMPERATURE_WIDTH, WEATHER_TEMPERATURE_HEIGHT));
  text_layer_set_background_color(s_layer_temperature, GColorClear);
  text_layer_set_text_alignment(s_layer_temperature, GTextAlignmentCenter);
  text_layer_set_font(s_layer_temperature, s_font);
  
  s_layer_weather = bitmap_layer_create(GRect(center.x - WEATHER_ICON_WIDTH/2, center.y + WEATHER_ICON_Y, WEATHER_ICON_WIDTH, WEATHER_ICON_WIDTH));
  bitmap_layer_set_compositing_mode(s_layer_weather, GCompOpSet);
  bitmap_layer_set_bitmap(s_layer_weather, s_bitmap_weather);
  layer_set_hidden((Layer *)s_layer_weather, true);
  
  s_layer_bluetooth = bitmap_layer_create(GRect(center.x + BLUETOOTH_X, center.y + BLUETOOTH_Y, BLUETOOTH_WIDTH, BLUETOOTH_HEIGHT));
  bitmap_layer_set_compositing_mode(s_layer_bluetooth, GCompOpSet);
  bitmap_layer_set_bitmap(s_layer_bluetooth, s_bitmap_bluetooth);
  
  s_layer_canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_layer_canvas, _layer_canvas_update_callback);
  
  s_layer_hands = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_layer_hands, _layer_hands_update_callback);

  s_layer_battery = layer_create(GRect(center.x + BATTERY_X, center.y + BATTERY_Y, BATTERY_WIDTH, 20));
  layer_set_update_proc(s_layer_battery, _layer_battery_update_callback);
  
  s_layer_steps = text_layer_create(GRect(center.x + STEPS_X + STEPS_WIDTH + STEPS_TEXT_OFFSET_X, center.y + STEPS_Y + STEPS_TEXT_OFFSET_Y, STEPS_TEXT_WIDTH, STEPS_TEXT_HEIGHT));
  text_layer_set_background_color(s_layer_steps, GColorClear);
  text_layer_set_text_alignment(s_layer_steps, GTextAlignmentLeft);
  text_layer_set_font(s_layer_steps, s_font);
  layer_set_hidden((Layer *)s_layer_steps, true);
  
  layer_add_child(window_layer, s_layer_canvas);
  layer_add_child(window_layer, text_layer_get_layer(s_layer_steps));
  layer_add_child(window_layer, text_layer_get_layer(s_layer_day_of_month));
  layer_add_child(window_layer, text_layer_get_layer(s_layer_weekday));
  layer_add_child(window_layer, text_layer_get_layer(s_layer_temperature));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_layer_weather));
  layer_add_child(window_layer, bitmap_layer_get_layer(s_layer_bluetooth));
  layer_add_child(window_layer, s_layer_battery);
  layer_add_child(window_layer, s_layer_hands);
}

void ui_unload(void) {
  text_layer_destroy(s_layer_day_of_month);
  text_layer_destroy(s_layer_weekday);
  text_layer_destroy(s_layer_temperature);
  text_layer_destroy(s_layer_steps);
  
  bitmap_layer_destroy(s_layer_weather);
  bitmap_layer_destroy(s_layer_bluetooth);
  
  layer_destroy(s_layer_battery);
  layer_destroy(s_layer_canvas);
  
  fonts_unload_custom_font(s_font);
  
  gbitmap_destroy(s_bitmap_weather);
  gbitmap_destroy(s_bitmap_bluetooth);
  gbitmap_destroy(s_bitmap_sunrise);
  gbitmap_destroy(s_bitmap_sunset);
  gbitmap_destroy(s_bitmap_steps1);
  gbitmap_destroy(s_bitmap_steps2);
  gbitmap_destroy(s_bitmap_achievement);
}

void ui_show() {  
  Layer* layer_root = window_get_root_layer(layer_get_window(s_layer_canvas));
  layer_set_hidden(layer_root, false);
  layer_mark_dirty(layer_root);
}

void ui_hide() {
  Layer* layer_root = window_get_root_layer(layer_get_window(s_layer_canvas));
  layer_set_hidden(layer_root, true);
}

void ui_bluetooth_set_available(bool is_available) {
  layer_set_hidden((Layer *)s_layer_bluetooth, is_available);
}

void ui_battery_charge_start(void) {
  s_is_battery_animation_active = true;
  _charge_animation_callback(NULL);
}

void ui_battery_charge_stop(void) {
  s_is_battery_animation_active = false;
  layer_mark_dirty(s_layer_battery);
}

void ui_update_weather() {
  _ui_set_temperature();
  _ui_set_weather_icon();
  
  ui_show();
}

void ui_update_config() {
  _generate_bitmaps();

  text_layer_set_text_color(s_layer_temperature, config_get_color_text());
  text_layer_set_text_color(s_layer_weekday, config_get_color_bg());
  text_layer_set_text_color(s_layer_day_of_month, config_get_color_bg());
  text_layer_set_text_color(s_layer_steps, config_get_color_text());
  
  ui_show();
}
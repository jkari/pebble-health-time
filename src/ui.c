#include <pebble.h>
#include "config.h"
#include "weather.h"
#include "ui.h"
#include "health.h"
#include "gbitmap_color_palette_manipulator.h"

static TextLayer *s_layer_temperature;
static TextLayer *s_layer_day_of_month;
static TextLayer *s_layer_weekday;
static Layer *s_layer_canvas;
static Layer *s_layer_hands;
static Layer *s_layer_battery;
static GBitmap *s_bitmap_weather = 0;
static GBitmap *s_bitmap_bluetooth = 0;
static GBitmap *s_bitmap_sunrise = 0;
static GBitmap *s_bitmap_sunset = 0;
static GBitmap *s_bitmap_steps = 0;
static GBitmap *s_bitmap_achievement = 0;
static BitmapLayer *s_layer_bluetooth;
static BitmapLayer *s_layer_weather;
static GFont s_font_big;
static GFont s_font_small;
static bool is_battery_animation_active = false;
static int battery_animation_percent = 0;
static bool show_activity_icon = false;

static void _draw_arc(GContext* ctx, GPoint center, int radius, int width, float angle_from, float angle_to, GColor color) {
  graphics_context_set_fill_color(ctx, color);
  graphics_fill_radial(
    ctx,
    GRect(
      center.x - radius,
      center.y - radius,
      2 * radius,
      2 * radius
    ),
    GOvalScaleModeFitCircle,
    width,
    DEG_TO_TRIGANGLE(angle_from),
    DEG_TO_TRIGANGLE(angle_to)
  );
}

static int get_hour_angle() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  return TRIG_MAX_ANGLE * ((tick_time->tm_hour % 12) / 12.f + tick_time->tm_min / (12.f * 60));
}

static void _ui_set_temperature() {
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
  
  replace_gbitmap_color(GColorWhite, color, *image, NULL);
}

static void _ui_set_weather_icon() {
  int32_t resource_id = weather_get_resource_id(weather_get_condition());
  
  _ui_reload_bitmap(&s_bitmap_weather, resource_id, config_get_color_text());
  bitmap_layer_set_bitmap(s_layer_weather, s_bitmap_weather);
  
  layer_set_hidden((Layer *)s_layer_weather, false);
}

static void _generate_bitmaps() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Generating bitmaps");
  _ui_set_weather_icon();
  
  GColor color = GColorBlack;
  _ui_reload_bitmap(&s_bitmap_sunrise, RESOURCE_ID_IMAGE_SUN, color);
  _ui_reload_bitmap(&s_bitmap_sunset, RESOURCE_ID_IMAGE_MOON, color);
}

static void _charge_animation_callback(void *data) {
  battery_animation_percent = (battery_animation_percent + 2) % 100;
  
  if (is_battery_animation_active) {
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

static void _draw_weather_bg(GContext *ctx, GPoint offset) {
  graphics_context_set_fill_color(ctx, GColorLightGray);
  graphics_fill_circle(ctx, GPoint(offset.x, offset.y + WEATHER_ICON_Y + WEATHER_ICON_WIDTH/2), WEATHER_ICON_BG_RADIUS);
  
  graphics_context_set_fill_color(ctx, GColorWhite);
  graphics_fill_circle(ctx, GPoint(offset.x + WEATHER_TEMPERATURE_X + WEATHER_TEMPERATURE_WIDTH/2, offset.y + WEATHER_TEMPERATURE_Y + WEATHER_TEMPERATURE_HEIGHT/2), WEATHER_TEMPERATURE_BG_RADIUS);
}

static void _draw_pin(GContext* ctx, GPoint offset, float angle, GBitmap* bitmap) {
  GPoint icon_point = ANGLE_POINT(offset.x, offset.y, angle, PIN_ARC_RADIUS);
  
  graphics_context_set_compositing_mode(ctx, GCompOpSet);
  graphics_draw_bitmap_in_rect(ctx, bitmap, GRect(icon_point.x - PIN_ICON_WIDTH/2, icon_point.y - PIN_ICON_WIDTH/2, PIN_ICON_WIDTH, PIN_ICON_WIDTH));
  
  graphics_context_set_stroke_color(ctx, GColorDarkGray);
  graphics_context_set_stroke_width(ctx, WIDTH_EVENT_POINTER);
  
  graphics_draw_line(
    ctx,
    (GPoint)ANGLE_POINT(offset.x, offset.y, angle, ARC_RADIUS_MARKER),
    (GPoint)ANGLE_POINT(offset.x, offset.y, angle, PIN_ARC_RADIUS + PIN_RADIUS)
  );
}

static void _draw_pins(GContext *ctx, GPoint offset) {
  _draw_pin(ctx, offset, _get_sunrise_ui_angle(offset), s_bitmap_sunrise);
  _draw_pin(ctx, offset, _get_sunset_ui_angle(offset), s_bitmap_sunset);
  
  if (health_is_activity_goal_achieved()) {
    _draw_pin(ctx, offset, health_get_activity_goal_angle(), s_bitmap_achievement);
  } 
}

static GColor _get_activity_color(int level) {
  if (level == 1) {
    return GColorRajah;
  } else if (level == 2) {
    return GColorOrange;
  }
  
  return GColorRed;
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
  uint8_t activity_data[DATA_ARRAY_SIZE];
  health_get_activity(activity_data);
  
  int current_index = health_get_index_for_time(time(NULL), true);
  int current_level_start_index = current_index;
  int current_activity_level = _get_activity_level(health_get_activity_value(activity_data, current_index));
    
  for (int i = current_index; i < current_index + 12 * 12; i++) {
    int activity_level = _get_activity_level(health_get_activity_value(activity_data, i % (12 * 12)));
    
    if (activity_level != current_activity_level || i == current_index + 12 * 12 - 1) {
      if (current_activity_level > 0) {
        _draw_arc(
          ctx, offset, ARC_RADIUS_ACTIVITY, WIDTH_CIRCLE,
          current_level_start_index * (360.f / (12.f * 12.f)),
          i * (360.f / (12.f * 12.f)) + 0.1f,
          _get_activity_color(current_activity_level)
        );
      }
      
      current_level_start_index = i;
      current_activity_level = activity_level;
    }
  }
}

void _draw_sleep_cycle(GContext *ctx, GPoint offset) {
  uint8_t sleep_data[DATA_ARRAY_SIZE];
  health_get_sleep(sleep_data);
  
  int current_index = health_get_index_for_time(time(NULL), false);
  int current_level_start_index = current_index;
  int current_sleep_level = health_get_sleep_value(sleep_data, current_index);
  
  for (int i = current_index; i < current_index + 24 * 12; i++) {
    int sleep_level = (int)health_get_sleep_value(sleep_data, i % (12 * 24));
   
    if (sleep_level != current_sleep_level || i == current_index + 24 * 12 - 1) {
      if (current_sleep_level != 0) {
        _draw_arc(
          ctx, offset, ARC_RADIUS_SLEEP, WIDTH_CIRCLE,
          current_level_start_index * (360.f / (12.f * 12.f)),
          i * (360.f / (12.f * 12.f)) + 0.1f,
          current_sleep_level == 1 ? GColorPictonBlue : GColorDukeBlue
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

static void _draw_current_activity(GContext* ctx, GPoint offset) {
  int current_activity = health_get_current_steps_per_minute();
  
  if (current_activity < ACTIVITY_SHOW_SPM_MIN) {
    return;
  }
  
  show_activity_icon = !show_activity_icon;
  
  if (show_activity_icon) {
    graphics_draw_bitmap_in_rect(ctx, s_bitmap_steps, GRect(offset.x + STEPS_X - STEPS_WIDTH/2, offset.y + STEPS_Y, STEPS_WIDTH, STEPS_HEIGHT));
  }
  
  _draw_arc(
    ctx,
    GPoint(offset.x + STEPS_X, offset.y + STEPS_Y + STEPS_HEIGHT/2),
    STEPS_CIRCLE_RADIUS,
    STEPS_CIRCLE_WIDTH,
    0,
    (current_activity / 255.f) * 360.f,
    GColorRed
  );
}

static void _draw_text_bg(GContext *ctx, GPoint offset) {
  graphics_context_set_fill_color(ctx, GColorDarkGray);
  graphics_fill_rect(ctx, GRect(offset.x + WEEKDAY_X, offset.y + WEEKDAY_Y, WEEKDAY_BG_WIDTH, WEEKDAY_BG_HEIGHT), 0, GCornerNone);
}

static void _draw_bg(GContext *ctx, GPoint offset, GRect area) {
  graphics_context_set_fill_color(ctx, config_get_color_bg());
  graphics_fill_rect(ctx, area, 0, GCornerNone);
}

static void _draw_activity_progress(GContext* ctx, GPoint offset) {
  float current_score = health_get_current_score();
  float avg_score = health_get_avg_score();
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Current score %d, avg %d", (int)(current_score * 100.f), (int)(avg_score * 100.f));
  
  float min_progress = current_score > avg_score ? avg_score : current_score;
  float max_progress = current_score > avg_score ? current_score : avg_score;
  
  _draw_arc(
    ctx,
    offset,
    ACTIVITY_PROGRESS_RADIUS,
    ACTIVITY_PROGRESS_WIDTH,
    0,
    min_progress * 360.f,
    GColorBlue
  );
  
  GColor color = current_score > avg_score ? GColorGreen : GColorRed;
  
  _draw_arc(
    ctx,
    offset,
    ACTIVITY_PROGRESS_RADIUS,
    ACTIVITY_PROGRESS_WIDTH,
    min_progress * 360.f,
    max_progress * 360.f,
    color
  );
}

static void _draw_markers(GContext *ctx, GPoint offset) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Draw markers");
  
  graphics_context_set_stroke_color(ctx, config_get_color_marker());

  for (int i = 0; i < 60; i++) {
    int length = i % 5 == 0 ? 6 : 3;
    int angle = TRIG_MAX_ANGLE * (i / 60.0f);
    GPoint from = {
      .x = (sin_lookup(angle) * ARC_RADIUS_MARKER / TRIG_MAX_RATIO) + offset.x,
      .y = (-cos_lookup(angle) * ARC_RADIUS_MARKER / TRIG_MAX_RATIO) + offset.y,
    };
    GPoint to = {
      .x = (sin_lookup(angle) * (ARC_RADIUS_MARKER - length) / TRIG_MAX_RATIO) + offset.x,
      .y = (-cos_lookup(angle) * (ARC_RADIUS_MARKER - length) / TRIG_MAX_RATIO) + offset.y,
    };
    
    graphics_context_set_stroke_width(ctx, i % 5 == 0 ? 3 : 1);
    
    graphics_draw_line(ctx, from, to);
  }
}

static void _draw_hands(GContext *ctx, GPoint offset) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Draw hands");

  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  
  int32_t minute_angle = TRIG_MAX_ANGLE * tick_time->tm_min / 60;
  int32_t hour_angle = get_hour_angle();
  
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
  GPoint offset = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  
  _draw_hands(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2));
}

static void _layer_canvas_update_callback(Layer *layer, GContext *ctx) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Update canvas");

  GRect bounds = layer_get_bounds(layer);
  GPoint offset = GPoint(bounds.size.w / 2, bounds.size.h / 2);
  
  _draw_bg(ctx, GPoint(bounds.size.w / 2, bounds.size.h / 2), bounds);
  _draw_activity_progress(ctx, offset);
  _draw_pins(ctx, offset);
  _draw_current_activity(ctx, offset);
  _draw_sleep_cycle(ctx, offset);
  _draw_activity_cycle(ctx, offset);
  _draw_text_bg(ctx, offset);
  _draw_markers(ctx, offset);
}

static void _layer_battery_update_callback(Layer *layer, GContext *ctx) {  
  BatteryChargeState charge_state = battery_state_service_peek();
  int current_charge = is_battery_animation_active ? battery_animation_percent : charge_state.charge_percent;
  
  for (int i = 0; i < current_charge; i+= 10) {
    GColor color = GColorOrange;
    
    if (i <= BATTERY_LOW_MAX) {
      color = GColorRed;  
    } else if (i >= BATTERY_HIGH_MIN) {
      color = GColorGreen;
    }
    
    graphics_context_set_fill_color(ctx, color);
    graphics_fill_rect(ctx, GRect(i/10 * (BATTERY_BLOCK_WIDTH + BATTERY_BLOCK_SPACING), 0, BATTERY_BLOCK_WIDTH, BATTERY_BLOCK_HEIGHT), 0, GCornerNone); 
  }
}

void ui_update_date(void) {
  time_t temp = time(NULL);
  struct tm *tick_time = localtime(&temp);

  static char s_day_of_month_buffer[6];
  strftime(s_day_of_month_buffer, sizeof(s_day_of_month_buffer), "%d", tick_time);
  
  static char** week_days;
  
  if (true) {
    //static char* fi_week_days[7] = {"su", "ma", "ti", "ke", "to", "pe", "la"};
    static char* en_week_days[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
    week_days = en_week_days;
  }
  
  //static char s_weekday_buffer[4];
  //strftime(s_weekday_buffer, sizeof(s_weekday_buffer), "%a", tick_time);
  
  text_layer_set_text(s_layer_day_of_month, s_day_of_month_buffer);
  text_layer_set_text(s_layer_weekday, week_days[tick_time->tm_wday]);
  
  APP_LOG(APP_LOG_LEVEL_INFO, week_days[tick_time->tm_wday]);
}

void ui_load(Window *window) {
                              
  Layer *window_layer = window_get_root_layer(window);
  GRect bounds = layer_get_bounds(window_layer);
  GPoint center = grect_center_point(&bounds);

  s_font_big = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_16));
  s_font_small = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_8));
  
  s_bitmap_bluetooth = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BLUETOOTH);
  s_bitmap_sunrise = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_SUN);
  s_bitmap_sunset = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_MOON);
  s_bitmap_steps = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STEPS);
  s_bitmap_achievement = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACHIEVEMENT);
  
  s_layer_day_of_month = text_layer_create(GRect(center.x + DATE_X, center.y + WEEKDAY_Y + WEEKDAY_FONT_Y, 26, 26));
  text_layer_set_background_color(s_layer_day_of_month, GColorClear);
  text_layer_set_font(s_layer_day_of_month, s_font_big);
  text_layer_set_text_alignment(s_layer_day_of_month, GTextAlignmentLeft);
  
  s_layer_weekday = text_layer_create(GRect(center.x + WEEKDAY_X + 1, center.y + WEEKDAY_Y + WEEKDAY_FONT_Y, 40, 20));
  text_layer_set_background_color(s_layer_weekday, GColorClear);
  text_layer_set_font(s_layer_weekday, s_font_big);
  text_layer_set_text_alignment(s_layer_weekday, GTextAlignmentLeft);

  s_layer_temperature = text_layer_create(GRect(center.x + WEATHER_TEMPERATURE_X, center.y + WEATHER_TEMPERATURE_Y, WEATHER_TEMPERATURE_WIDTH, WEATHER_TEMPERATURE_HEIGHT));
  text_layer_set_background_color(s_layer_temperature, GColorClear);
  text_layer_set_text_alignment(s_layer_temperature, GTextAlignmentCenter);
  text_layer_set_font(s_layer_temperature, s_font_big);
  
  s_layer_weather = bitmap_layer_create(GRect(center.x - WEATHER_ICON_WIDTH/2, center.y + WEATHER_ICON_Y, WEATHER_ICON_WIDTH, WEATHER_ICON_WIDTH));
  bitmap_layer_set_compositing_mode(s_layer_weather, GCompOpSet);
  bitmap_layer_set_bitmap(s_layer_weather, s_bitmap_weather);
  layer_set_hidden((Layer *)s_layer_weather, true);
  
  s_layer_bluetooth = bitmap_layer_create(GRect(center.x + BLUETOOTH_OFFSET_X, center.y - 10, 20, 20));
  bitmap_layer_set_compositing_mode(s_layer_bluetooth, GCompOpSet);
  bitmap_layer_set_bitmap(s_layer_bluetooth, s_bitmap_bluetooth);
  
  s_layer_canvas = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_layer_canvas, _layer_canvas_update_callback);
  
  s_layer_hands = layer_create(GRect(0, 0, bounds.size.w, bounds.size.h));
  layer_set_update_proc(s_layer_hands, _layer_hands_update_callback);

  s_layer_battery = layer_create(GRect(center.x + BATTERY_X, center.y + BATTERY_Y, BATTERY_WIDTH, 20));
  layer_set_update_proc(s_layer_battery, _layer_battery_update_callback);
  
  layer_add_child(window_layer, s_layer_canvas);
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
  
  bitmap_layer_destroy(s_layer_weather);
  bitmap_layer_destroy(s_layer_bluetooth);
  
  layer_destroy(s_layer_battery);
  layer_destroy(s_layer_canvas);
  
  fonts_unload_custom_font(s_font_big);
  fonts_unload_custom_font(s_font_small);
  
  gbitmap_destroy(s_bitmap_weather);
  gbitmap_destroy(s_bitmap_bluetooth);
  gbitmap_destroy(s_bitmap_sunrise);
  gbitmap_destroy(s_bitmap_sunset);
  gbitmap_destroy(s_bitmap_steps);
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
  is_battery_animation_active = true;
  _charge_animation_callback(NULL);
}

void ui_battery_charge_stop(void) {
  is_battery_animation_active = false;
  layer_mark_dirty(s_layer_battery);
}

void ui_update_weather() {
  _ui_set_temperature();
  _ui_set_weather_icon();
  
  ui_show();
}

void ui_update_colors() {
  _generate_bitmaps();

  text_layer_set_text_color(s_layer_temperature, config_get_color_text());
  text_layer_set_text_color(s_layer_weekday, config_get_color_bg());
  text_layer_set_text_color(s_layer_day_of_month, config_get_color_bg());

  ui_show();
}

void ui_poll_activity(void* data) {
  if (health_get_current_steps_per_minute() >= ACTIVITY_SHOW_SPM_MIN) {
    app_timer_register(UI_UPDATE_ACTIVITY_MS, ui_poll_activity, NULL);
    layer_mark_dirty(s_layer_canvas);
  }
}
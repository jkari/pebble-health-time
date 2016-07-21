#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include "kiezelpay.h"
#include "weather.h"
#include "health.h"
#include "config.h"
#include "communication.h"
#include "ui.h"

#define DEBUG 1

static Window *s_main_window;
static EventHandle handle_tick_timer;
static EventHandle handle_battery_state;
static EventHandle handle_connection;
static EventHandle handle_app_focus;
  
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
  LOG("%s", __func__);
  
  if (tick_time->tm_min % 30 == 0) {
    weather_update();
    health_update_half_hour();
  }
  
  if (tick_time->tm_hour == 0 && tick_time->tm_min == 0) {
    ui_update_date();
  }
  
  health_update_minute();

  ui_show();
}

static void worker_message_handler(uint16_t type, AppWorkerMessage *data) {
  ui_show();
}

static void battery_handler(BatteryChargeState charge_state) {
  if (charge_state.is_charging) {
    ui_battery_charge_start();
  } else {
    ui_battery_charge_stop();
  }
}

static void _bluetooth_handler(bool is_connected) {
  ui_bluetooth_set_available(is_connected);
  
  vibes_double_pulse();
}

static void _focusing_handler(bool in_focus) {
  if (in_focus) {
    ui_hide();
  }
}

static void _focused_handler(bool in_focus) {
  if (in_focus) {
    ui_show();
  }
}

static void main_window_load(Window *window) {
  LOG("%s", __func__);
  
  health_init();
  health_update_half_hour();
  
  LOG("%d", (int)heap_bytes_free());
  
  kiezelpay_init();
  
  LOG("%d", (int)heap_bytes_free());
  
  communication_init();
  
  LOG("%d", (int)heap_bytes_free());
  
  ui_load(window);
  
  handle_tick_timer = events_tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
  handle_battery_state = events_battery_state_service_subscribe(battery_handler);
  
  ui_bluetooth_set_available(connection_service_peek_pebble_app_connection());
  handle_connection = events_connection_service_subscribe((ConnectionHandlers) {
    .pebble_app_connection_handler = _bluetooth_handler
  });
  
  handle_app_focus = events_app_focus_service_subscribe_handlers((AppFocusHandlers){
    .did_focus = _focused_handler,
    .will_focus = _focusing_handler
  });
  
  if (!app_worker_is_running()) {
    app_worker_launch();
  }
  
  app_worker_message_subscribe(worker_message_handler);
  
  ui_update_weather();
  ui_update_date();
  ui_update_config();
}

static void main_window_unload(Window *window) {
  LOG("%s", __func__);
  
  kiezelpay_deinit();
  
  communication_deinit();
  health_deinit();
  
  events_connection_service_unsubscribe(handle_connection);
  events_battery_state_service_unsubscribe(handle_battery_state);
  events_tick_timer_service_unsubscribe(handle_tick_timer);
  events_app_focus_service_unsubscribe(handle_app_focus);
  
  ui_unload();
}

static void handle_init(void) {
  s_main_window = window_create();
  
  window_set_window_handlers(s_main_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  window_stack_push(s_main_window, true);
}

static void handle_deinit(void) {
  window_destroy(s_main_window);
}

int main(void) {
  handle_init();
  app_event_loop();
  handle_deinit();
}
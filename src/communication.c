#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include "communication.h"
#include "config.h"
#include "weather.h"

static EventHandle handle_app_message_inbox_received;
static EventHandle handle_app_message_inbox_dropped;
static EventHandle handle_app_message_outbox_sent;
static EventHandle handle_app_message_outbox_failed;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  Tuple *message_type = dict_find(iterator, MESSAGE_KEY_MESSAGE_TYPE);

  APP_LOG(APP_LOG_LEVEL_INFO, "Message received");
  
  if (message_type) {
    return;
  }
  
  switch (message_type->value->int32) {
    case MESSAGE_TYPE_READY:
      communication_ready();
      break;
    case MESSAGE_TYPE_WEATHER:
      weather_received_callback(iterator);
      break;
    case MESSAGE_TYPE_CONFIG:
      config_received_callback(iterator);
      break;
  }
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Message %d handled", (int)message_type->value->int32);
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

void communication_init() {
  APP_LOG(APP_LOG_LEVEL_INFO, "Initializing communication");
  //events_app_message_request_inbox_size(1024);
  //events_app_message_request_outbox_size(128);
  
  handle_app_message_inbox_received = events_app_message_register_inbox_received(inbox_received_callback, NULL);
  handle_app_message_inbox_dropped = events_app_message_register_inbox_dropped(inbox_dropped_callback, NULL);
  handle_app_message_outbox_sent = events_app_message_register_outbox_sent(outbox_sent_callback, NULL);
  handle_app_message_outbox_failed = events_app_message_register_outbox_failed(outbox_failed_callback, NULL);
  
  events_app_message_open();
}

void communication_deinit() {
  events_app_message_unsubscribe(handle_app_message_inbox_received);
  events_app_message_unsubscribe(handle_app_message_inbox_dropped);
  events_app_message_unsubscribe(handle_app_message_outbox_sent);
  events_app_message_unsubscribe(handle_app_message_outbox_failed);
}

void communication_request_weather() {
  int use_celcius = config_get_use_celcius() ? 1 : 0;
  APP_LOG(APP_LOG_LEVEL_INFO, "Sending weather request");
  
  DictionaryIterator *iter;
  app_message_outbox_begin(&iter);
  dict_write_int(iter, MESSAGE_KEY_USE_CELCIUS, &use_celcius, sizeof(int), true);
  app_message_outbox_send();
}

void communication_ready() {
  weather_init();
}
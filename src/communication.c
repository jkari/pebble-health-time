#include <pebble.h>
#include <pebble-events/pebble-events.h>
#include "communication.h"
#include "config.h"
#include "weather.h"

static EventHandle handle_app_message_inbox_received;
static EventHandle handle_app_message_inbox_dropped;
static EventHandle handle_app_message_outbox_sent;
static EventHandle handle_app_message_outbox_failed;

MessageType _get_message_type(DictionaryIterator* iterator) {
  if (dict_find(iterator, MESSAGE_KEY_USE_CELCIUS) != NULL) {
    return MSG_TYPE_CONFIG;
  }
  
  Tuple *message_type = dict_find(iterator, MESSAGE_KEY_MESSAGE_TYPE);
  
  if (message_type != NULL) {
    return message_type->value->int32;
  }
  
  return MSG_TYPE_UNKNOWN;
}

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
  LOG("%s", __func__);
  
  switch (_get_message_type(iterator)) {
    case MSG_TYPE_CONFIG: config_received_callback(iterator); break;
    case MSG_TYPE_READY: communication_ready(); break;
    case MSG_TYPE_UNKNOWN: LOG("Received unknown message"); break;
  }
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  LOG("Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  LOG("Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  LOG("Outbox send success!");
}

void communication_init() {
  LOG("Initializing communication");
  events_app_message_request_inbox_size(256);
  events_app_message_request_outbox_size(256);
  
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

void communication_ready() {
  weather_init();
}
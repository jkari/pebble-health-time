#pragma once

#define MESSAGE_TYPE_READY 1
#define MESSAGE_TYPE_WEATHER 2
#define MESSAGE_TYPE_CONFIG 3

typedef enum {
  MSG_TYPE_CONFIG,
  MSG_TYPE_READY = 100,
  MSG_TYPE_UNKNOWN
} MessageType;

void communication_init();
void communication_deinit();
void communication_request_weather();
void communication_ready();
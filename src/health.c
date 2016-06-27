#include <pebble.h>
#include "health.h"
#include "config.h"
#include "ui.h"

void _save_health_block_activity(time_t now, int total) {
  persist_write_int(PERSIST_KEY_LAST_BLOCK_ACTIVITY_TIME, now);
  persist_write_int(PERSIST_KEY_LAST_BLOCK_ACTIVITY_VALUE, total);
}

void _set_timeline_activity_achieved() {
  persist_write_int(PERSIST_KEY_ACTIVITY_GOAL_TIME, time(NULL));
}
                    
int _get_today_total_activity() {
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  tick_time->tm_sec = 0;
  tick_time->tm_min = 0;
  tick_time->tm_hour = 0;
  time_t start = mktime(tick_time);
  
  HealthServiceAccessibilityMask result = 
      health_service_metric_accessible(HealthMetricStepCount, start, time(NULL));
  
  if (result & HealthServiceAccessibilityMaskAvailable) {
      HealthValue steps = health_service_sum(HealthMetricStepCount, start, time(NULL));
    
      return (int)steps;
  }
  
  return 0;
}

time_t _get_last_block_update() {
  return persist_exists(PERSIST_KEY_LAST_BLOCK_ACTIVITY_TIME) ? persist_read_int(PERSIST_KEY_LAST_BLOCK_ACTIVITY_TIME) : 0;
}

void _save_activity(int index, int value) {
  value = value > 255 ? 255 : value;
  
  uint8_t data[DATA_ARRAY_SIZE];
  health_get_activity(data);
  data[index] = (uint8_t)value;
  persist_write_data(PERSIST_KEY_ACTIVITY, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
}

void _set_sleep_value(uint8_t *data, int key, int value) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Set value at %d: %d", key, value);
  int index = key / 2;
  
  if (key % 2 == 0) {
    data[index] = data[index] & 0x0f;
    data[index] = data[index] | (value << 4);
  } else {
    data[index] = data[index] & 0xf0;
    data[index] = data[index] | value;
  }
}

void _save_sleep(int index, uint8_t value) {
  value = value > 15 ? 15 : value;
  
  uint8_t data[DATA_ARRAY_SIZE];
  health_get_sleep(data);
  _set_sleep_value(data, index, value);
  persist_write_data(PERSIST_KEY_SLEEP, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
}

int health_get_current_steps_per_minute() {
  return persist_read_int(PERSIST_KEY_LAST_ACTIVITY_STEPS_PER_MINUTE);
}

int health_get_sleep_value(uint8_t *data, int key) {
  //APP_LOG(APP_LOG_LEVEL_INFO, "Get value at %d", key);
  int index = key / 2;
  
  if (key % 2 == 0) {
    return (data[index] & 0xf0) >> 4;
  }
  
  return data[index] & 0x0f;
}

int health_get_activity_value(uint8_t *data, int key) {
  return data[key];
}

void health_get_activity(uint8_t *data) {
  if (persist_exists(PERSIST_KEY_ACTIVITY)) {
    persist_read_data(PERSIST_KEY_ACTIVITY, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      data[i] = 0;
    }
  }
  
#ifdef DEBUG
  data[16] = 10;
  data[17] = 20;
  data[18] = 50;
  data[19] = 100;
  data[20] = 70;
  data[21] = 30;
  data[40] = 10;
  data[41] = 70;
  data[42] = 150;
  data[43] = 255;
  data[44] = 200;
  data[45] = 150;
#endif
}

void health_get_sleep(uint8_t *data) {
  if (persist_exists(PERSIST_KEY_SLEEP)) {
    persist_read_data(PERSIST_KEY_SLEEP, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      data[i] = 0;
    }
  }
  
#ifdef DEBUG
  data[12] = 1;
  data[13] = 1;
  data[14] = 2;
  data[15] = 1;
  data[16] = 2;
  data[17] = 2;
  data[18] = 2;
  data[19] = 1;
#endif
}

int health_get_index_for_time(time_t time, bool suppressTo12Hours) {
  struct tm *tick = localtime(&time);
  int hour = tick->tm_hour;
  if (suppressTo12Hours) {
    hour = hour % 12;
  }
  
  return hour * 12 + (int)(tick->tm_min / 5); 
}

int _get_current_block_activity() {
  return ((float)_get_today_total_activity() - persist_read_int(PERSIST_KEY_LAST_BLOCK_ACTIVITY_VALUE)) / ACTIVITY_BLOCK_MINUTES;
}


void _save_current_activity() {
  int interval = time(NULL) - persist_read_int(PERSIST_KEY_LAST_ACTIVITY_TIME);
 
  if (interval < MIN_UPDATE_ACTIVITY_SECONDS) {
    return;
  }
  
  int new_steps = _get_today_total_activity() - persist_read_int(PERSIST_KEY_LAST_ACTIVITY_VALUE);
  int steps_per_minute = new_steps / ((float)interval / 60.f);

  persist_write_int(PERSIST_KEY_LAST_ACTIVITY_TIME, time(NULL));
  persist_write_int(PERSIST_KEY_LAST_ACTIVITY_VALUE, _get_today_total_activity());
  persist_write_int(PERSIST_KEY_LAST_ACTIVITY_STEPS_PER_MINUTE, steps_per_minute);
  
  ui_poll_activity(NULL);
}

int _get_avg_steps_between(time_t start, time_t end) {
  const HealthMetric metric = HealthMetricStepCount;
  const HealthServiceTimeScope scope = HealthServiceTimeScopeDaily;
  
  HealthServiceAccessibilityMask mask = 
            health_service_metric_averaged_accessible(metric, start, end, scope);
  
  if(mask & HealthServiceAccessibilityMaskAvailable) {
    return (int)health_service_sum_averaged(metric, start, end, scope);
  }
  
  return 0;
}

int _get_daily_avg_steps() {
  const time_t start = time_start_of_today();
  const time_t end = start + 24 * 3600 - 1;
  
  return _get_avg_steps_between(start, end);
}

int _get_current_steps() {
  return (int)health_service_sum_today(HealthMetricStepCount);
}

void health_update() {
  time_t last_time = _get_last_block_update();
  int last_index_activity = health_get_index_for_time(last_time, true);
  
  time_t current_time = time(NULL);
  int current_index_activity = health_get_index_for_time(current_time, true);
  
  int total_activity = _get_today_total_activity();
    
  _save_activity(last_index_activity, _get_current_block_activity());
  
  HealthActivityMask activities = health_service_peek_current_activities();
  
  int sleep_status = 0;

  if (activities & HealthActivityRestfulSleep) {
    sleep_status = 2;
  } else if (activities & HealthActivitySleep) {
    sleep_status = 1;
  }
  
  _save_sleep(health_get_index_for_time(current_time, false), sleep_status);
  
  if (last_index_activity != current_index_activity) {
    _save_health_block_activity(current_time, total_activity);
  }
  
  _save_current_activity(current_time, total_activity);
  
  if (health_get_current_score() >= 1.f && !health_is_activity_goal_achieved()) {
    _set_timeline_activity_achieved();
  }
}

float health_get_current_score() {
  float average = _get_daily_avg_steps();
  return average > 0 ? (float)_get_current_steps() / average : 0;
}

float health_get_avg_score() {
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  
  APP_LOG(APP_LOG_LEVEL_INFO, "Avg with time %d, total daily %d", _get_avg_steps_between(start, end), _get_daily_avg_steps());
  
  return (float)_get_avg_steps_between(start, end) / _get_daily_avg_steps();
}

bool health_is_activity_goal_achieved() {
  return persist_read_int(PERSIST_KEY_ACTIVITY_GOAL_TIME) > time_start_of_today();
}

float health_get_activity_goal_angle() {
  time_t time = persist_read_int(PERSIST_KEY_ACTIVITY_GOAL_TIME);
  struct tm *tick_time = localtime(&time);
  return 360.f * (tick_time->tm_hour % 12) / 12.f + tick_time->tm_min / 60.f;
}
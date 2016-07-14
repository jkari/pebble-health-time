#include <pebble.h>
#include "health.h"
#include "config.h"

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

int health_get_current_steps_per_minute() {
#ifdef DEMO
  return 80;
#else
  return persist_read_int(PERSIST_HEALTH_LAST_ACTIVITY_SPM);
#endif
}

int health_get_sleep_value(uint8_t *data, int key) {
#ifdef DEBUG
  return (key < 12 * 7) ? ((key / 2) % 15 < 8 ? 3 : 1) : 0;
#endif
  int index = key / 2;
  
  if (key % 2 == 0) {
    return (data[index] & 0xf0) >> 4;
  }
  
  return data[index] & 0x0f;
}

int health_get_activity_value(uint8_t *data, int key) {
#ifdef DEBUG
  if (key == 8 * 12 || key == 8 * 12 + 3) {
    return 50;
  } else if (key > 8 * 12 && key < 8 * 12 + 8) {
    return 100;
  } else if (key > 11 * 12 && key < 11 * 12 + 4) {
    return 50;
  } else if (key == 5 * 12 || key == 5 * 12 + 1 || key == 5 * 12 + 9 || key == 5 * 12 + 10) {
    return 50;
  } else if (key > 5 * 12 && key < 5 * 12 + 9) {
    return 200;
  }
#endif
  return data[key];
}

void health_get_activity(uint8_t *data) {
  if (persist_exists(PERSIST_HEALTH_ACTIVITY)) {
    persist_read_data(PERSIST_HEALTH_ACTIVITY, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      data[i] = 0;
    }
  }
}

void health_get_sleep(uint8_t *data) {
  if (persist_exists(PERSIST_HEALTH_SLEEP)) {
    persist_read_data(PERSIST_HEALTH_SLEEP, data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      data[i] = 0;
    }
  }
}

int health_get_index_for_time(time_t time, bool suppressTo12Hours) {
  struct tm *tick = localtime(&time);
  int hour = tick->tm_hour;
  if (suppressTo12Hours) {
    hour = hour % 12;
  }
  
  return hour * 12 + (int)(tick->tm_min / 5); 
}

float health_get_current_score() {
#ifdef DEMO_COMPLETE
  return 1.15f;
#elif DEMO_INCOMPLETE
  return 0.55f;
#elif DEBUG
  return 0.8f;
#endif
  
  float average = _get_daily_avg_steps();
  return average > 0 ? (float)_get_current_steps() / average : 0;
}

float health_get_avg_score() {
#ifdef DEBUG
  return 0.7f;
#endif
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  
  LOG("Avg with time %d, total daily %d", _get_avg_steps_between(start, end), _get_daily_avg_steps());
  
  return (float)_get_avg_steps_between(start, end) / _get_daily_avg_steps();
}

float health_get_activity_goal_angle() {
#ifdef DEMO_COMPLETE
  return 220.f;
#endif
  time_t time = persist_read_int(PERSIST_HEALTH_ACTIVITY_GOAL_TIME);
  struct tm *tick_time = localtime(&time);
  return 360.f * (tick_time->tm_hour % 12 + tick_time->tm_min / 60.f) / 12.f;
}

bool health_is_activity_goal_achieved() {
#ifdef DEMO_COMPLETE
  return true;
#endif
  return persist_read_int(PERSIST_HEALTH_ACTIVITY_GOAL_TIME) >= time_start_of_today();
}
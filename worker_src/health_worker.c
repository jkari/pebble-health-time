#include <pebble_worker.h>
#include "health_worker.h"

static uint8_t _sleep_data[DATA_ARRAY_SIZE];
static uint8_t _activity_data[DATA_ARRAY_SIZE];

void _load_activity_data() {
  if (persist_exists(PERSIST_HEALTH_ACTIVITY)) {
    persist_read_data(PERSIST_HEALTH_ACTIVITY, _activity_data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      _activity_data[i] = 0;
    }
  }
}

void _load_sleep_data() {
  if (persist_exists(PERSIST_HEALTH_SLEEP)) {
    persist_read_data(PERSIST_HEALTH_SLEEP, _sleep_data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  } else {
    for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
      _sleep_data[i] = 0;
    }
  }
}

static int _get_today_total_activity() {
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

static void _save_health_block_activity(time_t now, int total) {
  persist_write_int(PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_TIME, now);
  persist_write_int(PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_VALUE, total);
}

static int _get_current_block_activity() {
  return ((float)_get_today_total_activity() - persist_read_int(PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_VALUE)) / ACTIVITY_BLOCK_MINUTES;
}

static time_t _get_last_block_update() {
  return persist_exists(PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_TIME) ? persist_read_int(PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_TIME) : 0;
}

static int _get_index_for_time(time_t time, bool suppressTo12Hours) {
  struct tm *tick = localtime(&time);
  int hour = tick->tm_hour;
  if (suppressTo12Hours) {
    hour = hour % 12;
  }
  
  return hour * 12 + (int)(tick->tm_min / 5); 
}

static int _get_current_steps_per_minute() {
  return persist_read_int(PERSIST_HEALTH_LAST_ACTIVITY_SPM);
}

static void _set_timeline_activity_achieved() {
  persist_write_int(PERSIST_HEALTH_ACTIVITY_GOAL_TIME, time(NULL));
}

static void _set_activity(int index, int value) {
  value = value > 255 ? 255 : value;
  
  _activity_data[index] = (uint8_t)value;
}


static void _health_event_handler(HealthEventType event, void *context) {
  if (event == HealthEventMovementUpdate) {
    _save_current_activity();
  }
}

void _save_current_activity() {
  int interval = time(NULL) - persist_read_int(PERSIST_HEALTH_LAST_ACTIVITY_TIME);
 
  if (interval < MOVEMENT_UPDATE_MIN_SECONDS) {
    return;
  }
  
  health_service_events_unsubscribe();
  
  int new_steps = _get_today_total_activity() - persist_read_int(PERSIST_HEALTH_LAST_ACTIVITY_VALUE);
  int steps_per_minute = new_steps / ((float)interval / 60.f);

  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_TIME, time(NULL));
  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_VALUE, _get_today_total_activity());
  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_SPM, steps_per_minute);
  
  if (_get_current_steps_per_minute() >= FAST_POLL_MIN_SPM) {
    health_service_events_subscribe(_health_event_handler, NULL);
    AppWorkerMessage msg_data;
    app_worker_send_message(MESSAGE_ID_WORKER_INSTANT_UPDATE, &msg_data);
  }
}

static void _set_sleep(int key, uint8_t value) {
  value = value > 15 ? 15 : value;
  
  int index = key / 2;
  if (key % 2 == 0) {
    _sleep_data[index] = _sleep_data[index] & 0x0f;
    _sleep_data[index] = _sleep_data[index] | (value << 4);
  } else {
    _sleep_data[index] = _sleep_data[index] & 0xf0;
    _sleep_data[index] = _sleep_data[index] | value;
  }
}

static void _save_sleep_data() {
  persist_write_data(PERSIST_HEALTH_SLEEP, _sleep_data, DATA_ARRAY_SIZE * sizeof(uint8_t));
}

static void _save_activity_data() {
  persist_write_data(PERSIST_HEALTH_ACTIVITY, _activity_data, DATA_ARRAY_SIZE * sizeof(uint8_t));
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

float _get_current_score() {
  float average = _get_daily_avg_steps();
  return average > 0 ? (float)_get_current_steps() / average : 0;
}

bool _is_activity_goal_achieved() {
  return persist_read_int(PERSIST_HEALTH_ACTIVITY_GOAL_TIME) >= time_start_of_today();
}

bool _callback_sleep_data(HealthActivity activity, time_t time_start, time_t time_end, void *context) {
    int iterations = (time_end - time_start) / (60 * ACTIVITY_BLOCK_MINUTES) + 1;
    int start_index = _get_index_for_time(time_start, false);
    int sleep_type = (activity == HealthActivityRestfulSleep) ? 2 : 1;

    for (int i = start_index; i < start_index + iterations; i++) {
        _set_sleep(i, sleep_type);
    }

    return true;
}

void health_update_minute() {
  time_t last_time = _get_last_block_update();
  int last_index_activity = _get_index_for_time(last_time, true);
  
  time_t current_time = time(NULL);
  int current_index_activity = _get_index_for_time(current_time, true);
  
  int total_activity = _get_today_total_activity();
    
  _set_activity(last_index_activity, _get_current_block_activity());
  
  HealthActivityMask activities = health_service_peek_current_activities();

  int sleep_status = 0;

  if (activities & HealthActivityRestfulSleep) {
    sleep_status = 2;
  } else if (activities & HealthActivitySleep) {
    sleep_status = 1;
  }
  
  _set_sleep(_get_index_for_time(current_time, false), sleep_status);
  
  if (last_index_activity != current_index_activity) {
    _save_health_block_activity(current_time, total_activity);
  }
  
  _save_current_activity();

  if (_get_current_score() >= 1.f && !_is_activity_goal_achieved()) {
    _set_timeline_activity_achieved();
  }
}

void health_update_half_hour() {
  health_service_activities_iterate(
    HealthActivitySleep,
    time(NULL) - 3600,
    time(NULL),
    HealthIterationDirectionFuture,
    _callback_sleep_data,
    (void*)NULL
  );

  health_service_activities_iterate(
    HealthActivityRestfulSleep,
    time(NULL) - 3600,
    time(NULL),
    HealthIterationDirectionFuture,
    _callback_sleep_data,
    (void*)NULL
  );

  _save_activity_data();
  _save_sleep_data();
}

void health_init() {
  _load_activity_data();
  _load_sleep_data();
}

void health_deinit() {
  _save_activity_data();
  _save_sleep_data();
}

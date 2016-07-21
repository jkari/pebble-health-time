#include <pebble.h>
#include "health.h"
#include "config.h"

static uint8_t _sleep_data[DATA_ARRAY_SIZE];
static uint8_t _activity_data[DATA_ARRAY_SIZE];
static bool _is_fast_poll_active = false;
  
static int _get_index_for_time(time_t time, bool suppressTo12Hours) {
  struct tm *tick = localtime(&time);
  int hour = tick->tm_hour;
  if (suppressTo12Hours) {
    hour = hour % 12;
  }
  
  return hour * 12 + (int)(tick->tm_min / 5); 
}

static void _set_sleep(int key, uint8_t value, bool reset) {
  value = value > 15 ? 15 : value;
  
  int index = key / 2;
  if (key % 2 == 0) {
    if (reset) {
      _sleep_data[index] = _sleep_data[index] & 0x0f;
    }
    _sleep_data[index] = _sleep_data[index] | (value << 4);
  } else {
    if (reset) {
      _sleep_data[index] = _sleep_data[index] & 0xf0;
    }
    _sleep_data[index] = _sleep_data[index] | value;
  }
}

bool _callback_sleep_data(HealthActivity activity, time_t time_start, time_t time_end, void *context) {
  int iterations = (time_end - time_start) / (60 * ACTIVITY_BLOCK_MINUTES) + 1;
  int start_index = _get_index_for_time(time_start, false);
  int sleep_type = SLEEP_NORMAL | ((activity & HealthActivityRestfulSleep) ? SLEEP_RESTFUL : 0);
  APP_LOG(APP_LOG_LEVEL_INFO, "%s type=%d start=%d length=%d", __func__, sleep_type, start_index, iterations);

  for (int i = start_index; i < start_index + iterations; i++) {
    _set_sleep(i % DATA_ARRAY_SIZE, sleep_type, false);
  }

  return true;
}

void _load_activity_data() {
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    _activity_data[i] = 0;
  }
  
  if (persist_exists(PERSIST_HEALTH_ACTIVITY)) {
    persist_read_data(PERSIST_HEALTH_ACTIVITY, _activity_data, DATA_ARRAY_SIZE * sizeof(uint8_t));
  }
}

void _load_sleep_data() {
  for (int i = 0; i < DATA_ARRAY_SIZE; i++) {
    _sleep_data[i] = 0;
  }

  health_service_activities_iterate(
    HealthActivitySleep | HealthActivityRestfulSleep,
    time(NULL) - 24 * 3600,
    time(NULL),
    HealthIterationDirectionFuture,
    _callback_sleep_data,
    (void*)NULL
  );
}

bool _is_day_reset() {
  return (time(NULL) - time_start_of_today() < 30);
}

static int _get_today_total_activity() {
  if (_is_day_reset()) {
    return 0;
  }
  
  HealthServiceAccessibilityMask result = 
      health_service_metric_accessible(HealthMetricStepCount, time_start_of_today(), time(NULL));
  
  if (result & HealthServiceAccessibilityMaskAvailable) {
      HealthValue steps = health_service_sum(HealthMetricStepCount, time_start_of_today(), time(NULL));
    
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
 
  if (interval < MIN_UPDATE_ACTIVITY_SECONDS) {
    return;
  }
  
  int new_steps = _get_today_total_activity() - persist_read_int(PERSIST_HEALTH_LAST_ACTIVITY_VALUE);
  int steps_per_minute = new_steps / ((float)interval / 60.f);

  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_TIME, time(NULL));
  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_VALUE, _get_today_total_activity());
  persist_write_int(PERSIST_HEALTH_LAST_ACTIVITY_SPM, steps_per_minute);
  
  bool steps_exceeded = _get_current_steps_per_minute() >= FAST_POLL_MIN_SPM;
  
  if (!_is_fast_poll_active && steps_exceeded) {
    _is_fast_poll_active = true;
    health_service_events_subscribe(_health_event_handler, NULL);
  } else if (_is_fast_poll_active && !steps_exceeded) {
    _is_fast_poll_active = false;
    health_service_events_unsubscribe();
  }
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

void _check_goal() {
  if (_get_current_score() >= 1.f && !_is_activity_goal_achieved() && !_is_day_reset()) {
    _set_timeline_activity_achieved();
  }
}

int health_get_current_steps_per_minute() {
#ifdef DEMO_STEPS
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

uint8_t* health_get_activity(uint8_t *data) {
  return _activity_data;
}

uint8_t* health_get_sleep(uint8_t *data) {
  return _sleep_data;
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
  return 1.05f;
#elif DEMO_INCOMPLETE
  return 0.2f;
#elif DEBUG
  return 0.8f;
#endif
  
  float average = _get_daily_avg_steps();
  return average > 0 ? (float)_get_current_steps() / average : 0;
}

float health_get_avg_score() {
#ifdef DEMO_COMPLETE
  return 0.95f;
#elif DEMO_INCOMPLETE
  return 0.3f;
#endif
  const time_t start = time_start_of_today();
  const time_t end = time(NULL);
  
  LOG("Avg with time %d, total daily %d", _get_avg_steps_between(start, end), _get_daily_avg_steps());
  
  return (float)_get_avg_steps_between(start, end) / _get_daily_avg_steps();
}

float health_get_activity_goal_angle() {
#ifdef DEMO_COMPLETE
  return 200.f;
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
    sleep_status = SLEEP_RESTFUL | SLEEP_NORMAL;
  } else if (activities & HealthActivitySleep) {
    sleep_status = SLEEP_NORMAL;
  }
  
  _set_sleep(_get_index_for_time(current_time, false), sleep_status, true);
  
  if (last_index_activity != current_index_activity) {
    _save_health_block_activity(current_time, total_activity);
  }
  
  _save_current_activity();
  _check_goal();
}

void health_update_half_hour() {
  health_service_activities_iterate(
    HealthActivitySleep | HealthActivityRestfulSleep,
    time(NULL) - 3600,
    time(NULL),
    HealthIterationDirectionFuture,
    _callback_sleep_data,
    (void*)NULL
  );

  _save_activity_data();
}

void health_init() {
  LOG("%s", __func__);
  _load_activity_data();
  _load_sleep_data();
}

void health_deinit() {
  _save_activity_data();
}

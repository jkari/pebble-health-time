#include <pebble_worker.h>
#include "health_worker.h"
#include "worker.h"

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Worker minute update");
  health_update_minute();

  if (tick_timer->tm_min == 0 || tick_timer->tm_min == 30) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Worker half hour update");
    health_update_half_hour();
  }

  APP_LOG(APP_LOG_LEVEL_INFO, "Worker updates complete");
}

static void worker_init() {
  health_init();
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void worker_deinit() {
  health_deinit();
  tick_timer_service_unsubscribe();
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}
#include <pebble_worker.h>
#include "health_worker.h"
#include "worker.h"

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
  health_update_minute();

  if (tick_timer->tm_min == 0 || tick_timer->tm_min == 30) {
    health_update_half_hour();
  }
}

static void worker_init() {
  health_init();
  health_update_half_hour();
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
#include <pebble_worker.h>
#include "worker.h"
#include "health.h"

static void tick_handler(struct tm *tick_timer, TimeUnits units_changed) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Worker minute update");
  health_update();
}

static void worker_init() {
  tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
}

static void worker_deinit() {
  // Stop using the TickTimerService
  tick_timer_service_unsubscribe();
}

int main(void) {
  worker_init();
  worker_event_loop();
  worker_deinit();
}
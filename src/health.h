#pragma once

#define PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_TIME 100
#define PERSIST_HEALTH_LAST_BLOCK_ACTIVITY_VALUE 101
#define PERSIST_HEALTH_ACTIVITY 102
#define PERSIST_HEALTH_SLEEP 103
#define PERSIST_HEALTH_ACTIVITY_GOAL_TIME 104
#define PERSIST_HEALTH_LAST_ACTIVITY_TIME 105
#define PERSIST_HEALTH_LAST_ACTIVITY_VALUE 106
#define PERSIST_HEALTH_LAST_ACTIVITY_SPM 107

#define MIN_UPDATE_ACTIVITY_SECONDS 10
#define DATA_ARRAY_SIZE 144
#define ACTIVITY_BLOCK_MINUTES 5
#define ACTIVITY_SPM_LOW 30
#define ACTIVITY_SPM_MEDIUM 90
#define ACTIVITY_SPM_HIGH 150

int health_get_index_for_time(time_t time, bool suppressTo12Hours);
void health_update();
void health_get_activity(uint8_t *data);
void health_get_sleep(uint8_t *data);
float health_get_current_score();
int health_get_sleep_value(uint8_t *data, int key);
int health_get_activity_value(uint8_t *data, int key);
int health_get_current_steps_per_minute();
float health_get_avg_score();
void health_save_current_activity();
bool health_is_activity_goal_achieved();
float health_get_activity_goal_angle();
bool health_is_fast_update_active();
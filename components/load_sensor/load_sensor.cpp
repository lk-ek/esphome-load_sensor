#include "esphome/core/log.h"
#include "load_sensor.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <cstring>

namespace esphome {
namespace load_sensor {

static const char *const TAG = "load_sensor";

void LoadSensor::setup() {
  first_run_ = true;
  for (size_t i = 0; i < HISTORY_SIZE; i++) {
    history_[i] = 0;
  }
}

void LoadSensor::update() {
  char buffer[512];
  vTaskList(buffer);
  ESP_LOGD(TAG, "Task list:\n%s", buffer);

  uint32_t active_ticks = 0;
  uint32_t idle_ticks = 0;

  // Parse the task list buffer
  char *line = strtok(buffer, "\n");
  while (line != nullptr) {
    char name[16];
    char state;
    int prio;
    uint32_t stack;
    uint32_t time;
    
    if (sscanf(line, "%15s %c %d %u %u", name, &state, &prio, &stack, &time) == 5) {
      // Count only running (R) or executing (X) tasks
      if (state == 'R' || state == 'X') {
        if (strncmp(name, "IDLE", 4) == 0) {
          idle_ticks += time;
        } else {
          active_ticks += time;
        }
      }
    }
    line = strtok(nullptr, "\n");
  }

  if (active_ticks + idle_ticks > 0) {
    float instant_load = 100.0f * ((float)active_ticks / (float)(active_ticks + idle_ticks));
    
    // Add to moving average
    history_[history_index_] = instant_load;
    history_index_ = (history_index_ + 1) % HISTORY_SIZE;

    // Calculate simple average
    float avg_load = 0;
    for (size_t i = 0; i < HISTORY_SIZE; i++) {
      avg_load += history_[i];
    }
    avg_load /= HISTORY_SIZE;

    // Ensure load stays within 0-100%
    if (avg_load < 0.0f) avg_load = 0.0f;
    if (avg_load > 100.0f) avg_load = 100.0f;
    
    ESP_LOGD(TAG, "CPU Usage - Active: %u, Idle: %u, Current: %.1f%%, Average: %.1f%%", 
             active_ticks, idle_ticks, instant_load, avg_load);
    
    this->publish_state(avg_load);
  }
}

}  // namespace load_sensor
}  // namespace esphome
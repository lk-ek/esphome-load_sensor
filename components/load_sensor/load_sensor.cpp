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

uint32_t LoadSensor::calculate_delta(uint32_t current, uint32_t previous) {
  return (current >= previous) ? (current - previous) : 
         (UINT32_MAX - previous + current + 1);
}

void LoadSensor::update() {
  char buffer[512];
  vTaskGetRunTimeStats(buffer);  // Get runtime stats instead of task list
  ESP_LOGV(TAG, "Runtime stats:\n%s", buffer);

  uint32_t active_ticks = 0;
  uint32_t idle_ticks = 0;

  // Parse the runtime stats buffer
  char *line = strtok(buffer, "\n");
  while (line != nullptr) {
    char name[16];
    uint32_t count;
    uint32_t percent;
    
    if (sscanf(line, "%15s %u %u%%", name, &count, &percent) == 3) {
      ESP_LOGV(TAG, "Task: %s, Count: %u, Percent: %u%%", name, count, percent);
      if (strncmp(name, "IDLE", 4) == 0) {
        idle_ticks += count;
      } else {
        active_ticks += count;
      }
    }
    line = strtok(nullptr, "\n");
  }

  if (!first_run_) {
    uint32_t delta_active = calculate_delta(active_ticks, last_active_ticks_);
    uint32_t delta_idle = calculate_delta(idle_ticks, last_idle_ticks_);
    uint32_t delta_total = delta_active + delta_idle;

    if (delta_total > 0) {
      float instant_load = 100.0f * ((float)delta_active / (float)delta_total);
      
      if (instant_load >= 0.0f && instant_load <= 100.0f) {
        history_[history_index_] = instant_load;
        history_index_ = (history_index_ + 1) % HISTORY_SIZE;

        float avg_load = 0;
        for (size_t i = 0; i < HISTORY_SIZE; i++) {
          avg_load += history_[i];
        }
        avg_load /= HISTORY_SIZE;
        
        ESP_LOGD(TAG, "CPU Usage - Delta active: %u, Delta idle: %u, Current: %.1f%%, Average: %.1f%%", 
                 delta_active, delta_idle, instant_load, avg_load);
        
        this->publish_state(avg_load);
      }
    }
  } else {
    first_run_ = false;
  }

  last_active_ticks_ = active_ticks;
  last_idle_ticks_ = idle_ticks;
}

}  // namespace load_sensor
}  // namespace esphome
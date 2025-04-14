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
  // Initialize all history buffers
  for (size_t i = 0; i < HISTORY_1M; i++) {
    history_1m_[i] = 0;
  }
  for (size_t i = 0; i < HISTORY_5M; i++) {
    history_5m_[i] = 0;
  }
  for (size_t i = 0; i < HISTORY_15M; i++) {
    history_15m_[i] = 0;
  }
}

uint32_t LoadSensor::calculate_delta(uint32_t current, uint32_t previous) {
  return (current >= previous) ? (current - previous) : 
         (UINT32_MAX - previous + current + 1);
}

float LoadSensor::calculate_average(const float *history, size_t size) {
  float sum = 0;
  for (size_t i = 0; i < size; i++) {
    sum += history[i];
  }
  return sum / size;
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
        // Update 1-minute history
        history_1m_[history_index_1m_] = instant_load;
        history_index_1m_ = (history_index_1m_ + 1) % HISTORY_1M;
        
        // Update 5-minute history
        history_5m_[history_index_5m_] = instant_load;
        history_index_5m_ = (history_index_5m_ + 1) % HISTORY_5M;
        
        // Update 15-minute history
        history_15m_[history_index_15m_] = instant_load;
        history_index_15m_ = (history_index_15m_ + 1) % HISTORY_15M;

        // Calculate and publish averages
        float avg_1m = calculate_average(history_1m_, HISTORY_1M);
        float avg_5m = calculate_average(history_5m_, HISTORY_5M);
        float avg_15m = calculate_average(history_15m_, HISTORY_15M);
        
        ESP_LOGD(TAG, "Load averages: %.1f%% (1m), %.1f%% (5m), %.1f%% (15m)", 
                 avg_1m, avg_5m, avg_15m);
        
        if (this->load_1m) this->load_1m->publish_state(avg_1m);
        if (this->load_5m) this->load_5m->publish_state(avg_5m);
        if (this->load_15m) this->load_15m->publish_state(avg_15m);
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
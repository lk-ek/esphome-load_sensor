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

float LoadSensor::calculate_load(uint32_t idle_delta, uint32_t total_delta) {
  if (total_delta == 0 || idle_delta > total_delta) return 0.0f;
  return 100.0f * (1.0f - ((float)idle_delta / (float)total_delta));
}

void LoadSensor::update() {
  char buffer[512];
  vTaskGetRunTimeStats(buffer);
  ESP_LOGV(TAG, "Runtime stats:\n%s", buffer);  // Changed to LOGV to reduce noise

  uint32_t idle_ticks = 0;
  uint32_t total_ticks = 0;

  // Parse the runtime stats buffer
  char *line = strtok(buffer, "\n");
  while (line != nullptr) {
    char name[16];
    uint32_t ticks;
    if (sscanf(line, "%15s %u", name, &ticks) == 2) {
      if (strncmp(name, "IDLE", 4) == 0) {
        idle_ticks += ticks;
      }
      total_ticks += ticks;
    }
    line = strtok(nullptr, "\n");
  }

  if (!first_run_ && total_ticks > last_total_ticks_) {
    uint32_t delta_total = total_ticks - last_total_ticks_;
    uint32_t delta_idle = idle_ticks - last_idle_ticks_;
    
    // Instantaneous CPU load based on current measurement interval
    float instant_load = calculate_load(delta_idle, delta_total);
    
    // Add to moving average window
    history_[history_index_] = instant_load;
    history_index_ = (history_index_ + 1) % HISTORY_SIZE;

    // Calculate weighted moving average (newer values have more weight)
    float avg_load = 0;
    float total_weight = 0;
    for (size_t i = 0; i < HISTORY_SIZE; i++) {
      size_t age = (HISTORY_SIZE + history_index_ - i) % HISTORY_SIZE;
      float weight = HISTORY_SIZE - age;  // Newer values get higher weight
      avg_load += history_[i] * weight;
      total_weight += weight;
    }
    avg_load /= total_weight;

    // Ensure load stays within 0-100%
    if (avg_load < 0.0f) avg_load = 0.0f;
    if (avg_load > 100.0f) avg_load = 100.0f;
    
    ESP_LOGD(TAG, "CPU Load - Instant: %.1f%%, Moving Average: %.1f%% (over %d samples)", 
             instant_load, avg_load, HISTORY_SIZE);
    
    this->publish_state(avg_load);  // We publish the smoothed value
  } else {
    first_run_ = false;
  }

  last_total_ticks_ = total_ticks;
  last_idle_ticks_ = idle_ticks;
}

}  // namespace load_sensor
}  // namespace esphome
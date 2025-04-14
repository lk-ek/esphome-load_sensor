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
  vTaskGetRunTimeStats(buffer);
  ESP_LOGD(TAG, "Runtime stats:\n%s", buffer);

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

  if (!first_run_) {
    uint32_t delta_total = (total_ticks >= last_total_ticks_) ? 
                          (total_ticks - last_total_ticks_) : 
                          (UINT32_MAX - last_total_ticks_ + total_ticks);
                          
    uint32_t delta_idle = (idle_ticks >= last_idle_ticks_) ? 
                         (idle_ticks - last_idle_ticks_) : 
                         (UINT32_MAX - last_idle_ticks_ + idle_ticks);

    if (delta_total > 0) {
      float load = 100.0f * (1.0f - ((float)delta_idle / (float)delta_total));
      
      // Add to moving average
      history_[history_index_] = load;
      history_index_ = (history_index_ + 1) % HISTORY_SIZE;

      // Calculate average
      float avg_load = 0;
      for (size_t i = 0; i < HISTORY_SIZE; i++) {
        avg_load += history_[i];
      }
      avg_load /= HISTORY_SIZE;

      // Ensure load stays within 0-100%
      if (avg_load < 0.0f) avg_load = 0.0f;
      if (avg_load > 100.0f) avg_load = 100.0f;
      
      ESP_LOGD(TAG, "Delta total: %lu, Delta idle: %lu, Raw Load: %.1f%%, Avg Load: %.1f%%", 
               delta_total, delta_idle, load, avg_load);
      
      this->publish_state(avg_load);
    }
  } else {
    first_run_ = false;
  }

  last_total_ticks_ = total_ticks;
  last_idle_ticks_ = idle_ticks;
}

}  // namespace load_sensor
}  // namespace esphome
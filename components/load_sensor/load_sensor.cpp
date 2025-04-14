#include "esphome/core/log.h"
#include "load_sensor.h"
#include <cstring>

namespace esphome {
namespace load_sensor {

static const char *const TAG = "load_sensor";

void LoadSensor::setup() {
  first_run_ = true;
#ifdef USING_ESP8266
  // Check if runtime stats are enabled on ESP8266
  has_runtime_stats_ = (portGET_RUN_TIME_COUNTER_VALUE() != 0);
  if (!has_runtime_stats_) {
    ESP_LOGW(TAG, "FreeRTOS runtime stats not enabled. Add build_flags to enable them.");
  }
#endif
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

uint32_t LoadSensor::gather_stats() {
#if USE_FREERTOS
  char buffer[512];
  vTaskGetRunTimeStats(buffer);
  ESP_LOGV(TAG, "Runtime stats:\n%s", buffer);

  uint32_t active_ticks = 0;
  uint32_t idle_ticks = 0;

  char *line = strtok(buffer, "\n");
  while (line != nullptr) {
    char name[16];
    uint32_t count;
    uint32_t percent;
    
    if (sscanf(line, "%15s %u %u%%", name, &count, &percent) == 3) {
      if (strncmp(name, "IDLE", 4) == 0) {
        idle_ticks += count;
      } else {
        active_ticks += count;
      }
    }
    line = strtok(nullptr, "\n");
  }
  return active_ticks;
#else
  return gather_stats_esp8266();
#endif
}

#if !USE_FREERTOS
uint32_t LoadSensor::gather_stats_esp8266() {
  static uint32_t last_update = 0;
  uint32_t now = millis();
  
  // Calculate time slice and CPU usage
  uint32_t time_slice = now - last_update;
  last_update = now;
  
  // Get CPU cycles for this time slice
  uint32_t cycles = ESP.getCycleCount() / (ESP.getCpuFreqMHz() * 1000);  // Convert to ms
  
  // Estimate active time (empirically determined factors)
  uint32_t active_time = time_slice * cycles / (80 * time_slice);  // Assuming 80MHz base clock
  
  ESP_LOGV(TAG, "ESP8266 stats - Slice: %ums, Cycles: %u, Active: %ums", 
           time_slice, cycles, active_time);
  
  return active_time;
}
#endif

void LoadSensor::update() {
  uint32_t active_ticks = gather_stats();
  
#if USE_FREERTOS
  char buffer[512];
  vTaskGetRunTimeStats(buffer);
  ESP_LOGV(TAG, "Runtime stats:\n%s", buffer);

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
#else
  // ESP8266 specific update code
  uint32_t delta_time = calculate_delta(millis(), last_system_time_);
  uint32_t delta_active = calculate_delta(active_ticks, last_active_ticks_);

  ESP_LOGD(TAG, "ESP8266 timing - Active: %u, Total: %u", delta_active, delta_time);

  if (!first_run_ && delta_time > 0) {
    float instant_load = 100.0f * ((float)delta_active / (float)delta_time);
    
    ESP_LOGD(TAG, "ESP8266 instant load: %.1f%%", instant_load);
    
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
    } else {
      ESP_LOGW(TAG, "Invalid load value: %.1f%%, clamping", instant_load);
      instant_load = (instant_load < 0.0f) ? 0.0f : 100.0f;
    }
  } else {
    first_run_ = false;
  }

  last_active_ticks_ = active_ticks;
  last_system_time_ = millis();

  // ESP8266: Use sliding window for load calculation
  static uint32_t total_active = 0;
  static uint32_t total_time = 0;
  const uint32_t WINDOW_SIZE = 1000;  // 1 second window
  
  total_active = (total_active * 9 + active_ticks) / 10;
  total_time = (total_time * 9 + delta_time) / 10;
  
  if (total_time > 0) {
    float instant_load = 100.0f * ((float)total_active / (float)total_time);
    instant_load = std::min(100.0f, std::max(0.0f, instant_load));
    
    ESP_LOGV(TAG, "ESP8266 load calculation - Active: %u, Time: %u, Load: %.1f%%",
             total_active, total_time, instant_load);
    
    // Update histories
    // ...existing code...
  }
#endif
}

}  // namespace load_sensor
}  // namespace esphome
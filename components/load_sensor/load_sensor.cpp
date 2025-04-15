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
  static uint32_t last_time = 0;
  static uint32_t last_cycles = 0;
  static uint32_t idle_cycles_per_ms = 0;
  static bool baseline_set = false;
  
  uint32_t now = millis();
  uint32_t current_cycles = ESP.getCycleCount();
  
  if (first_run_) {
    last_time = now;
    last_cycles = current_cycles;
    return 0;
  }
  
  uint32_t time_delta = now - last_time;
  if (time_delta == 0) time_delta = 1;
  
  // Calculate cycles with overflow protection
  uint32_t cycle_delta;
  if (current_cycles < last_cycles) {
    cycle_delta = (0xFFFFFFFF - last_cycles) + current_cycles;
  } else {
    cycle_delta = current_cycles - last_cycles;
  }
  
  // Establish baseline on second run (typical idle cycle count)
  if (!baseline_set) {
    idle_cycles_per_ms = cycle_delta / time_delta;
    baseline_set = true;
    ESP_LOGD(TAG, "ESP8266 idle cycles/ms: %u", idle_cycles_per_ms);
    return 0;
  }
  
  // Calculate cycles in excess of idle baseline
  uint32_t total_cycles = time_delta * idle_cycles_per_ms;
  uint32_t excess_cycles = (cycle_delta > total_cycles) ? 
                          (cycle_delta - total_cycles) : 0;
  
  // Convert excess cycles to active time with 20% threshold
  uint32_t active_time = 0;
  if (excess_cycles > (total_cycles / 5)) {  // More than 20% above baseline
    active_time = (excess_cycles * time_delta) / cycle_delta;
  }
  
  ESP_LOGV(TAG, "ESP8266 stats - Time: %ums, Cycles: %u/%u, Active: %ums",
           time_delta, cycle_delta, total_cycles, active_time);
  
  last_time = now;
  last_cycles = current_cycles;
  
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
  // ESP8266 specific update code using loop time

  // Use a fixed baseline for idle loop time (empirically: 15ms is typical for idle with only loop_time sensor active)
  static constexpr float BASELINE_LOOP_TIME = 15.0f;   // ms, adjust as needed
  static constexpr float MAX_LOOP_TIME = 350.0f;      // ms, adjust as needed

  float current_loop_time = loop_time_ms_;
  // If an internal loop_time sensor is set, use its value
  if (loop_time_sensor_ != nullptr && !std::isnan(loop_time_sensor_->state)) {
    current_loop_time = loop_time_sensor_->state;
  }

  // Clamp current_loop_time to at least baseline
  if (current_loop_time < BASELINE_LOOP_TIME)
    current_loop_time = BASELINE_LOOP_TIME;

  float instant_load = (current_loop_time - BASELINE_LOOP_TIME) / (MAX_LOOP_TIME - BASELINE_LOOP_TIME);
  instant_load = std::min(1.0f, std::max(0.0f, instant_load));
  instant_load *= 100.0f;

  ESP_LOGD(TAG, "ESP8266 load (loop time): %.2f ms, Load: %.1f%%", current_loop_time, instant_load);

  if (!first_run_) {
    // Update histories with smoothed value
    history_1m_[history_index_1m_] = instant_load;
    history_index_1m_ = (history_index_1m_ + 1) % HISTORY_1M;
    
    history_5m_[history_index_5m_] = instant_load;
    history_index_5m_ = (history_index_5m_ + 1) % HISTORY_5M;
    
    history_15m_[history_index_15m_] = instant_load;
    history_index_15m_ = (history_index_15m_ + 1) % HISTORY_15M;

    float avg_1m = calculate_average(history_1m_, HISTORY_1M);
    float avg_5m = calculate_average(history_5m_, HISTORY_5M);
    float avg_15m = calculate_average(history_15m_, HISTORY_15M);
    
    ESP_LOGD(TAG, "Load averages: %.1f%% (1m), %.1f%% (5m), %.1f%% (15m)", 
             avg_1m, avg_5m, avg_15m);
    
    if (this->load_1m) this->load_1m->publish_state(avg_1m);
    if (this->load_5m) this->load_5m->publish_state(avg_5m);
    if (this->load_15m) this->load_15m->publish_state(avg_15m);
  } else {
    first_run_ = false;
  }
#endif
}

}  // namespace load_sensor
}  // namespace esphome
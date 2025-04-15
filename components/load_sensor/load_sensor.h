#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

// Platform detection
#if defined(USE_ESP_IDF)
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #define USE_FREERTOS 1
#elif defined(ESP32)
  #include "freertos/FreeRTOS.h"
  #include "freertos/task.h"
  #define USE_FREERTOS 1
#elif defined(ESP8266)
  #include <Arduino.h>
  #include <ESP8266WiFi.h>
  #include "esphome/components/debug/debug_component.h"  // <-- Use this path
  #define USE_FREERTOS 0
#else
  #error "Unsupported platform"
#endif

namespace esphome {
namespace load_sensor {

class LoadSensor : public PollingComponent, public sensor::Sensor {
 public:
  void setup() override;
  void update() override;

  void set_load_1m(sensor::Sensor *sens) { this->load_1m = sens; }
  void set_load_5m(sensor::Sensor *sens) { this->load_5m = sens; }
  void set_load_15m(sensor::Sensor *sens) { this->load_15m = sens; }

  #if !USE_FREERTOS
  void set_loop_time(float ms) { loop_time_ms_ = ms; }
  void set_loop_time(sensor::Sensor *sens) { loop_time_sensor_ = sens; }
  void set_baseline_loop_time(float ms) { baseline_loop_time_ = ms; }
  void set_max_loop_time(float ms) { max_loop_time_ = ms; }
  #endif

  sensor::Sensor *load_1m{nullptr};
  sensor::Sensor *load_5m{nullptr};
  sensor::Sensor *load_15m{nullptr};

 protected:
  static const size_t HISTORY_1M = 6;    // 6 * 10s = 1 minute
  static const size_t HISTORY_5M = 30;   // 30 * 10s = 5 minutes
  static const size_t HISTORY_15M = 90;  // 90 * 10s = 15 minutes

  float history_1m_[HISTORY_1M] = {0};
  float history_5m_[HISTORY_5M] = {0};
  float history_15m_[HISTORY_15M] = {0};
  
  size_t history_index_1m_ = 0;
  size_t history_index_5m_ = 0;
  size_t history_index_15m_ = 0;
  
  bool first_run_ = true;
  uint32_t last_active_ticks_ = 0;
  uint32_t last_idle_ticks_ = 0;

  uint32_t calculate_delta(uint32_t current, uint32_t previous);
  float calculate_average(const float *history, size_t size);

  #if !USE_FREERTOS
  uint32_t last_system_time_{0};
  uint32_t last_cycles_{0};
  uint32_t gather_stats_esp8266();
  float loop_time_ms_{0}; // Store latest loop time in ms
  sensor::Sensor *loop_time_sensor_{nullptr};
  float baseline_loop_time_{-1.0f}; // -1 means use default
  float max_loop_time_{-1.0f};      // -1 means use default
  #endif

  uint32_t gather_stats();
};

}  // namespace load_sensor
}  // namespace esphome
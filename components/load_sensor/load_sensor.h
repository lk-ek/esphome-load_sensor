#pragma once

#include "esphome/core/component.h"
#include "esphome/components/sensor/sensor.h"

namespace esphome {
namespace load_sensor {

class LoadSensor : public PollingComponent, public sensor::Sensor {
 public:
  void setup() override;
  void update() override;

 protected:
  static const size_t HISTORY_SIZE = 10;  // Increased from 3
  float history_[HISTORY_SIZE] = {0};
  size_t history_index_ = 0;
  uint32_t last_idle_ticks_ = 0;
  uint32_t last_total_ticks_ = 0;
  bool first_run_ = true;

  static const size_t MAX_TASKS = 16;
  struct TaskStats {
    char name[16];
    uint32_t last_runtime;
    eTaskState last_state;
  };
  TaskStats task_stats_[MAX_TASKS] = {};
  size_t num_tasks_ = 0;

  // Helper method
  float calculate_load(uint32_t idle_delta, uint32_t total_delta);
};

}  // namespace load_sensor
}  // namespace esphome
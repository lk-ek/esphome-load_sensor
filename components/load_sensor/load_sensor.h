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
  static const size_t HISTORY_SIZE = 10;
  float history_[HISTORY_SIZE] = {0};
  size_t history_index_ = 0;
  bool first_run_ = true;
  uint32_t last_active_ticks_ = 0;
  uint32_t last_idle_ticks_ = 0;

  uint32_t calculate_delta(uint32_t current, uint32_t previous);
};

}  // namespace load_sensor
}  // namespace esphome
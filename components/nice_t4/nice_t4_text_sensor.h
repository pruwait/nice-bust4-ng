#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "nice_t4_hub.h"

namespace esphome {
namespace nice_t4 {

class NiceT4TextSensor : public text_sensor::TextSensor, public Component {
 public:
  NiceT4TextSensor(NiceT4Hub *hub) : hub_(hub) {}

  void setup() override;
  void dump_config() override;

  // Метод для обновления значения извне (например, из Controller или Hub)
  void publish_state_custom(const std::string &state) {
    this->publish_state(state);
  }

 protected:
  NiceT4Hub *hub_;
};

}  // namespace nice_t4
}  // namespace esphome

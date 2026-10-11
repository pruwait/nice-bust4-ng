#pragma once

#include "esphome/core/component.h"
#include "esphome/components/text_sensor/text_sensor.h"
#include "nice_t4_hub.h"

namespace esphome {
namespace nice_t4 {

enum class TextSensorType : uint8_t {
  MENU_JSON = 0,
  MODEL_NAME = 1,
  FW_VERSION = 2,
  STATUS = 3,
  ERROR = 4,
  CAPABILITIES = 5,
};

class NiceT4TextSensor : public text_sensor::TextSensor, public Component {
 public:
  // Конструктор принимает ДВА параметра: hub и type
  NiceT4TextSensor(NiceT4Hub *hub, uint8_t type) 
      : hub_(hub), type_(static_cast<TextSensorType>(type)) {}

  void setup() override;
  void dump_config() override;

  // Метод для обновления значения извне (например, из Controller или Hub)
  void publish_state_custom(const std::string &state) {
    this->publish_state(state);
  }

  TextSensorType get_type() const { return type_; }

 protected:
  NiceT4Hub *hub_;
  TextSensorType type_;
};

}  // namespace nice_t4
}  // namespace esphome

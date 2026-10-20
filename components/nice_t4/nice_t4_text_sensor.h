#pragma once
#include "esphome/components/text_sensor/text_sensor.h"
#include "esphome/core/component.h"

#include "nice_t4_hub.h"

namespace esphome {
namespace nice_t4 {

enum class TextType : uint8_t { STATUS = 0, LAST_EVENT = 1 };

class NiceT4TextSensor : public text_sensor::TextSensor, public Component, public HubListener {
 public:
  NiceT4TextSensor(NiceT4Hub *hub, uint8_t type) : hub_(hub), type_(static_cast<TextType>(type)) {}
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  void on_gate_state(GateState s) override;
  void on_event(const Frame &f) override;

 protected:
  NiceT4Hub *hub_;
  TextType type_;
};

}  // namespace nice_t4
}  // namespace esphome

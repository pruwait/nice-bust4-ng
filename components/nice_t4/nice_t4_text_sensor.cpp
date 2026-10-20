#include "nice_t4_text_sensor.h"

#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.text_sensor";

void NiceT4TextSensor::setup() {
  hub_->add_listener(this);
  if (type_ == TextType::STATUS) this->publish_state(gate_state_name(hub_->gate_state()));
}

void NiceT4TextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Nice T4 Text Sensor", this);
  ESP_LOGCONFIG(TAG, "  Тип: %s", type_ == TextType::STATUS ? "status" : "last_event");
}

void NiceT4TextSensor::on_gate_state(GateState s) {
  if (type_ == TextType::STATUS) this->publish_state(gate_state_name(s));
}

void NiceT4TextSensor::on_event(const Frame &f) {
  if (type_ != TextType::LAST_EVENT) return;
  uint8_t buf[kMaxFrame];
  const size_t n = encode(f, buf, sizeof(buf));
  char hex[kMaxFrame * 3 + 1];
  format_hex(buf, n, hex, sizeof(hex));
  this->publish_state(hex);
}

}  // namespace nice_t4
}  // namespace esphome

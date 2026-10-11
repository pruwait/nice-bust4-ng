#include "nice_t4_text_sensor.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.text_sensor";

void NiceT4TextSensor::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Nice T4 Text Sensor...");
  // Начальное состояние
  this->publish_state("Initializing...");
  
  // TODO P2/P3: Здесь можно запросить у hub_->get_controller() 
  // версию прошивки или модель и вызвать publish_state_custom(...)
}

void NiceT4TextSensor::dump_config() {
  LOG_TEXT_SENSOR("", "Nice T4 Text Sensor", this);
}

}  // namespace nice_t4
}  // namespace esphome

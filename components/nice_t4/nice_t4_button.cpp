#include "nice_t4_button.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.button";

void NiceT4Button::dump_config() {
  LOG_BUTTON("", "Nice T4 Button", this);
  ESP_LOGCONFIG(TAG, "  Command: 0x%02X", this->command_);
}

void NiceT4Button::press_action() {
  ESP_LOGD(TAG, "Button pressed, sending command: 0x%02X", this->command_);
  
  // TODO P2/P3: Реализовать формирование кадра (Frame) и отправку через T4Engine
  // Пример:
  // Frame frame(Address(0x04), Address(0x01), Proto::DEP, {this->command_});
  // RequestOptions opt;
  // hub_->get_engine()->submit(frame, opt, nullptr, nullptr);
}

}  // namespace nice_t4
}  // namespace esphome

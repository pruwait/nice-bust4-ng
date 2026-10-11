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
  ESP_LOGD(TAG, ">>> Button pressed, command: 0x%02X", this->command_);
  
  if (hub_) {
    bool res = hub_->send_dep_cmd(this->command_);
    if (!res) {
      ESP_LOGE(TAG, ">>> send_dep_cmd returned false!");
    }
  } else {
    ESP_LOGE(TAG, ">>> Hub pointer is null!");
  }
}

}  // namespace nice_t4
}  // namespace esphome

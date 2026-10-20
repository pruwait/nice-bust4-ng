#include "nice_t4_button.h"

#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.button";

void NiceT4Button::dump_config() {
  LOG_BUTTON("", "Nice T4 Button", this);
  ESP_LOGCONFIG(TAG, "  Команда DEP: 0x%02X", command_);
}

void NiceT4Button::press_action() {
  if (!hub_->send_dep_cmd(command_)) ESP_LOGW(TAG, "Команда 0x%02X не отправлена", command_);
}

}  // namespace nice_t4
}  // namespace esphome

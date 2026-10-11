#include "nice_t4_cover.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.cover";

void NiceT4Cover::setup() {
  auto traits = this->get_traits();
  traits.set_supports_stop(true);
  this->cover_traits_ = traits;
  ESP_LOGI(TAG, "Nice T4 Cover initialized");
  
  // Начальное состояние (будет обновляться по мере опроса на этапе P2/P3)
  this->position = cover::COVER_CLOSED;
  this->current_operation = cover::COVER_OPERATION_IDLE;
  this->publish_state();
}

cover::CoverTraits NiceT4Cover::get_traits() {
  return this->cover_traits_;
}

void NiceT4Cover::control(const cover::CoverCall &call) {
  if (!hub_) {
    ESP_LOGE(TAG, "Hub is not initialized!");
    return;
  }

  if (call.get_stop()) {
    ESP_LOGD(TAG, "Cover STOP command");
    hub_->send_dep_cmd(cmd::STOP);
    this->current_operation = cover::COVER_OPERATION_IDLE;
    this->publish_state();
  } else if (call.get_position().has_value()) {
    float pos = *call.get_position();
    if (pos == cover::COVER_OPEN) {
      ESP_LOGD(TAG, "Cover OPEN command");
      hub_->send_dep_cmd(cmd::OPEN);
      this->current_operation = cover::COVER_OPERATION_OPENING;
      this->position = cover::COVER_OPEN;
      this->publish_state();
    } else if (pos == cover::COVER_CLOSED) {
      ESP_LOGD(TAG, "Cover CLOSE command");
      hub_->send_dep_cmd(cmd::CLOSE);
      this->current_operation = cover::COVER_OPERATION_CLOSING;
      this->position = cover::COVER_CLOSED;
      this->publish_state();
    }
  }
}

}  // namespace nice_t4
}  // namespace esphome

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/cover/cover.h"
#include "nice_t4_hub.h"
#include "t4_types.h"

namespace esphome {
namespace nice_t4 {

class NiceT4Cover : public cover::Cover, public Component {
 public:
  NiceT4Cover(NiceT4Hub *hub) : hub_(hub) {}

  void setup() override {
    auto traits = this->get_traits();
    traits.set_supports_stop(true);
    this->cover_traits_ = traits;
    ESP_LOGI("nice_t4.cover", "Nice T4 Cover initialized");
    
    // TODO P2: вызвать hub_->get_controller()->start_initial_poll();
  }

  cover::CoverTraits get_traits() override {
    return this->cover_traits_;
  }

  void control(const cover::CoverCall &call) override {
    if (call.get_stop()) {
      this->send_command_(cmd::STOP);
    } else if (call.get_position().has_value()) {
      float pos = *call.get_position();
      if (pos == cover::COVER_OPEN) {
        this->send_command_(cmd::OPEN);
      } else if (pos == cover::COVER_CLOSED) {
        this->send_command_(cmd::CLOSE);
      }
    }
  }

 protected:
  void send_command_(uint8_t command) {
    ESP_LOGD("nice_t4.cover", "Sending command: 0x%02X", command);
    // TODO P2: реализовать формирование кадра (Frame) и вызов:
    // hub_->get_engine()->submit(frame, options, callback, context);
  }

  NiceT4Hub *hub_;
  cover::CoverTraits cover_traits_;
};

}  // namespace nice_t4
}  // namespace esphome

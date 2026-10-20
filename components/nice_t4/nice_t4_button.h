#pragma once
#include "esphome/components/button/button.h"
#include "esphome/core/component.h"

#include "nice_t4_hub.h"

namespace esphome {
namespace nice_t4 {

class NiceT4Button : public button::Button, public Component {
 public:
  NiceT4Button(NiceT4Hub *hub, uint8_t command) : hub_(hub), command_(command) {}
  void dump_config() override;

 protected:
  void press_action() override;
  NiceT4Hub *hub_;
  uint8_t command_;
};

}  // namespace nice_t4
}  // namespace esphome

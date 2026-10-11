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

  void setup() override;
  cover::CoverTraits get_traits() override;
  void control(const cover::CoverCall &call) override;

 protected:
  NiceT4Hub *hub_;
  cover::CoverTraits cover_traits_;
};

}  // namespace nice_t4
}  // namespace esphome

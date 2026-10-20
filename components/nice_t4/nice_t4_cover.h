#pragma once
#include "esphome/components/cover/cover.h"
#include "esphome/core/component.h"

#include "nice_t4_hub.h"

namespace esphome {
namespace nice_t4 {

// Ворота как cover: открыть, закрыть, стоп, переключить (SBS). Состояние берётся из событий привода
// и из периодического опроса регистра 01, а не из «оптимистичного» предположения.
class NiceT4Cover : public cover::Cover, public Component, public HubListener {
 public:
  explicit NiceT4Cover(NiceT4Hub *hub) : hub_(hub) {}
  void setup() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }
  cover::CoverTraits get_traits() override;
  void on_gate_state(GateState s) override;

 protected:
  void control(const cover::CoverCall &call) override;
  NiceT4Hub *hub_;
  bool published_ = false;
};

}  // namespace nice_t4
}  // namespace esphome

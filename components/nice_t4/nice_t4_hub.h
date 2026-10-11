#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "t4_engine.h"

namespace esphome {
namespace nice_t4 {

class NiceT4Hub : public Component, public uart::UARTDevice, public Transport {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  // Реализация интерфейса Transport для T4Engine
  uint32_t now_ms() override { return millis(); }
  void send_break() override;
  void write(const uint8_t *data, size_t n) override;
  int read_byte() override;

  T4Engine* get_engine() { return engine_.get(); }

 protected:
  std::unique_ptr<T4Engine> engine_;
};

}  // namespace nice_t4
}  // namespace esphome

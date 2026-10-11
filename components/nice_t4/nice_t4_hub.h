#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "t4_engine.h"

#ifdef USE_ESP8266
#include <uart.h>
#endif

namespace esphome {
namespace nice_t4 {

class NiceT4Hub : public Component, public uart::UARTDevice, public Transport {
 public:
  void setup() override;
  void loop() override;
  void dump_config() override;

  void set_address(uint8_t addr) { address_ = addr; }
  void set_identity(uint8_t id) { identity_ = id; }
  void set_model(const std::string &model) { model_ = model; }
  void set_allow_write(bool allow) { allow_write_ = allow; }

  uint32_t now_ms() override { return millis(); }
  void send_break() override;
  void write(const uint8_t *data, size_t n) override;
  int read_byte() override;

  T4Engine* get_engine() { return engine_.get(); }
  uint8_t get_address() const { return address_; }
  uint8_t get_identity() const { return identity_; }
  const std::string& get_model() const { return model_; }
  bool get_allow_write() const { return allow_write_; }

  bool send_dep_cmd(uint8_t cmd_code);

 protected:
  std::unique_ptr<T4Engine> engine_;
  uint8_t address_ = 0x66;
  uint8_t identity_ = 0x01;
  std::string model_ = "auto";
  bool allow_write_ = true;

#ifdef USE_ESP8266
  uart_t* hw_uart_ = nullptr; // Прямой указатель на аппаратный UART
#endif
};

}  // namespace nice_t4
}  // namespace esphome

#pragma once
#include <string>

#include "esphome/components/uart/uart.h"
#include "esphome/core/component.h"

#include "t4_engine.h"
#include "t4_state.h"
#include "uart_transport.h"

namespace esphome {
namespace nice_t4 {

// Подписчик хаба (cover, text_sensor и др.). Без std::function: работает одинаково на всех версиях ESPHome.
class HubListener {
 public:
  virtual ~HubListener() {}
  virtual void on_gate_state(GateState) {}
  virtual void on_event(const Frame &) {}
};

class NiceT4Hub : public Component, public uart::UARTDevice {
 public:
  static const uint8_t kMaxListeners = 6;

  NiceT4Hub() : transport_(this), engine_(transport_, Address(0x00, 0x66)) {}

  void setup() override;
  void loop() override;
  void dump_config() override;
  float get_setup_priority() const override { return setup_priority::DATA; }

  // Настройки из yaml. Адреса 16-битные: старший байт ADR, младший EPT.
  void set_address(uint16_t a) { self_ = Address(static_cast<uint8_t>(a >> 8), static_cast<uint8_t>(a & 0xFF)); }
  void set_target(uint16_t a) { target_ = Address(static_cast<uint8_t>(a >> 8), static_cast<uint8_t>(a & 0xFF)); }
  void set_identity(uint8_t v) { identity_ = v; }
  void set_model(const std::string &m) { model_ = m; }
  void set_allow_write(bool v) { allow_write_ = v; }        // запись параметров (SET)
  void set_allow_commands(bool v) { allow_commands_ = v; }  // команды управления (CMD)
  void set_break_mode(uint8_t m) { break_mode_ = static_cast<BreakMode>(m); }
  void set_status_poll_interval(uint32_t ms) { status_poll_ms_ = ms; }

  // Команда DEP (код из DepCmd или cmd::). Отправляется один раз, подтверждение приходит событием.
  bool send_dep_cmd(uint8_t code);
  // Запись регистра (SET). Нужен allow_write.
  bool write_register(uint8_t device, uint8_t reg, const uint8_t *data, uint8_t len);

  bool add_listener(HubListener *l);
  GateState gate_state() const { return gate_state_; }
  T4Engine &engine() { return engine_; }

 protected:
  static void on_event_cb(void *ctx, const Frame &f);
  static void on_status_cb(void *ctx, Result r, const Frame *f);
  void handle_event(const Frame &f);
  void handle_status(Result r, const Frame *f);
  void set_state(GateState s);
  void poll_status();

  UartTransport transport_;
  T4Engine engine_;
  Address self_ = Address(0x00, 0x66);
  Address target_ = Address(0x00, 0xFF);
  uint8_t identity_ = static_cast<uint8_t>(Device::OVIEW);
  std::string model_ = "auto";
  bool allow_write_ = false;
  bool allow_commands_ = true;
  BreakMode break_mode_ = BreakMode::BYTE;
  uint32_t status_poll_ms_ = 15000;
  uint32_t next_poll_ms_ = 0;
  bool poll_pending_ = false;
  GateState gate_state_ = GateState::UNKNOWN;
  HubListener *listeners_[kMaxListeners] = {nullptr};
  uint8_t listener_count_ = 0;
};

}  // namespace nice_t4
}  // namespace esphome

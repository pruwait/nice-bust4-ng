#include "nice_t4_hub.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.hub";

void NiceT4Hub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Nice T4 Hub...");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", address_);
  ESP_LOGCONFIG(TAG, "  Identity: 0x%02X", identity_);
  ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Allow Write: %s", allow_write_ ? "true" : "false");
  
  // Инициализируем T4Engine с адресом из конфигурации
  // Address требует два параметра: adr и ept (endpoint)
  engine_ = std::make_unique<T4Engine>(*this, Address(address_, 0x00));
  engine_->set_bus_idle_ms(100);
  ESP_LOGI(TAG, "Nice T4 Hub initialized (P2 stage)");
}

void NiceT4Hub::loop() {
  if (engine_) {
    engine_->loop();
  }
}

void NiceT4Hub::dump_config() {
  ESP_LOGCONFIG(TAG, "Nice T4 Hub:");
  ESP_LOGCONFIG(TAG, "  Address: 0x%02X", address_);
  ESP_LOGCONFIG(TAG, "  Identity: 0x%02X", identity_);
  ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Allow Write: %s", allow_write_ ? "true" : "false");
}

void NiceT4Hub::send_break() {
  this->flush();
  delay(1);
}

void NiceT4Hub::write(const uint8_t *data, size_t n) {
  if (!allow_write_) {
    ESP_LOGW(TAG, "Write blocked by configuration");
    return;
  }
  this->write_array(data, n);
  this->flush();
}

int NiceT4Hub::read_byte() {
  uint8_t b;
  // Явно вызываем метод из базового класса UARTDevice
  if (this->uart::UARTDevice::read_byte(&b)) {
    return b;
  }
  return -1;
}

}  // namespace nice_t4
}  // namespace esphome

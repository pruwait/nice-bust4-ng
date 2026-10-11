#include "nice_t4_hub.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.hub";

void NiceT4Hub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Nice T4 Hub...");
  // Инициализируем T4Engine, передавая этот объект как Transport и адрес 0x01
  engine_ = std::make_unique<T4Engine>(*this, Address(0x01));
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
  LOG_PIN("  TX Pin: ", this->tx_pin_);
  LOG_PIN("  RX Pin: ", this->rx_pin_);
}

void NiceT4Hub::send_break() {
  // Отправка break-сигнала (низкий уровень ~1 мс)
  // Для P2 используем базовую реализацию через flush и задержку
  this->flush();
  delay(1);
}

void NiceT4Hub::write(const uint8_t *data, size_t n) {
  this->write_array(data, n);
  this->flush();
}

int NiceT4Hub::read_byte() {
  uint8_t b;
  if (this->read_byte(&b)) {
    return b;
  }
  return -1;
}

}  // namespace nice_t4
}  // namespace esphome

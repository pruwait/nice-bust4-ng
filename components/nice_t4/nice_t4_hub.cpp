#include "nice_t4_hub.h"
#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.hub";

void NiceT4Hub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Nice T4 Hub...");
  ESP_LOGCONFIG(TAG, "  Address (EPT): 0x%02X", address_);
  ESP_LOGCONFIG(TAG, "  Identity (DEVICE): 0x%02X", identity_);
  ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Allow Write: %s", allow_write_ ? "true" : "false");
  
  engine_ = std::make_unique<T4Engine>(*this, Address(0x00, address_));
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
  ESP_LOGCONFIG(TAG, "  Address (EPT): 0x%02X", address_);
  ESP_LOGCONFIG(TAG, "  Identity (DEVICE): 0x%02X", identity_);
  ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Allow Write: %s", allow_write_ ? "true" : "false");
}

void NiceT4Hub::send_break() {
  // 1. Гарантируем, что аппаратный UART полностью завершил предыдущую передачу
  this->flush();

  // 2. Получаем доступ к TX пину через публичный API ESPHome
  auto *tx_pin = this->parent_->get_tx_pin();
  if (tx_pin != nullptr) {
    // 3. Принудительно опускаем линию в LOW (break condition)
    tx_pin->digital_write(false);
    
    // 4. Держим LOW ~1.1 мс. Спецификация Nice BusT4 требует 0.5–1.0 мс.
    // 1100 мкс гарантированно будут восприняты приемником как валидный break.
    delayMicroseconds(1100);
    
    // 5. Возвращаем линию в HIGH (UART idle state)
    tx_pin->digital_write(true);
    
    // 6. Небольшая пауза для стабилизации линии перед тем, как аппаратный 
    // UART начнет отправлять байт синхронизации 0x55
    delayMicroseconds(100);
  } else {
    // Fallback на случай, если пин по какой-то причине не определен (маловероятно)
    ESP_LOGW(TAG, "TX pin not available, using simple delay for break");
    delayMicroseconds(1100);
  }
}

void NiceT4Hub::write(const uint8_t *data, size_t n) {
  if (!allow_write_) {
    ESP_LOGW(TAG, "Write blocked by configuration");
    return;
  }
  
  ESP_LOGD(TAG, ">>> UART write called, size: %d", n);
  char hex_str[96] = {0};
  for (size_t i = 0; i < n && i < 32; i++) {
    sprintf(hex_str + i * 3, "%02X ", data[i]);
  }
  ESP_LOGD(TAG, ">>> Data to TX: %s", hex_str);

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

bool NiceT4Hub::send_dep_cmd(uint8_t cmd_code) {
  if (!engine_) {
    ESP_LOGE(TAG, "Engine not initialized!");
    return false;
  }

  if (!allow_write_) {
    ESP_LOGW(TAG, "Command send blocked by allow_write=false");
    return false;
  }

  Frame f;
  f.to = Address(0x00, 0xFF);
  f.from = Address(0x00, address_);
  f.proto = static_cast<uint8_t>(Proto::DEP);
  
  f.msg[0] = identity_;
  f.msg[1] = 0x82;
  f.msg[2] = cmd_code;
  f.msg[3] = 0x64;
  f.msg_len = 4;

  RequestOptions opt;
  opt.timeout_ms = 500;
  opt.retries = 2;
  opt.expect_response = true;
  opt.multi_response = false;

  ESP_LOGD(TAG, ">>> Attempting to queue DEP CMD: 0x%02X", cmd_code);
  
  bool res = engine_->submit(f, opt, nullptr, nullptr);
  if (!res) {
    ESP_LOGE(TAG, ">>> FAILED to submit to T4Engine (queue full or encode error)");
  } else {
    ESP_LOGD(TAG, ">>> Successfully queued DEP CMD: 0x%02X", cmd_code);
  }
  
  return res;
}

}  // namespace nice_t4
}  // namespace esphome

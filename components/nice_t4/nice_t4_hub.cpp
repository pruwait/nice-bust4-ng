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
  // 1. Очищаем буфер отправки
  this->flush();
  
  // 2. Занижаем бодрейт до 9200. 
  // На скорости 9200 бит длится ~108 мкс. Байт 0x00 (10 бит: старт + 8 данных + стоп) 
  // даст низкий уровень ~1080 мкс (1.08 мс), что идеально попадает в спецификацию Nice (0.5–1 мс).
  this->parent_->set_baud_rate(9200);
  
  // 3. Отправляем байт 0x00
  uint8_t br_ch = 0x00;
  this->write_array(&br_ch, 1);
  this->flush();
  
  // 4. КРИТИЧЕСКИ ВАЖНАЯ ЗАДЕРЖКА (из вашего старого кода uart_break.ino)
  // В ядре ESP8266 функции ожидания окончания передачи недостаточно перед мгновенной 
  // сменой бодрейта. Без этой задержки последний бит break-сигнала обрезается или 
  // уходит на новой скорости, что портит форму сигнала на логическом анализаторе.
  // 90 мкс дают идеальный сигнал break на Wemos D1 Mini / ESP8266.
  delayMicroseconds(90);
  
  // 5. Возвращаем рабочий бодрейт 19200 для основной передачи данных
  this->parent_->set_baud_rate(19200);
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
  // Явно вызываем метод из базового класса UARTDevice, чтобы избежать конфликта имён
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

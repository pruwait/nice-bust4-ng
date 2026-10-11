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
  
  // Инициализируем T4Engine. ADR=0x00 (сеть по умолчанию), EPT=address_
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
  this->flush();
  delay(1); // ~1 мс break-сигнал
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
  // Явно вызываем метод из базового класса UARTDevice, чтобы избежать конфликта имён
  if (this->uart::UARTDevice::read_byte(&b)) {
    return b;
  }
  return -1;
}

bool NiceT4Hub::send_dep_cmd(uint8_t cmd_code) {
  if (!engine_) {
    ESP_LOGE(TAG, "Engine not initialized");
    return false;
  }

  if (!allow_write_) {
    ESP_LOGW(TAG, "Command send blocked by configuration");
    return false;
  }

  Frame f;
  f.to = Address(0x00, 0xFF);       // Широковещание в сети 0 (все устройства)
  f.from = Address(0x00, address_); // Адрес ESP-модуля (ADR=0x00, EPT=address_)
  f.proto = static_cast<uint8_t>(Proto::DEP);
  
  // Формируем тело сообщения DEP CMD согласно спецификации
  f.msg[0] = identity_; // DEVICE (0x01 для O-View, 0x02 для IT4WIFI и т.д.)
  f.msg[1] = 0x82;      // COMMAND = RUN (выполнение команды)
  f.msg[2] = cmd_code;  // Код команды (например, 0x03 для OPEN, 0x02 для STOP)
  f.msg[3] = 0x64;      // OFFSET (безопасное значение по спецификации, работает на DPRO924)
  f.msg_len = 4;

  RequestOptions opt;
  opt.timeout_ms = 500;
  opt.retries = 2;
  opt.expect_response = true;
  opt.multi_response = false;

  ESP_LOGD(TAG, "Sending DEP CMD: 0x%02X (device=0x%02X)", cmd_code, identity_);
  
  return engine_->submit(f, opt, nullptr, nullptr);
}

}  // namespace nice_t4
}  // namespace esphome

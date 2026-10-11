#include "nice_t4_hub.h"
#include "esphome/core/log.h"

// === Хак для прямого доступа к аппаратному UART на ESP8266 ===
#ifdef USE_ESP8266
// Включаем заголовок ДО определения хак-класса, чтобы компилятор знал ESP8266UartComponent
#include "esphome/components/uart/uart_component_esp8266.h"

namespace esphome {
namespace uart {
// Наследуемся, чтобы получить публичный доступ к protected-полю uart_
class ESP8266UartComponentHack : public ESP8266UartComponent {
 public:
  uart_t* get_hw_uart() { return this->uart_; }
};
}  // namespace uart
}  // namespace esphome
#endif
// ============================================================

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
  // 1. Очищаем буфер высокоуровневого UART ESPHome
  this->flush();

#ifdef USE_ESP8266
  // 2. Получаем прямой доступ к аппаратному UART ESP8266 через хак
  auto *uart_comp = static_cast<esphome::uart::ESP8266UartComponentHack*>(this->parent_);
  uart_t *hw_uart = uart_comp->get_hw_uart();
  
  if (hw_uart) {
    uart_flush(hw_uart);                                               // очищаем аппаратный uart
    uart_set_baudrate(hw_uart, 9200);                                  // занижаем бодрэйт
    
    uint8_t br_ch = 0x00;
    uart_write(hw_uart, (const char *)&br_ch, 1);                      // отправляем ноль на низкой скорости
    uart_wait_tx_empty(hw_uart);                                       // ждём, пока отправка завершится
    
    // 3. КРИТИЧЕСКИ ВАЖНАЯ ЗАДЕРЖКА (из вашего рабочего кода)
    // В библиотеке uart.h (esp8266 core 3.0.2) ожидания недостаточно.
    // Без этой задержки скорость переключится раньше отправки последнего бита.
    delayMicroseconds(90);
    
    uart_set_baudrate(hw_uart, 19200);                                 // возвращаем рабочий бодрэйт
    return;
  }
#endif

  // Fallback на случай, если хак не сработал (например, компиляция под другую платформу)
  ESP_LOGW(TAG, "Hardware UART access failed, using fallback break");
  uint8_t br_ch = 0x00;
  this->write_array(&br_ch, 1);
  this->flush();
  delayMicroseconds(100);
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

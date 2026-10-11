#include "nice_t4_hub.h"
#include "esphome/core/log.h"

#ifdef USE_ESP8266
#include <uart.h> // Аппаратный UART API ядра ESP8266
#endif

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.hub";

// Настройки из оригинального рабочего кода pruwait/Nice_BusT4
static const int UART_NO = UART0;
static const int TX_PIN = 1; // GPIO1 (стандартный TX ESP8266)
static const int RX_PIN = 3; // GPIO3 (стандартный RX ESP8266)
static const uint32_t BAUD_BREAK = 9200;
static const uint32_t BAUD_WORK = 19200;

void NiceT4Hub::setup() {
  ESP_LOGCONFIG(TAG, "Setting up Nice T4 Hub...");
  ESP_LOGCONFIG(TAG, "  Address (EPT): 0x%02X", address_);
  ESP_LOGCONFIG(TAG, "  Identity (DEVICE): 0x%02X", identity_);
  ESP_LOGCONFIG(TAG, "  Model: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Allow Write: %s", allow_write_ ? "true" : "false");
  
#ifdef USE_ESP8266
  ESP_LOGW(TAG, "Используется прямой доступ к аппаратному UART0 (GPIO1=TX, GPIO3=RX).");
  // Инициализируем аппаратный UART напрямую, как в pruwait/Nice_BusT4
  hw_uart_ = uart_init(UART_NO, BAUD_WORK, SERIAL_8N1, SERIAL_FULL, TX_PIN, 256, false);
  if (!hw_uart_) {
    ESP_LOGE(TAG, "Failed to initialize hardware UART");
    this->mark_failed();
    return;
  }
#endif

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
#ifdef USE_ESP8266
  if (hw_uart_) {
    uart_flush(hw_uart);                                               // очищаем uart
    uart_set_baudrate(hw_uart, BAUD_BREAK);                            // занижаем бодрэйт до 9200
    char br_ch = 0x00;
    uart_write(hw_uart, &br_ch, 1);                                    // отправляем ноль на низкой скорости
    uart_wait_tx_empty(hw_uart);                                       // ждём опустошения FIFO
    delayMicroseconds(90);                                             // КРИТИЧЕСКИ ВАЖНАЯ ЗАДЕРЖКА из кода pruwait
    uart_set_baudrate(hw_uart, BAUD_WORK);                             // возвращаем рабочий бодрэйт 19200
    return;
  }
#endif
  ESP_LOGW(TAG, "Hardware UART not available, break may be incorrect");
}

void NiceT4Hub::write(const uint8_t *data, size_t n) {
  if (!allow_write_) {
    ESP_LOGW(TAG, "Write blocked by configuration");
    return;
  }
#ifdef USE_ESP8266
  if (hw_uart_) {
    uart_write(hw_uart, (const char *)data, n);
    uart_wait_tx_empty(hw_uart);
    return;
  }
#endif
}

int NiceT4Hub::read_byte() {
#ifdef USE_ESP8266
  if (hw_uart_ && uart_rx_available(hw_uart_) > 0) {
    return (uint8_t)uart_read_char(hw_uart_);
  }
#endif
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

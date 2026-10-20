#pragma once
// Адаптер Transport поверх ESPHome uart. Отделён от хаба: нет множественного наследования и коллизий имён.
#include "esphome/components/uart/uart.h"
#include "esphome/core/hal.h"
#include "esphome/core/log.h"

#include "t4_engine.h"

namespace esphome {
namespace nice_t4 {

enum class BreakMode : uint8_t {
  BYTE = 0,  // простой байт 00 на рабочей скорости (проверено на RB500HS)
  BAUD = 1,  // байт 00 на 9200 бод, около 0,98 мс низкого уровня (как в других реализациях)
};

class UartTransport : public Transport {
 public:
  explicit UartTransport(uart::UARTDevice *dev) : dev_(dev) {}
  void set_parent(uart::UARTComponent *p) { parent_ = p; }
  void set_break_mode(BreakMode m) { mode_ = m; }
  void set_baud_rate(uint32_t b) { baud_ = b; }

  uint32_t now_ms() override { return millis(); }

  int read_byte() override {
    uint8_t b;
    if (dev_->available() > 0 && dev_->read_byte(&b)) return b;
    return -1;
  }

  void write(const uint8_t *d, size_t n) override {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
    char hex[kMaxFrame * 3 + 1];
    format_hex(d, n, hex, sizeof(hex));
    ESP_LOGV(TAG, "TX %s", hex);
#endif
    dev_->write_array(d, n);
    dev_->flush();
  }

  void send_break() override {
    dev_->flush();
#if defined(USE_ESP8266) || defined(USE_ESP32)  // load_settings() есть только у этих платформ
    if (mode_ == BreakMode::BAUD && parent_ != nullptr) {
      parent_->set_baud_rate(kBreakBaud);
      parent_->load_settings(false);
      dev_->write_byte(0x00);
      dev_->flush();
      parent_->set_baud_rate(baud_);
      parent_->load_settings(false);
    } else
#endif
    {
      dev_->write_byte(0x00);
      dev_->flush();
    }
    delayMicroseconds(kGapUs);  // короткая пауза между break и кадром
  }

 private:
  static constexpr const char *TAG = "nice_t4.uart";
  static constexpr uint32_t kBreakBaud = 9200;
  static constexpr uint32_t kGapUs = 100;
  uart::UARTDevice *dev_;
  uart::UARTComponent *parent_ = nullptr;
  BreakMode mode_ = BreakMode::BYTE;
  uint32_t baud_ = 19200;
};

}  // namespace nice_t4
}  // namespace esphome

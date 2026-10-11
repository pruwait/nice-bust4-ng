# Заметки к P0: адаптер UART (не проверено на железе)

Движок (`T4Engine`) не знает про ESPHome и работает через интерфейс `Transport`. Для P0 нужен адаптер:

```cpp
class UartTransport : public Transport {
 public:
  explicit UartTransport(uart::UARTComponent *u) : uart_(u) {}
  uint32_t now_ms() override { return millis(); }
  int read_byte() override {
    uint8_t b;
    return (uart_->available() > 0 && uart_->read_byte(&b)) ? b : -1;
  }
  void write(const uint8_t *d, size_t n) override { uart_->write_array(d, n); }
  void send_break() override {                 // 0x00 на 9200 бод даёт около 0,98 мс низкого уровня
    uart_->flush();
    uart_->set_baud_rate(9200);
    uart_->load_settings(false);
    uart_->write_byte(0x00);
    uart_->flush();
    uart_->set_baud_rate(19200);
    uart_->load_settings(false);
  }
 private:
  uart::UARTComponent *uart_;
};
```

Что проверить на железе в первую очередь:

1. Существуют ли `set_baud_rate()` и `load_settings()` в нужной версии ESPHome и работает ли смена скорости на лету на ESP32 и ESP8266.
2. Реальная длительность break на осциллографе (допустимо 0,5–1 мс по известным реализациям).
3. ESP8266: аппаратный UART0 с логгером на UART1 и вариант с программным UART.
4. Эхо: приходят ли собственные кадры обратно на RX (зависит от схемы подключения).
5. Время между отправкой и ответом привода на общей шине (ожидается до 55 мс).

#pragma once

#include "esphome/core/component.h"
#include "esphome/components/uart/uart.h"
#include "t4_engine.h"

namespace esphome {
namespace nice_t4 {

class NiceT4Component : public Component, public uart::UARTDevice {
 public:
  void setup() override {
    // Здесь инициализируется T4Engine. 
    // Вам потребуется адаптировать ваш интерфейс Transport, чтобы он использовал методы UARTDevice
    // engine_ = make_unique<T4Engine>(*this, Address(0x01));
    ESP_LOGI("nice_t4", "Nice T4 Component initialized");
  }

  void loop() override {
    // Регулярный вызов для обработки приёма/передачи без блокировок
    // if (engine_) engine_->loop();
  }

  // Метод для регистрации будущих платформ (cover, sensor и т.д.)
  // void register_cover(T4Cover *cover) { covers_.push_back(cover); }

 private:
  // std::unique_ptr<T4Engine> engine_;
};

}  // namespace nice_t4
}  // namespace esphome

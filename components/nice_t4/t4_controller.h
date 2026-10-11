#pragma once

#include "t4_engine.h"
#include "t4_capabilities.h"

namespace esphome {
namespace nice_t4 {

class Controller {
 public:
  Controller(T4Engine *engine) : engine_(engine) {}

  void start_initial_poll() {
    // P2: Запрос WHO, типа привода, прошивки, Capabilities
    // TODO: реализовать очередь запросов при включении через engine_->submit(...)
  }

  const Capabilities& get_capabilities() const { return caps_; }

 protected:
  T4Engine *engine_;
  Capabilities caps_;
};

}  // namespace nice_t4
}  // namespace esphome

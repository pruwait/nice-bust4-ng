#include "nice_t4_cover.h"

#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4.cover";

void NiceT4Cover::setup() { hub_->add_listener(this); }  // состояние не публикуется, пока привод не сообщил его

void NiceT4Cover::dump_config() { LOG_COVER("", "Nice T4 Cover", this); }

cover::CoverTraits NiceT4Cover::get_traits() {
  cover::CoverTraits traits;
  traits.set_supports_stop(true);
  traits.set_supports_toggle(true);
  traits.set_supports_position(false);
  traits.set_is_assumed_state(true);  // привод может стоять посередине, поэтому обе кнопки доступны всегда
  return traits;
}

void NiceT4Cover::control(const cover::CoverCall &call) {
  bool ok = true;
  if (call.get_stop()) {
    ok = hub_->send_dep_cmd(cmd::STOP);
  } else if (call.get_toggle().has_value()) {
    ok = hub_->send_dep_cmd(cmd::SBS);
  } else if (call.get_position().has_value()) {
    const float p = *call.get_position();
    if (p >= cover::COVER_OPEN) {
      ok = hub_->send_dep_cmd(cmd::OPEN);
    } else if (p <= cover::COVER_CLOSED) {
      ok = hub_->send_dep_cmd(cmd::CLOSE);
    } else {
      ESP_LOGW(TAG, "Промежуточное положение %.2f не поддерживается, используйте кнопки частичного открывания", p);
      return;
    }
  }
  if (!ok) ESP_LOGW(TAG, "Команда не отправлена");
  // Состояние не публикуется здесь: его сообщит привод.
}

void NiceT4Cover::on_gate_state(GateState s) {
  float pos = this->position;
  cover::CoverOperation op = this->current_operation;
  switch (s) {
    case GateState::OPEN:
      pos = cover::COVER_OPEN;
      op = cover::COVER_OPERATION_IDLE;
      break;
    case GateState::CLOSED:
      pos = cover::COVER_CLOSED;
      op = cover::COVER_OPERATION_IDLE;
      break;
    case GateState::OPENING:
      op = cover::COVER_OPERATION_OPENING;
      if (!published_) pos = 0.5f;
      break;
    case GateState::CLOSING:
      op = cover::COVER_OPERATION_CLOSING;
      if (!published_) pos = 0.5f;
      break;
    case GateState::STOPPED:
    case GateState::PARTIAL:
      pos = 0.5f;  // остановлено или открыто частично
      op = cover::COVER_OPERATION_IDLE;
      break;
    default:
      return;  // TIMEOUT, SEARCH_DONE: на состояние ворот не влияют
  }
  if (published_ && pos == this->position && op == this->current_operation) return;
  this->position = pos;
  this->current_operation = op;
  published_ = true;
  this->publish_state();
}

}  // namespace nice_t4
}  // namespace esphome

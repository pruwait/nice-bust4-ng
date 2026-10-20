#include "nice_t4_hub.h"

#include "esphome/core/log.h"

namespace esphome {
namespace nice_t4 {

static const char *const TAG = "nice_t4";
static const uint32_t kFirstPollDelayMs = 2000;

void NiceT4Hub::setup() {
  transport_.set_parent(this->parent_);
  transport_.set_break_mode(break_mode_);
  engine_.set_self(self_);
  engine_.set_event_callback(&NiceT4Hub::on_event_cb, this);
  next_poll_ms_ = millis() + kFirstPollDelayMs;
}

void NiceT4Hub::loop() {
  engine_.loop();
  if (status_poll_ms_ > 0 && !poll_pending_ && static_cast<int32_t>(millis() - next_poll_ms_) >= 0 &&
      engine_.queued() == 0) {
    poll_status();
  }
}

void NiceT4Hub::dump_config() {
  ESP_LOGCONFIG(TAG, "Nice T4:");
  ESP_LOGCONFIG(TAG, "  Адрес модуля: ADR %02X, EPT %02X", self_.adr, self_.ept);
  ESP_LOGCONFIG(TAG, "  Адрес привода: ADR %02X, EPT %02X", target_.adr, target_.ept);
  ESP_LOGCONFIG(TAG, "  Идентичность (DEVICE): %02X", identity_);
  ESP_LOGCONFIG(TAG, "  Модель: %s", model_.c_str());
  ESP_LOGCONFIG(TAG, "  Break: %s", break_mode_ == BreakMode::BAUD ? "baud (00 на 9200)" : "byte (00 на рабочей скорости)");
  ESP_LOGCONFIG(TAG, "  Команды разрешены: %s, запись параметров разрешена: %s", YESNO(allow_commands_),
                YESNO(allow_write_));
  ESP_LOGCONFIG(TAG, "  Опрос состояния: %u мс", static_cast<unsigned>(status_poll_ms_));
  const T4Rx::Stats &rx = engine_.rx_stats();
  ESP_LOGCONFIG(TAG, "  Принято кадров: %u, ошибок CRC: %u, потерь синхронизации: %u", static_cast<unsigned>(rx.frames),
                static_cast<unsigned>(rx.bad_crc), static_cast<unsigned>(rx.resyncs));
}

bool NiceT4Hub::add_listener(HubListener *l) {
  if (l == nullptr || listener_count_ >= kMaxListeners) return false;
  listeners_[listener_count_++] = l;
  return true;
}

bool NiceT4Hub::send_dep_cmd(uint8_t code) {
  if (!allow_commands_) {
    ESP_LOGW(TAG, "Команда %02X не отправлена: allow_commands выключен", code);
    return false;
  }
  const Frame f = make_cmd(target_, self_, code, identity_, cmd::DEFAULT_OFFSET);
  RequestOptions o;
  o.expect_response = false;  // ответ RSP придёт событием; движок и сам не повторяет DEP
  o.retries = 0;
  if (!engine_.submit(f, o, nullptr, nullptr)) {
    ESP_LOGW(TAG, "Очередь запросов заполнена, команда %02X отброшена", code);
    return false;
  }
  return true;
}

bool NiceT4Hub::write_register(uint8_t device, uint8_t reg, const uint8_t *data, uint8_t len) {
  if (!allow_write_) {
    ESP_LOGW(TAG, "Запись регистра %02X/%02X запрещена: allow_write выключен", device, reg);
    return false;
  }
  const Frame f = make_dmp(target_, self_, device, reg, op::SET, data, len);
  if (f.msg_len == 0 || !engine_.submit(f, RequestOptions(), nullptr, nullptr)) {
    ESP_LOGW(TAG, "Запись регистра %02X/%02X не поставлена в очередь", device, reg);
    return false;
  }
  return true;
}

void NiceT4Hub::poll_status() {
  const Frame f = make_dmp(target_, self_, static_cast<uint8_t>(Device::CONTROLLER), 0x01, op::GET);
  RequestOptions o;
  o.timeout_ms = 300;
  o.retries = 1;
  poll_pending_ = engine_.submit(f, o, &NiceT4Hub::on_status_cb, this);
  next_poll_ms_ = millis() + status_poll_ms_;
}

void NiceT4Hub::on_event_cb(void *ctx, const Frame &f) { static_cast<NiceT4Hub *>(ctx)->handle_event(f); }
void NiceT4Hub::on_status_cb(void *ctx, Result r, const Frame *f) { static_cast<NiceT4Hub *>(ctx)->handle_status(r, f); }

void NiceT4Hub::handle_event(const Frame &f) {
#if ESPHOME_LOG_LEVEL >= ESPHOME_LOG_LEVEL_VERBOSE
  uint8_t buf[kMaxFrame];
  const size_t n = encode(f, buf, sizeof(buf));
  char hex[kMaxFrame * 3 + 1];
  format_hex(buf, n, hex, sizeof(hex));
  ESP_LOGV(TAG, "RX %s", hex);
#endif
  for (uint8_t i = 0; i < listener_count_; i++) listeners_[i]->on_event(f);
  GateState s;
  if (gate_state_from_event(f, s)) set_state(s);
}

void NiceT4Hub::handle_status(Result r, const Frame *f) {
  poll_pending_ = false;
  if (r == Result::RESPONSE && f != nullptr) {
    GateState s;
    if (gate_state_from_status(*f, s)) set_state(s);
  } else if (r == Result::TIMEOUT) {
    ESP_LOGW(TAG, "Привод не ответил на запрос состояния (адрес %02X %02X)", target_.adr, target_.ept);
  }
}

void NiceT4Hub::set_state(GateState s) {
  if (s == GateState::UNKNOWN || s == gate_state_) return;
  gate_state_ = s;
  ESP_LOGD(TAG, "Состояние ворот: %s", gate_state_name(s));
  for (uint8_t i = 0; i < listener_count_; i++) listeners_[i]->on_gate_state(s);
}

}  // namespace nice_t4
}  // namespace esphome

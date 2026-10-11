#pragma once
// Очередь запросов поверх абстрактного транспорта: пауза на шине, break, повторы, сопоставление ответов.
// Не зависит от ESPHome: UART подключается через интерфейс Transport.
#include "t4_frame.h"
#include "t4_rx.h"

namespace esphome {
namespace nice_t4 {

class Transport {
 public:
  virtual ~Transport() {}
  virtual uint32_t now_ms() = 0;
  virtual void send_break() = 0;                          // низкий уровень около 1 мс
  virtual void write(const uint8_t *data, size_t n) = 0;
  virtual int read_byte() = 0;                            // -1, если байтов нет
};

enum class Result : uint8_t {
  RESPONSE,    // получен ответ (для multi_response вызывается на каждый ответ)
  SENT,        // кадр отправлен, ответ не ожидался
  TIMEOUT,     // ответа нет после всех повторов
  WINDOW_END,  // окно сбора ответов закрыто (multi_response)
  DROPPED,     // кадр не удалось закодировать
};

typedef void (*ResponseCb)(void *ctx, Result r, const Frame *resp);
typedef void (*EventCb)(void *ctx, const Frame &f);

struct RequestOptions {
  uint32_t timeout_ms;
  uint8_t retries;
  bool expect_response;
  bool multi_response;  // принимать все ответы до конца таймаута (широковещательные запросы)
  bool any_run;         // принимать любой RUN от нужного регистра
  RequestOptions() : timeout_ms(300), retries(2), expect_response(true), multi_response(false), any_run(false) {}
};

class T4Engine {
 public:
  static const uint8_t kQueueSize = 8;
  struct Stats {
    uint32_t tx_frames = 0, tx_retries = 0, tx_timeouts = 0, echoes = 0, events = 0, responses = 0;
  };

  T4Engine(Transport &tr, Address self);

  void set_bus_idle_ms(uint32_t ms) { bus_idle_ms_ = ms; }
  void set_event_callback(EventCb cb, void *ctx) {
    ev_cb_ = cb;
    ev_ctx_ = ctx;
  }
  bool submit(const Frame &f, const RequestOptions &o, ResponseCb cb, void *ctx);
  void loop();  // вызывать из loop() компонента, не блокирует (кроме send_break)

  uint8_t queued() const { return count_; }
  const Stats &stats() const { return stats_; }
  const T4Rx::Stats &rx_stats() const { return rx_.stats(); }
  Address self() const { return self_; }

 private:
  struct Slot {
    Frame frame;
    RequestOptions opt;
    ResponseCb cb = nullptr;
    void *ctx = nullptr;
  };
  enum class State : uint8_t { IDLE, WAITING };

  void on_frame(const Frame &f);
  void start_tx(uint32_t now);
  void finish(Result r, const Frame *resp);
  static uint32_t elapsed(uint32_t now, uint32_t since) { return static_cast<uint32_t>(now - since); }

  Transport &tr_;
  Address self_;
  T4Rx rx_;
  Slot slots_[kQueueSize];
  uint8_t head_ = 0, count_ = 0;
  State state_ = State::IDLE;
  uint32_t tx_ms_ = 0, last_activity_ms_ = 0, bus_idle_ms_ = 100;
  uint8_t attempt_ = 0;
  bool got_response_ = false;
  EventCb ev_cb_ = nullptr;
  void *ev_ctx_ = nullptr;
  Stats stats_;
};

}  // namespace nice_t4
}  // namespace esphome

#include "t4_engine.h"

namespace esphome {
namespace nice_t4 {

T4Engine::T4Engine(Transport &tr, Address self) : tr_(tr), self_(self) {
  last_activity_ms_ = static_cast<uint32_t>(tr.now_ms() - bus_idle_ms_);  // первый кадр можно слать сразу
}

bool T4Engine::submit(const Frame &f, const RequestOptions &o, ResponseCb cb, void *ctx) {
  if (count_ >= kQueueSize) return false;
  Slot &s = slots_[(head_ + count_) % kQueueSize];
  s.frame = f;
  s.opt = o;
  if (f.is_dep()) {
    s.opt.expect_response = false;
    s.opt.multi_response = false;
    s.opt.retries = 0;
  }
  s.cb = cb;
  s.ctx = ctx;
  count_++;
  return true;
}

void T4Engine::finish(Result r, const Frame *resp) {
  const ResponseCb cb = slots_[head_].cb;
  void *ctx = slots_[head_].ctx;
  head_ = static_cast<uint8_t>((head_ + 1) % kQueueSize);
  count_--;
  state_ = State::IDLE;
  attempt_ = 0;
  got_response_ = false;
  if (cb) cb(ctx, r, resp);  // колбэк может вызвать submit()
}

void T4Engine::start_tx(uint32_t now) {
  Slot &s = slots_[head_];
  uint8_t buf[kMaxFrame];
  const size_t n = encode(s.frame, buf, sizeof(buf));
  if (n == 0) {
    finish(Result::DROPPED, nullptr);
    return;
  }
  tr_.send_break();
  tr_.write(buf, n);
  now = tr_.now_ms();
  tx_ms_ = now;
  last_activity_ms_ = now;
  stats_.tx_frames++;
  if (!s.opt.expect_response) {
    finish(Result::SENT, nullptr);
    return;
  }
  state_ = State::WAITING;
}

void T4Engine::on_frame(const Frame &f) {
  if (f.from == self_) {  // эхо собственной передачи
    stats_.echoes++;
    return;
  }
  if (state_ == State::WAITING) {
    Slot &s = slots_[head_];
    if (response_matches(s.frame, f, s.opt.any_run)) {
      stats_.responses++;
      if (s.opt.multi_response) {
        got_response_ = true;
        if (s.cb) s.cb(s.ctx, Result::RESPONSE, &f);
      } else {
        finish(Result::RESPONSE, &f);
      }
      return;
    }
  }
  stats_.events++;
  if (ev_cb_) ev_cb_(ev_ctx_, f);
}

void T4Engine::loop() {
  uint32_t now = tr_.now_ms();
  int c;
  while ((c = tr_.read_byte()) >= 0) {
    last_activity_ms_ = now;
    if (rx_.feed(static_cast<uint8_t>(c), now)) {
      on_frame(rx_.frame());
      while (rx_.next()) on_frame(rx_.frame());
    }
  }
  if (state_ == State::WAITING && elapsed(now, tx_ms_) >= slots_[head_].opt.timeout_ms) {
    Slot &s = slots_[head_];
    if (s.opt.multi_response && got_response_) {
      finish(Result::WINDOW_END, nullptr);
    } else if (attempt_ < s.opt.retries) {
      attempt_++;
      stats_.tx_retries++;
      state_ = State::IDLE;  // повтор после паузы на шине
    } else {
      stats_.tx_timeouts++;
      finish(Result::TIMEOUT, nullptr);
    }
  }
  if (state_ == State::IDLE && count_ > 0 && elapsed(now, last_activity_ms_) >= bus_idle_ms_) start_tx(now);
}

}  // namespace nice_t4
}  // namespace esphome

#include "t4_rx.h"

namespace esphome {
namespace nice_t4 {

bool T4Rx::feed(uint8_t b, uint32_t now_ms) {
  if (n_ > 0 && static_cast<uint32_t>(now_ms - last_ms_) > byte_timeout_ms_) {
    stats_.timeouts++;
    n_ = 0;
  }
  last_ms_ = now_ms;
  if (n_ == 0) {
    if (b != kSync) return false;  // break (00) и шум
  }
  if (n_ >= sizeof(buf_)) drop_to_next_sync();
  buf_[n_++] = b;
  return process();
}

bool T4Rx::next() { return process(); }

bool T4Rx::process() {
  for (;;) {
    if (n_ < 2) return false;
    const uint8_t size = buf_[1];
    if (size < kMinBody || size > kMaxBody) {
      stats_.bad_frame++;
      drop_to_next_sync();
      continue;
    }
    const size_t total = static_cast<size_t>(size) + 3;
    if (n_ < total) return false;
    const ParseError e = decode(buf_, total, frame_);
    if (e == ParseError::OK) {
      stats_.frames++;
      memmove(buf_, buf_ + total, n_ - total);
      n_ -= total;
      return true;
    }
    if (e == ParseError::BAD_CRC1 || e == ParseError::BAD_CRC2) stats_.bad_crc++;
    else stats_.bad_frame++;
    drop_to_next_sync();
  }
}

void T4Rx::drop_to_next_sync() {
  size_t i = 1;
  while (i < n_ && buf_[i] != kSync) i++;
  memmove(buf_, buf_ + i, n_ - i);
  n_ -= i;
  stats_.resyncs++;
}

}  // namespace nice_t4
}  // namespace esphome

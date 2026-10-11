#pragma once
// Постраничное чтение регистра: ответы с RUN 18 продолжаются запросом с новым смещением, RUN 19 завершает.
#include "t4_frame.h"

namespace esphome {
namespace nice_t4 {

class ChunkReader {
 public:
  enum class State : uint8_t { IDLE, CONTINUE, DONE, ERROR };
  enum class Error : uint8_t { NONE, DEVICE_STATUS, BAD_RESPONSE, OVERFLOW, NO_PROGRESS };

  ChunkReader(uint8_t *buf, size_t cap) : buf_(buf), cap_(cap) {}

  // req_data: данные запроса (например, номер меню для списка INF-команд), до 8 байт.
  bool begin(Address to, Address from, uint8_t device, uint8_t reg, const uint8_t *req_data = nullptr,
             uint8_t req_len = 0);
  Frame next_request() const;
  State feed(const Frame &resp);

  State state() const { return state_; }
  Error error() const { return error_; }
  uint8_t device_status() const { return device_status_; }
  size_t size() const { return size_; }
  const uint8_t *data() const { return buf_; }

 private:
  uint8_t *buf_;
  size_t cap_;
  size_t size_ = 0;
  Address to_, from_;
  uint8_t device_ = 0, reg_ = 0, offset_ = 0, req_len_ = 0, device_status_ = 0;
  uint8_t req_data_[8];
  State state_ = State::IDLE;
  Error error_ = Error::NONE;
};

}  // namespace nice_t4
}  // namespace esphome

#include "t4_chunk.h"

namespace esphome {
namespace nice_t4 {

bool ChunkReader::begin(Address to, Address from, uint8_t device, uint8_t reg, const uint8_t *req_data,
                        uint8_t req_len) {
  if (req_len > sizeof(req_data_)) return false;
  to_ = to;
  from_ = from;
  device_ = device;
  reg_ = reg;
  req_len_ = req_len;
  if (req_len) memcpy(req_data_, req_data, req_len);
  offset_ = 0;
  size_ = 0;
  error_ = Error::NONE;
  device_status_ = 0;
  state_ = State::CONTINUE;
  return true;
}

Frame ChunkReader::next_request() const { return make_dmp(to_, from_, device_, reg_, op::GET, req_data_, req_len_, offset_); }

ChunkReader::State ChunkReader::feed(const Frame &resp) {
  if (state_ != State::CONTINUE) return state_;
  DmpResponse r;
  if (!parse_dmp_response(resp, r) || r.device != device_ || r.reg != reg_) {
    error_ = Error::BAD_RESPONSE;
    return state_ = State::ERROR;
  }
  if (r.status != status::OK) {
    error_ = Error::DEVICE_STATUS;
    device_status_ = r.status;
    return state_ = State::ERROR;
  }
  if (size_ + r.data_len > cap_) {
    error_ = Error::OVERFLOW;
    return state_ = State::ERROR;
  }
  memcpy(buf_ + size_, r.data, r.data_len);
  size_ += r.data_len;
  if (r.fin()) return state_ = State::DONE;
  if (r.next <= offset_) {  // защита от зацикливания
    error_ = Error::NO_PROGRESS;
    return state_ = State::ERROR;
  }
  offset_ = r.next;
  return state_;
}

}  // namespace nice_t4
}  // namespace esphome

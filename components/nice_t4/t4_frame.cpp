#include "t4_frame.h"

namespace esphome {
namespace nice_t4 {

uint8_t xor_sum(const uint8_t *data, size_t n) {
  uint8_t r = 0;
  for (size_t i = 0; i < n; i++) r ^= data[i];
  return r;
}

size_t encode(const Frame &f, uint8_t *out, size_t cap) {
  if (f.msg_len < 2 || f.msg_len > kMaxMsg) return 0;
  const size_t body = kHeaderLen + f.msg_len + 1;
  const size_t total = body + 3;
  if (cap < total) return 0;
  out[0] = kSync;
  out[1] = static_cast<uint8_t>(body);
  out[2] = f.to.adr;
  out[3] = f.to.ept;
  out[4] = f.from.adr;
  out[5] = f.from.ept;
  out[6] = f.proto;
  out[7] = static_cast<uint8_t>(f.msg_len + 1);
  out[8] = xor_sum(out + 2, 6);
  memcpy(out + 9, f.msg, f.msg_len);
  out[9 + f.msg_len] = xor_sum(f.msg, f.msg_len);
  out[10 + f.msg_len] = static_cast<uint8_t>(body);
  return total;
}

ParseError decode(const uint8_t *buf, size_t len, Frame &f) {
  if (len < static_cast<size_t>(kMinBody) + 3) return ParseError::TOO_SHORT;
  if (buf[0] != kSync) return ParseError::BAD_SYNC;
  const uint8_t size = buf[1];
  if (size < kMinBody || size > kMaxBody) return ParseError::BAD_LENGTH;
  if (len < static_cast<size_t>(size) + 3) return ParseError::TOO_SHORT;
  if (len != static_cast<size_t>(size) + 3) return ParseError::BAD_LENGTH;
  if (buf[len - 1] != size) return ParseError::BAD_TAIL;
  if (xor_sum(buf + 2, 6) != buf[8]) return ParseError::BAD_CRC1;
  if (buf[7] != size - kHeaderLen) return ParseError::BAD_MSG_SIZE;
  const uint8_t msg_len = static_cast<uint8_t>(size - kHeaderLen - 1);
  if (xor_sum(buf + 9, msg_len) != buf[9 + msg_len]) return ParseError::BAD_CRC2;
  f.to = Address(buf[2], buf[3]);
  f.from = Address(buf[4], buf[5]);
  f.proto = buf[6];
  f.msg_len = msg_len;
  memset(f.msg, 0, sizeof(f.msg));
  memcpy(f.msg, buf + 9, msg_len);
  return ParseError::OK;
}

bool operator==(const Frame &a, const Frame &b) {
  return a.to == b.to && a.from == b.from && a.proto == b.proto && a.msg_len == b.msg_len &&
         memcmp(a.msg, b.msg, a.msg_len) == 0;
}

Frame make_cmd(Address to, Address from, uint8_t code, uint8_t identity, uint8_t offset) {
  Frame f;
  f.to = to;
  f.from = from;
  f.proto = static_cast<uint8_t>(Proto::DEP);
  f.msg_len = 4;
  f.msg[0] = identity;
  f.msg[1] = cmd::RUN_COMMAND;
  f.msg[2] = code;
  f.msg[3] = offset;
  return f;
}

Frame make_dmp(Address to, Address from, uint8_t device, uint8_t reg, uint8_t run_op, const uint8_t *data, uint8_t len,
               uint8_t offset) {
  Frame f;
  f.to = to;
  f.from = from;
  f.proto = static_cast<uint8_t>(Proto::DMP);
  if (len > kMaxMsg - 5) return f;  // msg_len остаётся 0: encode() вернёт 0
  f.msg_len = static_cast<uint8_t>(5 + len);
  f.msg[0] = device;
  f.msg[1] = reg;
  f.msg[2] = run_op;
  f.msg[3] = offset;
  f.msg[4] = len;
  if (len) memcpy(f.msg + 5, data, len);
  return f;
}

bool parse_dmp_request(const Frame &f, DmpRequest &o) {
  if (!f.is_dmp() || f.msg_len < 5 || !(f.msg[2] & run::REQ)) return false;
  o.device = f.msg[0];
  o.reg = f.msg[1];
  o.op = f.msg[2];
  o.offset = f.msg[3];
  o.len = f.msg[4];
  o.data = f.msg + 5;
  return o.len <= f.msg_len - 5;
}

bool parse_dmp_response(const Frame &f, DmpResponse &o) {
  if (!f.is_dmp() || f.msg_len < 5 || (f.msg[2] & run::REQ)) return false;
  o.device = f.msg[0];
  o.reg = f.msg[1];
  o.run = f.msg[2];
  o.next = f.msg[3];
  o.status = f.msg[4];
  o.data_len = static_cast<uint8_t>(f.msg_len - 5);
  o.data = f.msg + 5;
  return true;
}

DepKind classify_dep(const Frame &f) {
  if (!f.is_dep()) return DepKind::UNKNOWN;
  if (f.msg_len == 4 && f.msg[1] == cmd::RUN_COMMAND) return DepKind::CMD;
  if (f.msg_len == 6) {
    // RSP: COMMAND 02 и маркер 19 в первом байте данных. Признак «RUN >= 80» не годится:
    // у событий STA (COMMAND 40) RUN тоже 83 и 84, а в логах встречается RSP с RUN 04.
    return (f.msg[1] == 0x02 && f.msg[3] == 0x19) ? DepKind::RSP : DepKind::EVT;
  }
  return DepKind::UNKNOWN;
}

bool parse_dep_cmd(const Frame &f, DepCmdView &o) {
  if (classify_dep(f) != DepKind::CMD) return false;
  o.device = f.msg[0];
  o.cmd = f.msg[2];
  o.offset = f.msg[3];
  return true;
}

bool parse_dep_status(const Frame &f, DepStatusView &o) {
  const DepKind k = classify_dep(f);
  if (k != DepKind::RSP && k != DepKind::EVT) return false;
  o.device = f.msg[0];
  o.command = f.msg[1];
  o.run = f.msg[2];
  o.d1 = f.msg[3];
  o.d2 = f.msg[4];
  o.status = f.msg[5];
  return true;
}

bool response_matches(const Frame &req, const Frame &resp, bool any_run) {
  DmpRequest rq;
  DmpResponse rs;
  if (!parse_dmp_request(req, rq) || !parse_dmp_response(resp, rs)) return false;
  if (rs.device != rq.device || rs.reg != rq.reg) return false;
  if (!(resp.to == req.from || resp.to.broadcast())) return false;
  if (!req.to.broadcast() && resp.from != req.to) return false;
  if (any_run) return true;
  const uint8_t expected = rq.op & static_cast<uint8_t>(~run::REQ);
  return rs.run == expected || rs.run == (expected & static_cast<uint8_t>(~run::FIN));
}

}  // namespace nice_t4
}  // namespace esphome

#pragma once
// Кадр T4: сборка, разбор, контрольные суммы. Без динамической памяти, без зависимостей от ESPHome.
#include <string.h>

#include "t4_types.h"

namespace esphome {
namespace nice_t4 {

struct Address {
  uint8_t adr;  // ADR (сеть, «серия»)
  uint8_t ept;  // EPT (устройство)
  Address() : adr(0), ept(0) {}
  Address(uint8_t a, uint8_t e) : adr(a), ept(e) {}
  bool broadcast() const { return adr == 0xFF || ept == 0xFF; }
  bool operator==(const Address &o) const { return adr == o.adr && ept == o.ept; }
  bool operator!=(const Address &o) const { return !(*this == o); }
};

struct Frame {
  Address to;
  Address from;
  uint8_t proto;                // Proto::DEP или Proto::DMP
  uint8_t msg_len;              // число байт в msg (без CRC2)
  uint8_t msg[kMaxMsg];         // DEVICE, COMMAND, далее содержимое
  Frame() : proto(0), msg_len(0) { memset(msg, 0, sizeof(msg)); }
  uint8_t device() const { return msg_len > 0 ? msg[0] : 0; }
  uint8_t command() const { return msg_len > 1 ? msg[1] : 0; }
  bool is_dep() const { return proto == static_cast<uint8_t>(Proto::DEP); }
  bool is_dmp() const { return proto == static_cast<uint8_t>(Proto::DMP); }
};

uint8_t xor_sum(const uint8_t *data, size_t n);

// Безопасный hex-дамп "XX XX ..." в out (с завершающим нулём, обрезается по cap).
// Возвращает число записанных символов без нуля. Заменяет самодельные sprintf-циклы в логах.
size_t format_hex(const uint8_t *data, size_t n, char *out, size_t cap);

// Возвращает число записанных байт (55, SIZE, тело, SIZE) или 0 при ошибке.
size_t encode(const Frame &f, uint8_t *out, size_t cap);
// buf начинается с 0x55 и содержит ровно один кадр.
ParseError decode(const uint8_t *buf, size_t len, Frame &out);
bool operator==(const Frame &a, const Frame &b);

// ---- построители ----
Frame make_cmd(Address to, Address from, uint8_t code, uint8_t identity = static_cast<uint8_t>(Device::OVIEW),
               uint8_t offset = cmd::DEFAULT_OFFSET);
Frame make_dmp(Address to, Address from, uint8_t device, uint8_t reg, uint8_t run_op, const uint8_t *data = nullptr,
               uint8_t len = 0, uint8_t offset = 0);

// ---- разбор DMP ----
struct DmpRequest {
  uint8_t device, reg, op, offset, len;
  const uint8_t *data;
};
struct DmpResponse {
  uint8_t device, reg, run, next, status;
  uint8_t data_len;
  const uint8_t *data;
  bool fin() const { return (run & run::FIN) != 0; }
  bool ok() const { return status == status::OK; }
};
bool parse_dmp_request(const Frame &f, DmpRequest &out);
bool parse_dmp_response(const Frame &f, DmpResponse &out);

// ---- разбор DEP ----
struct DepCmdView {
  uint8_t device, cmd, offset;
};
struct DepStatusView {  // RSP и EVT
  uint8_t device, command, run, d1, d2, status;
  uint16_t value() const { return static_cast<uint16_t>((d1 << 8) | d2); }
};
DepKind classify_dep(const Frame &f);
bool parse_dep_cmd(const Frame &f, DepCmdView &out);
bool parse_dep_status(const Frame &f, DepStatusView &out);

// Является ли resp ответом DMP на запрос req (адреса, регистр, RUN).
// any_run: принимать любой RUN (для регистров приёмника с нестандартным RUN).
bool response_matches(const Frame &req, const Frame &resp, bool any_run = false);

}  // namespace nice_t4
}  // namespace esphome

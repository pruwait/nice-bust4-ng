#include "t4_state.h"

namespace esphome {
namespace nice_t4 {

const char *gate_state_name(GateState s) {
  switch (s) {
    case GateState::STOPPED: return "stopped";
    case GateState::OPENING: return "opening";
    case GateState::CLOSING: return "closing";
    case GateState::OPEN: return "open";
    case GateState::CLOSED: return "closed";
    case GateState::TIMEOUT: return "timeout";
    case GateState::SEARCH_DONE: return "search_done";
    case GateState::PARTIAL: return "partial";
    case GateState::UNKNOWN: break;
  }
  return "unknown";
}

bool gate_state_from_code(uint8_t code, GateState &out) {
  switch (code) {
    case 0x00: out = GateState::UNKNOWN; return true;
    case 0x01:
    case 0x08: out = GateState::STOPPED; return true;
    case 0x02:
    case 0x83: out = GateState::OPENING; return true;
    case 0x03:
    case 0x84: out = GateState::CLOSING; return true;
    case 0x04: out = GateState::OPEN; return true;
    case 0x05: out = GateState::CLOSED; return true;
    case 0x06: out = GateState::TIMEOUT; return true;
    case 0x0B: out = GateState::SEARCH_DONE; return true;
    case 0x10: out = GateState::PARTIAL; return true;
    default: return false;
  }
}

bool gate_state_from_event(const Frame &f, GateState &out) {
  DepStatusView s;
  if (classify_dep(f) != DepKind::EVT || !parse_dep_status(f, s)) return false;
  if (s.command != 0x02 && s.command != 0x40) return false;
  if (s.status != 0) return false;  // события с ненулевым последним байтом не расшифрованы
  return gate_state_from_code(s.run, out);
}

bool gate_state_from_status(const Frame &f, GateState &out) {
  DmpResponse r;
  if (!parse_dmp_response(f, r)) return false;
  if (r.device != static_cast<uint8_t>(Device::CONTROLLER) || r.reg != 0x01 || !r.ok() || r.data_len < 1) return false;
  return gate_state_from_code(r.data[0], out);
}

}  // namespace nice_t4
}  // namespace esphome

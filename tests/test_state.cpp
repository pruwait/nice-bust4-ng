#include "test_util.h"
#include "t4_state.h"

static GateState evt(const char *label, bool *ok) {
  GateState s = GateState::UNKNOWN;
  *ok = gate_state_from_event(F(V(label)), s);
  return s;
}
static GateState from_status(const char *label, bool *ok) {
  GateState s = GateState::UNKNOWN;
  *ok = gate_state_from_status(F(V(label)), s);
  return s;
}

TEST(state_from_events) {
  bool ok;
  CHECK(evt("EVT sub 40: движение при закрывании", &ok) == GateState::CLOSING); CHECK(ok);   // run 84 (Road 400 и новые блоки)
  CHECK(evt("EVT sub 40: положение 0330", &ok) == GateState::OPENING); CHECK(ok);            // run 02
  CHECK(evt("EVT sub 02: закрытие завершено (05)", &ok) == GateState::CLOSED); CHECK(ok);
  evt("EVT sub 02: код 07", &ok); CHECK(!ok);                                                 // код не расшифрован
  evt("EVT sub 02 на 00 FF: пара одинаковых байт F4", &ok); CHECK(!ok);                       // ненулевой последний байт
  evt("RSP: команда принята", &ok); CHECK(!ok);                                               // RSP не состояние
  evt("GET автозакрывание (80)", &ok); CHECK(!ok);                                            // не DEP
}

TEST(state_from_status_register) {
  bool ok;
  CHECK(from_status("Ответ: закрыто (05)", &ok) == GateState::CLOSED); CHECK(ok);
  CHECK(from_status("Ответ: открыто (04)", &ok) == GateState::OPEN); CHECK(ok);
  CHECK(from_status("Ответ: остановлено (01)", &ok) == GateState::STOPPED); CHECK(ok);
  CHECK(from_status("00 FE 00 00", &ok) == GateState::UNKNOWN); CHECK(ok);
  from_status("Ответ: автозакрывание включено", &ok); CHECK(!ok);                             // другой регистр
  from_status("Ответ с ошибкой FD", &ok); CHECK(!ok);
}

TEST(state_names_and_codes) {
  CHECK(strcmp(gate_state_name(GateState::OPEN), "open") == 0);
  CHECK(strcmp(gate_state_name(GateState::UNKNOWN), "unknown") == 0);
  GateState s = GateState::UNKNOWN;
  CHECK(gate_state_from_code(0x83, s) && s == GateState::OPENING);
  CHECK(!gate_state_from_code(0x07, s));
  CHECK(gate_state_from_code(0x10, s) && s == GateState::PARTIAL);
}

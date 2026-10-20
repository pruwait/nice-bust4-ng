#pragma once
// Состояние ворот по событиям DEP и по регистру 01. Без зависимостей от ESPHome.
#include "t4_frame.h"

namespace esphome {
namespace nice_t4 {

// Читаемое имя состояния (snake_case, подходит для text_sensor и Home Assistant).
const char *gate_state_name(GateState s);

// Код состояния (регистр 01, события COMMAND 02 и 40; 83 и 84 у Road 400) в GateState.
// false для неизвестных кодов. Код 00 даёт UNKNOWN (хаб такие обновления игнорирует).
bool gate_state_from_code(uint8_t code, GateState &out);

// Событие EVT с COMMAND 02 или 40 без ошибки. RSP и прочие кадры дают false.
bool gate_state_from_event(const Frame &f, GateState &out);

// Ответ DMP на GET регистра 01 блока управления.
bool gate_state_from_status(const Frame &f, GateState &out);

}  // namespace nice_t4
}  // namespace esphome

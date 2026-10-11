#pragma once
// Базовые типы и константы протокола Nice BusT4 (T4). Без зависимостей от ESPHome.
#include <stddef.h>
#include <stdint.h>

namespace esphome {
namespace nice_t4 {

static const uint8_t kSync = 0x55;
static const uint8_t kHeaderLen = 7;                       // TO(2) FROM(2) PROTO MSG_SIZE CRC1
static const uint8_t kMaxBody = 63;                        // предел SIZE (буфер прошивки BiDi-WiFi)
static const uint8_t kMinBody = kHeaderLen + 3;            // DEVICE, COMMAND, CRC2
static const uint8_t kMaxMsg = kMaxBody - kHeaderLen - 1;  // байты от DEVICE до конца данных
static const uint8_t kMaxFrame = kMaxBody + 3;             // 55, SIZE, тело, SIZE

enum class Proto : uint8_t { DEP = 0x01, DMP = 0x08 };

enum class Device : uint8_t {
  STANDARD = 0x00,
  OVIEW = 0x01,
  IT4WIFI = 0x02,  // идентичность для команд блокировки (по makstech)
  CONTROLLER = 0x04,
  SCREEN = 0x06,
  RADIO = 0x0A,
  PSU = 0x11,
};

namespace op {  // байт RUN в запросах DMP
static const uint8_t GET = 0x99;
static const uint8_t SET = 0xA9;
static const uint8_t GET_SUPP = 0x89;
}  // namespace op

namespace run {  // флаги байта RUN (позиции выведены из кодов, п. 5.2.1 спецификации)
static const uint8_t REQ = 0x80;
static const uint8_t SET = 0x20;
static const uint8_t GET = 0x10;
static const uint8_t ACK = 0x08;
static const uint8_t FIN = 0x01;
}  // namespace run

namespace status {
static const uint8_t OK = 0x00;
static const uint8_t UNSUPPORTED = 0xFD;
}  // namespace status

namespace cmd {  // команды DEP, основные (полный список: DepCmd в t4_cmds.h)
static const uint8_t RELEASE_LAST = 0x00;
static const uint8_t SBS = 0x01;
static const uint8_t STOP = 0x02;
static const uint8_t OPEN = 0x03;
static const uint8_t CLOSE = 0x04;
static const uint8_t PARTIAL_OPEN_1 = 0x05;
static const uint8_t PARTIAL_OPEN_2 = 0x06;
static const uint8_t PARTIAL_OPEN_3 = 0x07;
static const uint8_t APARTMENT_SBS = 0x0B;
static const uint8_t SBS_HIGH_PRIORITY = 0x0C;
static const uint8_t OPEN_AND_BLOCK = 0x0D;
static const uint8_t CLOSE_AND_BLOCK = 0x0E;
static const uint8_t BLOCK = 0x0F;
static const uint8_t RELEASE = 0x10;
static const uint8_t RELEASE_AND_OPEN = 0x19;
static const uint8_t RELEASE_AND_CLOSE = 0x1A;
static const uint8_t RUN_COMMAND = 0x82;  // COMMAND в кадре CMD
static const uint8_t DEFAULT_OFFSET = 0x64;
}  // namespace cmd

enum class DepKind : uint8_t { CMD, RSP, EVT, UNKNOWN };

enum class ParseError : uint8_t {
  OK,
  TOO_SHORT,
  BAD_SYNC,
  BAD_LENGTH,
  BAD_TAIL,
  BAD_CRC1,
  BAD_MSG_SIZE,
  BAD_CRC2,
};

enum class GateState : uint8_t {
  UNKNOWN = 0x00,
  STOPPED = 0x01,
  OPENING = 0x02,
  CLOSING = 0x03,
  OPEN = 0x04,
  CLOSED = 0x05,
  TIMEOUT = 0x06,
  SEARCH_DONE = 0x0B,
  PARTIAL = 0x10,
};

enum class AutomationType : uint8_t { UNKNOWN = 0, SLIDING = 1, SECTIONAL = 2, SWING = 3, BARRIER = 4, UP_AND_OVER = 5 };

}  // namespace nice_t4
}  // namespace esphome

#pragma once

#include <stdint.h>

namespace esphome {
namespace nice_t4 {

struct Capabilities {
  uint64_t supported_cmds = 0;      // Битовая маска поддерживаемых команд (до 64)
  uint8_t supported_regs[32] = {0}; // Битовая маска поддерживаемых регистров (256 бит)

  bool has_cmd(uint8_t cmd) const {
    return cmd < 64 && ((supported_cmds >> cmd) & 1);
  }
  
  bool has_reg(uint8_t reg) const {
    return (supported_regs[reg / 8] >> (reg % 8)) & 1;
  }
};

}  // namespace nice_t4
}  // namespace esphome

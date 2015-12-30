#ifndef DISASM_H
#define DISASM_H

#include <string>

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"

typedef struct disasm_state {
  std::string instruction;
  std::string mode_suffix;
  uint8_t prefix, suffix;
  uint32_t base_address;
  uint32_t new_address;
  bool mode;            /* 0 for Z80, 1 for ADL */
  uint8_t *data;
} disasm_state_t;

extern disasm_state_t disasm;

void disassembleInstruction();

#ifdef __cplusplus
}
#endif

#endif

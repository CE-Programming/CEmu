#ifndef DISASM_H
#define DISASM_H

#include <string>
#include <stdint.h>

typedef struct {
  std::string opcode;
  std::string arguments;
  std::string mode_suffix;
} eZ80_instuction_t;

typedef struct {
  eZ80_instuction_t instruction;
  uint8_t prefix, suffix;
  uint32_t base_address;
  uint32_t new_address;
  uint8_t IL, IS, S, L;
  uint8_t *data;
} disasm_state_t;

extern disasm_state_t disasm;

void disassembleInstruction(void);

#endif

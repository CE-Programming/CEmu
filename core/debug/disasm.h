#ifndef DISASM_H
#define DISASM_H

#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdbool.h>

typedef std::unordered_map<uint32_t, std::string> addressMap_t;

typedef struct {
    std::string opcode;
    std::string arguments;
    std::string mode_suffix;
    std::string data;
    int size;
} eZ80_instuction_t;

typedef struct {
    eZ80_instuction_t instruction;
    int32_t base_address;
    int32_t new_address;
    uint8_t prefix, suffix;
    bool adl, il;
    addressMap_t address_map;
} disasm_state_t;

extern disasm_state_t disasm;

void disassembleInstruction(void);

#endif

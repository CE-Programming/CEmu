#ifdef DEBUG_SUPPORT

#ifndef DISASM_H
#define DISASM_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
    bool pc;
    bool rWatch;
    bool wWatch;
    bool xBreak;
    int32_t addr;
} disasm_highlights_state_t;

extern disasm_highlights_state_t disasmHighlight;

#ifdef __cplusplus
}

#include <string>
#include <unordered_map>
#include <stdint.h>

typedef std::unordered_multimap<uint32_t, std::string> map_t;
typedef std::unordered_map<std::string, uint32_t> map_value_t;

enum forceTypes {
    FORCE_NONE,
    FORCE_ADL,
    FORCE_NONADL
};

typedef struct {
    std::string opcode;
    std::string arguments;
    std::string modeSuffix;
    std::string data;
    unsigned int size;
} eZ80_instuction_t;

typedef struct {
    eZ80_instuction_t instruction;
    int32_t baseAddress;
    int32_t newAddress;
    uint8_t prefix, suffix;
    bool adl, iw, il, l;
    map_t map;
    map_value_t reverseMap;
    std::string space;
} disasm_state_t;

extern disasm_state_t disasm;

void disassembleInstruction(void);

#endif

#endif

#endif

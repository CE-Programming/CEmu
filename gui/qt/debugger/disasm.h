#ifndef DISASM_H
#define DISASM_H

#include "../../core/debug/zdis/zdis.h"

#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdbool.h>

typedef std::unordered_multimap<uint32_t, std::string> map_t;
typedef std::unordered_map<std::string, uint32_t> map_value_t;

typedef struct {
    struct zdis_ctx ctx;
    std::string *cur;
    struct {
        std::string opcode;
        std::string operands;
        std::string data;
        unsigned int size;
    } instr;
    struct {
        bool pc;
        bool watchR;
        bool watchW;
        bool breakP;
        int32_t addr;
    } highlight;
    int32_t base;
    int32_t next;
    bool adl       : 1;
    bool il        : 1;
    bool uppercase : 1;
    bool implicit  : 1;
    bool bytes     : 1;
    bool addr      : 1;
    bool bold_sym  : 1;
    bool tab       : 1;
    map_t map;
    map_value_t reverse;
    std::string comma;
} disasm_state_t;

extern disasm_state_t disasm;

void disasmInit();
void disasmGet(bool useCpuMode = false);

extern "C" [[gnu::used]] void debugInstruction(void);

#endif

#ifndef DISASM_H
#define DISASM_H

#include <string>
#include <unordered_map>
#include <stdint.h>
#include <stdbool.h>

typedef std::unordered_multimap<uint32_t, std::string> map_t;
typedef std::unordered_map<std::string, uint32_t> map_value_t;

typedef struct {
    struct {
        std::string opcode;
        std::string operands;
        std::string suffix;
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
    uint8_t prefix, suffix;
    bool adl, iw, il, l;
    map_t map;
    map_value_t reverse;
    std::string space;
} disasm_state_t;

extern disasm_state_t disasm;

void disasmInstr(void);

#endif


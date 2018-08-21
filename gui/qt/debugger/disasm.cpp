#include <cstring>
#include <unordered_map>

#include "disasm.h"
#include "../../core/debug/debug.h"
#include "../../core/cpu.h"
#include "../../core/mem.h"

disasm_state_t disasm;

static char tmpbuf[20];

static std::string strW(uint32_t data) {
    bool high = data > 511;
    if (disasm.il) {
        snprintf(tmpbuf, sizeof(tmpbuf), "$%06X", data);
    } else {
        snprintf(tmpbuf, sizeof(tmpbuf), "$%04X", data);
    }
    if (high) {
        std::pair<map_t::iterator, map_t::iterator> range;
        map_t::iterator sit;
        std::string ret;
        range = disasm.map.equal_range(data);
        for (sit = range.first; sit != range.second; ++sit) {
            ret += sit->second;
            ret += '|';
        }
        if (!ret.empty()) {
            ret += std::string(tmpbuf);
            return ret;
        }
        if (!disasm.il) {
            range = disasm.map.equal_range(cpu.registers.MBASE<<16|data);
            for (sit = range.first; sit != range.second; ++sit) {
                ret += sit->second;
                ret += '|';
            }
            if (!ret.empty()) {
                ret += std::string(tmpbuf);
                if (data > 0xFFFF) {
                    ret += " & $FFFF";
                }
                return ret;
            }
        }
    }
    return {tmpbuf};
}

static std::string strA(uint32_t data) {
    map_t::iterator sit;
    std::string ret;
    const bool high = data > 511;
    std::pair<map_t::iterator, map_t::iterator> range = disasm.map.equal_range(data);
    for (sit = range.first; sit != range.second; ++sit) {
        if (high || sit->second[0] == '_') {
            if (!ret.empty()) {
                ret += '|';
            }
            ret += sit->second;
        }
    }
    if (!ret.empty()) {
        return ret;
    }
    if (disasm.il) {
        snprintf(tmpbuf, sizeof(tmpbuf), "$%06X", data);
    } else {
        range = disasm.map.equal_range(cpu.registers.MBASE<<16|data);
        for (sit = range.first; sit != range.second; ++sit) {
            if (high || sit->second[0] == '_') {
                if (!ret.empty()) {
                    ret += '|';
                }
                ret += sit->second;
            }
        }
        if (!ret.empty()) {
            if (data > 0xFFFF) {
                ret += " & $FFFF";
            }
            return ret;
        }
        snprintf(tmpbuf, sizeof(tmpbuf), "$%04X", data);
    }
    return {tmpbuf};
}

static int disasmFetch([[maybe_unused]] struct zdis_ctx *ctx, uint32_t addr) {
    char tmp[3];
    uint8_t value = mem_peek_byte(addr), data;
    if ((data = debug.addr[addr])) {
        disasm.highlight.watchR |= data & DBG_MASK_READ ? true : false;
        disasm.highlight.watchW |= data & DBG_MASK_WRITE ? true : false;
        disasm.highlight.breakP |= data & DBG_MASK_EXEC ? true : false;
        if (data & DBG_INST_START_MARKER && disasm.highlight.addr < 0) {
            disasm.highlight.addr = static_cast<int32_t>(addr);
        }
    }

    if (cpu.registers.PC == addr) {
        disasm.highlight.pc = true;
    }

    if (disasm.bytes) {
        snprintf(tmp, 3, "%02X", value);
        uint8_t offset = (addr - disasm.ctx.zdis_start_addr) * 2;
        if (offset + 2 > disasm.instr.data.size()) {
            disasm.instr.data.resize(offset + 2, '0');
        }
        memcpy(&disasm.instr.data[offset], tmp, 2);
    }

    return value;
}

static bool disasmPut([[maybe_unused]] struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
    char tmp[11], sign = '+';
    disasm.il = il;
    switch (kind) {
        case ZDIS_PUT_BYTE:
        case ZDIS_PUT_PORT:
            snprintf(tmp, 10, "$%02X", val);
            *disasm.cur += tmp;
            break;
        case ZDIS_PUT_WORD:
            *disasm.cur += strW(static_cast<uint32_t>(val));
            break;
        case ZDIS_PUT_OFF:
            if (val < 0) {
                val = -val;
                sign = '-';
            }
            if (val) {
                snprintf(tmp, 11, "%c$%02X", sign, val);
                *disasm.cur += tmp;
            }
            break;
        case ZDIS_PUT_REL:
            val += disasm.ctx.zdis_end_addr;
            [[gnu::fallthrough]];
        case ZDIS_PUT_ADDR:
        case ZDIS_PUT_ABS:
        case ZDIS_PUT_RST:
            *disasm.cur += strA(static_cast<uint32_t>(val));
            break;
        case ZDIS_PUT_CHAR:
            *disasm.cur += char(val);
            break;
        case ZDIS_PUT_MNE_SEP:
            disasm.instr.operands = disasm.tab ? '\t' : ' ';
            disasm.cur = &disasm.instr.operands;
            break;
        case ZDIS_PUT_ARG_SEP:
            *disasm.cur += disasm.comma;
            break;
        case ZDIS_PUT_END:
            disasm.cur = nullptr;
            break;
    }
    return true;
}

void disasmInit() {
    disasm.ctx.zdis_read = disasmFetch;
    disasm.ctx.zdis_put = disasmPut;
}

void disasmGet(bool useCpuMode) {
    disasm.ctx.zdis_lowercase = !disasm.uppercase;
    disasm.ctx.zdis_implicit = !disasm.implicit;
    disasm.ctx.zdis_end_addr = uint32_t(disasm.base);
    disasm.ctx.zdis_adl = useCpuMode ? cpu.ADL : disasm.adl;
    disasm.cur = &disasm.instr.opcode;

    disasm.highlight.watchR = false;
    disasm.highlight.watchW = false;
    disasm.highlight.breakP = false;
    disasm.highlight.pc = false;
    disasm.highlight.addr = -1;

    disasm.instr.data.clear();
    disasm.instr.opcode.clear();
    disasm.instr.operands.clear();

    zdis_put_inst(&disasm.ctx);

    if (disasm.highlight.pc && cpu.registers.PC != uint32_t(disasm.base)) {
        static char tmpbuf[20];
        size_t size = cpu.registers.PC - static_cast<uint32_t>(disasm.base);
        disasm.instr.data = disasm.instr.data.substr(0, size * 2);
        disasm.instr.operands = disasm.tab ? '\t' : ' ';
        int precision;
        if (size % 3 == 0) {
            size /= 3;
            precision = 6;
            disasm.instr.opcode = "dl";
        } else if (size % 2 == 0) {
            size /= 2;
            precision = 4;
            disasm.instr.opcode = "dw";
        } else {
            precision = 2;
            disasm.instr.opcode = "db";
        }
        do {
            snprintf(tmpbuf, sizeof(tmpbuf) - 1, "$%0*X", precision, mem_peek_long(static_cast<uint32_t>(disasm.base)) & ((1 << 4*precision) - 1));
            disasm.instr.operands += tmpbuf;
            if (--size) {
                disasm.instr.operands += ',';
            }
        } while (size);
        disasm.base = int32_t(disasm.ctx.zdis_start_addr);
        disasm.next = int32_t(cpu.registers.PC);
        disasm.highlight.pc = false;
    } else {
        disasm.base = int32_t(disasm.ctx.zdis_start_addr);
        disasm.next = int32_t(disasm.ctx.zdis_end_addr);
    }

    disasm.instr.size = static_cast<unsigned int>(disasm.next) - static_cast<unsigned int>(disasm.base);
}

#include <iostream>
void debugInstruction(void) {
    disasm.base = cpu.registers.PC;
    disasmGet(true);
    std::cerr << disasm.instr.opcode << '\t' << disasm.instr.operands << std::endl;
}

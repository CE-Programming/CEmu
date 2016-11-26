#include <cstring>
#include <unordered_map>

#include "disasm.h"
#include "../../core/debug/debug.h"
#include "../../core/cpu.h"
#include "../../core/mem.h"

disasm_state_t disasm;

static char tmpbuf[20];

static std::string strW(uint32_t data) {
    std::pair<map_t::iterator, map_t::iterator> range;
    map_t::iterator sit;
    std::string ret;
    bool high = data > 511;
    if (disasm.il) {
        sprintf(tmpbuf, "$%06X", data);
    } else {
        sprintf(tmpbuf, "$%04X", data);
    }
    if (high && disasm.map.count(data)) {
        range = disasm.map.equal_range(data);
        for (sit = range.first;  sit != range.second;) {
            if (disasm.bold_sym) {
                ret += "<b>" + sit->second + "</b>";
            } else {
                ret += sit->second;
            }
           ++sit;
           ret += (sit != range.second ? "|" : "");
        }
        if (!ret.empty()) {
            ret += "|";
        }
        ret += std::string(tmpbuf);
        return ret;
    }
    if (!disasm.il) {
        if (high && disasm.map.count(cpu.registers.MBASE<<16|data)) {
            range = disasm.map.equal_range(cpu.registers.MBASE<<16|data);
            for (sit = range.first;  sit != range.second;) {
                if (disasm.bold_sym) {
                    ret += "<b>" + sit->second + "</b>";
                } else {
                    ret += sit->second;
                }
               ++sit;
               ret += (sit != range.second ? "|" : "");
            }
            if (!ret.empty()) {
                ret += "|";
            }
            ret += std::string(tmpbuf);
            if (data > 0xFFFF) {
                ret += " & $FFFF";
            }
            return ret;
        }
    }
    return std::string(tmpbuf);
}

static std::string strA(uint32_t data) {
    std::pair<map_t::iterator, map_t::iterator> range;
    map_t::iterator sit;
    std::string ret;
    bool high = data > 511;
    if (disasm.map.count(data)) {
        range = disasm.map.equal_range(data);
        for (sit = range.first;  sit != range.second;) {
           if (high || (!high && sit->second[0] == '_')) {
               if (disasm.bold_sym) {
                   ret += "<b>" + sit->second + "</b>";
               } else {
                   ret += sit->second;
               }
               ++sit;
               ret += (sit != range.second ? "|" : "");
           } else {
               ++sit;
           }
        }
        if (ret.back() == '|') {
            ret.pop_back();
        }
        return ret;
    }
    if (disasm.il) {
        sprintf(tmpbuf, "$%06X", data);
    } else {
        if (disasm.map.count(cpu.registers.MBASE<<16|data)) {
            range = disasm.map.equal_range(cpu.registers.MBASE<<16|data);
            for (sit = range.first;  sit != range.second;) {
               if (high || (!high && sit->second[0] == '_')) {
                   if (disasm.bold_sym) {
                       ret += "<b>" + sit->second + "</b>";
                   } else {
                       ret += sit->second;
                   }
                   ++sit;
                   ret += (sit != range.second ? "|" : "");
               } else {
                   ++sit;
               }
            }
            if (ret.back() == '|') {
                ret.pop_back();
            }
            if (data > 0xFFFF) {
                ret += " & $FFFF";
            }
            return ret;
        }
        sprintf(tmpbuf, "$%04X", data);
    }
    return std::string(tmpbuf);
}

static int disasmFetch(struct zdis_ctx *ctx, uint32_t addr) {
    char tmp[3];
    uint8_t value = mem_peek_byte(addr), data;
    (void)ctx;
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
        disasm.instr.data += tmp;
    }

    return value;
}

static bool disasmPut(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
    char tmp[11], sign = '+';
    (void)ctx;
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
            [[fallthrough]];
        case ZDIS_PUT_ADDR:
        case ZDIS_PUT_ABS:
        case ZDIS_PUT_RST:
            *disasm.cur += strA(static_cast<uint32_t>(val));
            break;
        case ZDIS_PUT_CHAR:
            *disasm.cur += char(val);
            break;
        case ZDIS_PUT_MNE_SEP:
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

void disasmGet() {
    disasm.ctx.zdis_lowercase = !disasm.uppercase;
    disasm.ctx.zdis_implicit = !disasm.implicit;
    disasm.ctx.zdis_end_addr = uint32_t(disasm.base);
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
        disasm.instr.operands.clear();
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
extern "C" {
__attribute__((used)) void debugInstruction(void) {
    disasm.ctx.zdis_end_addr = cpu.registers.PC;
    disasm.ctx.zdis_adl = cpu.ADL;
    zdis_put_inst(&disasm.ctx);
    std::cerr << disasm.instr.opcode << '\t' << disasm.instr.operands << std::endl;
}
}

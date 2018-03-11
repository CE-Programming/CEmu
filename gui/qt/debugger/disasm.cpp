#include <string.h>
#include <unordered_map>

#include "disasm.h"
#include "../../core/debug/debug.h"
#include "../../core/cpu.h"
#include "../../core/mem.h"

disasm_state_t disasm;

static char tmpbuf[20];

static const std::string index_h[] = {
    "h",
    "i",
    "ixh",
    "iyh",
};

static const std::string index_l[] = {
    "l",
    "i",
    "ixl",
    "iyl",
};

static const std::string index_table[] = {
    "hl",
    "i",
    "ix",
    "iy",
};

static const std::string alu_table[] = {
    "add",
    "adc",
    "sub",
    "sbc",
    "and",
    "xor",
    "or",
    "cp",
};

static const std::string rot_table[] = {
    "rlc",
    "rrc",
    "rl",
    "rr",
    "sla",
    "sra",
    "OPCODETRAP",
    "srl",
};

static const std::string rot_acc_table[] = {
    "rlca",
    "rrca",
    "rla",
    "rra",
    "daa",
    "cpl",
    "scf",
    "ccf",
};

static const std::string cc_table[] = {
    "nz",
    "z",
    "nc",
    "c",
    "po",
    "pe",
    "p",
    "m",
};

static const std::string im_table[] = {
    "0",
    "?",
    "1",
    "2",
};

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
           ret += sit->second;
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
               ret += sit->second;
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
               ret += sit->second;
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
                   ret += sit->second;
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

static std::string strWind(uint32_t data) {
    return "("+strA(data)+")";
}

static std::string strB(uint8_t data) {
    sprintf(tmpbuf, "$%02X", data);
    return std::string(tmpbuf);
}

static std::string strBind(uint8_t data) {
    return "("+strB(data)+")";
}

static std::string strOffset(uint8_t data) {
    if (data & 128) {
        sprintf(tmpbuf, "-$%02X", 256-data);
    } else if (data) {
        sprintf(tmpbuf, "+$%02X", data);
    } else {
        *tmpbuf = '\0';
    }
    return std::string(tmpbuf);
}

static uint8_t disasm_fetch_byte(void) {
    uint32_t addr = disasm.next;
    uint8_t value = mem_peek_byte(addr), data;
    if ((data = debug.addr[addr])) {
        disasm.highlight.watchR |= data & DBG_MASK_READ ? true : false;
        disasm.highlight.watchW |= data & DBG_MASK_WRITE ? true : false;
        disasm.highlight.breakP |= data & DBG_MASK_EXEC ? true : false;
        if (data & DBG_INST_START_MARKER && disasm.highlight.addr < 0) {
            disasm.highlight.addr = addr;
        }
    }

    if (cpu.registers.PC == addr) {
        disasm.highlight.pc = true;
    }

    sprintf(tmpbuf, "%02X", value);
    disasm.instr.data += std::string(tmpbuf);
    disasm.instr.size++;
    disasm.next++;
    return value;
}

static int8_t disasm_fetch_offset(void) {
    return (int8_t)disasm_fetch_byte();
}

static uint32_t disasm_fetch_word(void) {
    uint32_t value = disasm_fetch_byte();
    if (disasm.iw) {
        value |= disasm_fetch_byte() << 8;
        if (disasm.il) {
            value |= disasm_fetch_byte() << 16;
        }
    }
    return value;
}

static std::string disasm_read_index(void) {
    return index_table[disasm.prefix];
}

static std::string disasm_index_address(void) {
    std::string value = disasm_read_index();
    if (disasm.prefix) {
        value += strOffset(disasm_fetch_offset());
    }
    return value;
}

static std::string disasm_read_reg(int i) {
    std::string value;
    switch (i) {
        case 0: value = "b"; break;
        case 1: value = "c"; break;
        case 2: value = "d"; break;
        case 3: value = "e"; break;
        case 4: value = index_h[disasm.prefix]; break;
        case 5: value = index_l[disasm.prefix]; break;
        case 6: value = "("+index_table[disasm.prefix]+ ((disasm.prefix) ? strOffset(disasm_fetch_offset()) : "") +")"; break;
        case 7: value = "a"; break;
        default: break;
    }
    return value;
}

static void disasm_write_reg(int i, std::string const& value) {
    disasm.instr.operands = disasm_read_reg(i)+disasm.space+value;
}

static void disasm_read_write_reg(uint8_t read, uint8_t write) {
    std::string value;
    uint8_t old_prefix = disasm.prefix;
    disasm.prefix = (write != 6) ? old_prefix : 0;
    value = disasm_read_reg(read);
    disasm.prefix = (read != 6) ? old_prefix : 0;
    disasm_write_reg(write, value);
    disasm.instr.opcode = "ld";
}

static std::string disasm_read_reg_prefetched(int i, std::string const& address) {
    std::string value;
    switch (i) {
        case 0: value = "b"; break;
        case 1: value = "c"; break;
        case 2: value = "d"; break;
        case 3: value = "e"; break;
        case 4: value = index_h[disasm.prefix]; break;
        case 5: value = index_l[disasm.prefix]; break;
        case 6: value = "("+address+")"; break;
        case 7: value = "a"; break;
        default: abort();
    }
    return value;
}

static void disasm_write_reg_prefetched(int i, std::string const& address, std::string const& value) {
    disasm.instr.operands = disasm_read_reg_prefetched(i, address)+disasm.space+value;
}

static std::string disasm_read_rp(int i) {
    std::string value;
    switch (i) {
        case 0: value = "bc"; break;
        case 1: value = "de"; break;
        case 2: value = index_table[disasm.prefix]; break;
        case 3: value = "sp"; break;
        default: abort();
    }
    return value;
}

static std::string disasm_read_rp2(int i) {
    if (i == 3) {
        return "af";
    } else {
        return disasm_read_rp(i);
    }
}

static std::string disasm_read_rp3(int i) {
    std::string value;
    switch (i) {
        case 0: value = "bc"; break;
        case 1: value = "de"; break;
        case 2: value = "hl"; break;
        case 3: value = index_table[disasm.prefix]; break;
        default: abort();
    }
    return value;
}

static void disasm_bli(int y, int z) {
    switch (y) {
        case 0:
            switch (z) {
                case 2: // INIM
                    disasm.instr.opcode = "inim";
                    break;
                case 3: // OTIM
                    disasm.instr.opcode = "otim";
                    break;
                case 4: // INI2
                    disasm.instr.opcode = "ini2";
                    break;
            }
            break;
        case 1:
            switch (z) {
                case 2: // INDM
                    disasm.instr.opcode = "indm";
                    break;
                case 3: // OTDM
                    disasm.instr.opcode = "otdm";
                    break;
                case 4: // IND2
                    disasm.instr.opcode = "ind2";
                    break;
            }
            break;
        case 2:
            switch (z) {
                case 2: // INIMR
                    disasm.instr.opcode = "inimr";
                    break;
                case 3: // OTIMR
                    disasm.instr.opcode = "otimr";
                    break;
                case 4: // INI2R
                    disasm.instr.opcode = "ini2r";
                    break;
            }
            break;
        case 3:
            switch (z) {
                case 2: // INDMR
                    disasm.instr.opcode = "indmr";
                    break;
                case 3: // OTDMR
                    disasm.instr.opcode = "otdmr";
                    break;
                case 4: // IND2R
                    disasm.instr.opcode = "ind2r";
                    break;
            }
            break;
        case 4:
            switch (z) {
                case 0: // LDI
                    disasm.instr.opcode = "ldi";
                    break;
                case 1: // CPI
                    disasm.instr.opcode = "cpi";
                    break;
                case 2: // INI
                    disasm.instr.opcode = "ini";
                    break;
                case 3: // OUTI
                    disasm.instr.opcode = "outi";
                    break;
                case 4: // OUTI2
                    disasm.instr.opcode = "outi2";
                    break;
            }
            break;
        case 5:
            switch (z) {
                case 0: // LDD
                    disasm.instr.opcode = "ldd";
                    break;
                case 1: // CPD
                   disasm.instr.opcode = "cpd";
                    break;
                case 2: // IND
                   disasm.instr.opcode = "ind";
                    break;
                case 3: // OUTD
                    disasm.instr.opcode = "outd";
                    break;
                case 4: // OUTD2
                    disasm.instr.opcode = "outd2";
                    break;
            }
            break;
        case 6:
            switch (z) {
                case 0: // LDIR
                    disasm.instr.opcode = "ldir";
                    break;
                case 1: // CPIR
                    disasm.instr.opcode = "cpir";
                    break;
                case 2: // INIR
                    disasm.instr.opcode = "inir";
                    break;
                case 3: // OTIR
                    disasm.instr.opcode = "otir";
                    break;
                case 4: // OTI2R
                    disasm.instr.opcode = "oti2r";
                    break;
            }
            break;
        case 7:
            switch (z) {
                case 0: // LDDR
                    disasm.instr.opcode = "lddr";
                    break;
                case 1: // CPDR
                    disasm.instr.opcode = "cpdr";
                    break;
                case 2: // INDR
                    disasm.instr.opcode = "indr";
                    break;
                case 3: // OTDR
                    disasm.instr.opcode = "otdr";
                    break;
                case 4: // OTD2R
                    disasm.instr.opcode = "otdr2";
                    break;
            }
            break;
    }
}

void disasmInstr(void) {
    std::string old;
    std::string w;
    int32_t o;

    disasm.next = disasm.base;

    disasm.highlight.watchR = false;
    disasm.highlight.watchW = false;
    disasm.highlight.breakP = false;
    disasm.highlight.pc = false;
    disasm.highlight.addr = -1;

    disasm.instr.data = "";
    disasm.instr.opcode = "";
    disasm.instr.suffix = " ";
    disasm.instr.operands = "";
    disasm.instr.size = 0;

    disasm.iw = true;
    disasm.prefix = 0;
    disasm.suffix = 0;

    disasm.il = disasm.adl;
    disasm.l = disasm.adl;

    union {
        uint8_t opcode;
        struct {
            uint8_t z : 3;
            uint8_t y : 3;
            uint8_t x : 2;
        };
        struct {
            uint8_t r : 1;
            uint8_t dummy : 2;
            uint8_t q : 1;
            uint8_t p : 2;
        };
    } context;

    while (true) {
        // fetch opcode
        context.opcode = disasm_fetch_byte();

        switch (context.x) {
            case 0:
                switch (context.z) {
                    case 0:
                        switch (context.y) {
                            case 0:  // NOP
                                disasm.instr.opcode = "nop";
                                break;
                            case 1:  // EX af,af'
                                disasm.instr.opcode = "ex";
                                disasm.instr.operands = "af"+disasm.space+"af'";
                                break;
                            case 2: // DJNZ d
                                o = static_cast<int32_t>(disasm_fetch_offset());
                                disasm.instr.opcode = "djnz";
                                disasm.instr.operands = strA(disasm.next+o);
                                break;
                            case 3: // JR d
                                o = static_cast<int32_t>(disasm_fetch_offset());
                                disasm.instr.opcode = "jr";
                                disasm.instr.operands = strA(disasm.next+o);
                                break;
                            case 4:
                            case 5:
                            case 6:
                            case 7: // JR cc[y-4], d
                                o = static_cast<int32_t>(disasm_fetch_offset());
                                disasm.instr.opcode = "jr";
                                disasm.instr.operands = cc_table[context.y-4]+disasm.space+strA(disasm.next+o);
                                break;
                        }
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // LD rr, Mmn
                                if (context.p == 3 && disasm.prefix) { // LD IY/IX, (IX/IY + d)
                                    disasm.instr.opcode = "ld";
                                    disasm.instr.operands = index_table[disasm.prefix ^ 1] + disasm.space+"(" + index_table[disasm.prefix] + strOffset(disasm_fetch_offset()) + ")" ;
                                    break;
                                }
                                disasm.instr.opcode = "ld";
                                disasm.instr.operands = disasm_read_rp(context.p)+disasm.space+strW(disasm_fetch_word());
                                break;
                            case 1: // ADD HL,rr
                                disasm.instr.opcode = "add";
                                disasm.instr.operands = index_table[disasm.prefix]+disasm.space+disasm_read_rp(context.p);
                                break;
                        }
                        break;
                    case 2:
                        switch (context.q) {
                            case 0:
                                switch (context.p) {
                                    case 0: // LD (BC), A
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "(bc)"+disasm.space+"a";
                                        break;
                                    case 1: // LD (DE), A
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "(de)"+disasm.space+"a";
                                        break;
                                    case 2: // LD (Mmn), HL
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = strWind(disasm_fetch_word())+disasm.space+index_table[disasm.prefix];
                                        break;
                                    case 3: // LD (Mmn), A
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = strWind(disasm_fetch_word())+disasm.space+"a";
                                        break;
                                }
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // LD A, (BC)
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "a"+disasm.space+"(bc)";
                                        break;
                                    case 1: // LD A, (DE)
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "a"+disasm.space+"(de)";
                                        break;
                                    case 2: // LD HL, (Mmn)
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = index_table[disasm.prefix]+disasm.space+strWind(disasm_fetch_word());
                                        break;
                                    case 3: // LD A, (Mmn)
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "a"+disasm.space+strWind(disasm_fetch_word());
                                        break;
                                }
                                break;
                        }
                        break;
                    case 3:
                        switch (context.q) {
                            case 0: // INC rp[p]
                                disasm.instr.opcode = "inc";
                                disasm.instr.operands = disasm_read_rp(context.p);
                                break;
                            case 1: // DEC rp[p]
                                disasm.instr.opcode = "dec";
                                disasm.instr.operands = disasm_read_rp(context.p);
                                break;
                        }
                        break;
                    case 4: // INC r[y]
                        disasm.instr.opcode = "inc";
                        w = (context.y == 6) ? disasm_index_address() : "0";
                        disasm.instr.operands = disasm_read_reg_prefetched(context.y, w);
                        break;
                    case 5: // DEC r[y]
                        disasm.instr.opcode = "dec";
                        w = (context.y == 6) ? disasm_index_address() : "0";
                        disasm.instr.operands = disasm_read_reg_prefetched(context.y, w);
                        break;
                    case 6: // LD r[y], n
                        if (context.y == 7 && disasm.prefix) { // LD (IX/IY + d), IY/IX
                            disasm.instr.opcode = "ld";
                            disasm.instr.operands = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+")"+disasm.space+index_table[disasm.prefix ^ 1];
                            break;
                        }
                        disasm.instr.opcode = "ld";
                        w = (context.y == 6) ? disasm_index_address() : "";
                        disasm_write_reg_prefetched(context.y, w, strB(disasm_fetch_byte()));
                        break;
                    case 7:
                        if (disasm.prefix) {
                            if (context.q) { // LD (IX/IY + d), rp3[p]
                                disasm.instr.opcode = "ld";
                                disasm.instr.operands = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+")"+disasm.space+disasm_read_rp3(context.p);
                            } else { // LD rp3[p], (IX/IY + d)
                                disasm.instr.opcode = "ld";
                                disasm.instr.operands = disasm_read_rp3(context.p)+disasm.space+"("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+")";
                            }
                        } else {
                            disasm.instr.opcode = rot_acc_table[context.y];
                        }
                        break;
                }
                break;
            case 1: // ignore prefixed prefixes
                if (context.z == context.y) {
                    switch (context.z) {
                        case 0: // .SIS
                            disasm.suffix = 1;
                            disasm.instr.suffix = ".sis ";
                            disasm.l = false;
                            disasm.il = false;
                            continue;
                        case 1: // .LIS
                            disasm.suffix = 1;
                            disasm.instr.suffix = ".lis ";
                            disasm.l = true;
                            disasm.il = false;
                            continue;
                        case 2: // .SIL
                            disasm.suffix = 1;
                            disasm.instr.suffix = ".sil ";
                            disasm.l = false;
                            disasm.il = true;
                            continue;
                        case 3: // .LIL
                            disasm.suffix = 1;
                            disasm.instr.suffix = ".lil ";
                            disasm.l = true;
                            disasm.il = true;
                            continue;
                        case 6: // HALT
                            disasm.instr.opcode = "halt";
                            break;
                        case 4: // LD H, H
                            disasm.instr.opcode = "ld";
                            disasm.instr.operands = index_h[disasm.prefix]+disasm.space+index_h[disasm.prefix];
                            break;
                        case 5: // LD L, L
                            disasm.instr.opcode = "ld";
                            disasm.instr.operands = index_l[disasm.prefix]+disasm.space+index_l[disasm.prefix];
                            break;
                        case 7: // LD A, A
                            disasm.instr.opcode = "ld";
                            disasm.instr.operands = "a"+disasm.space+"a";
                            break;
                        default:
                            abort();
                    }
                } else {
                    disasm_read_write_reg(context.z, context.y);
                }
                break;
            case 2: // ALU[y] r[z]
                disasm.instr.opcode = alu_table[context.y];
                disasm.instr.operands = "a"+disasm.space+disasm_read_reg(context.z);
                break;
            case 3:
                switch (context.z) {
                    case 0: // RET cc[y]
                        disasm.instr.opcode = "ret";
                        disasm.instr.operands = cc_table[context.y];
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // POP rp2[p]
                                disasm.instr.opcode = "pop";
                                disasm.instr.operands = disasm_read_rp2(context.p);
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // RET
                                        disasm.instr.opcode = "ret";
                                        break;
                                    case 1: // EXX
                                        disasm.instr.opcode = "exx";
                                        break;
                                    case 2: // JP (rr)
                                        disasm.instr.opcode = "jp";
                                        disasm.instr.operands = "("+index_table[disasm.prefix]+")";
                                        break;
                                    case 3: // LD SP, INDEX
                                        disasm.instr.opcode = "ld";
                                        disasm.instr.operands = "sp"+disasm.space+index_table[disasm.prefix];
                                        break;
                                }
                                break;
                        }
                        break;
                    case 2: // JP cc[y], nn
                        disasm.instr.opcode = "jp";
                        disasm.instr.operands = cc_table[context.y]+disasm.space+strA(disasm_fetch_word());
                        break;
                    case 3:
                        switch (context.y) {
                            case 0: // JP nn
                                disasm.instr.opcode = "jp";
                                disasm.instr.operands = strA(disasm_fetch_word());
                                break;
                            case 1: // 0xCB prefixed opcodes
                                w = disasm_index_address();
                                context.opcode = disasm_fetch_byte();
                                old = disasm_read_reg_prefetched(context.z, w);
                                switch (context.x) {
                                    case 0: // rot[y] r[z]
                                        disasm.instr.opcode = rot_table[context.y];
                                        disasm.instr.operands = old;
                                        break;
                                    case 1: // BIT y, r[z]
                                        disasm.instr.opcode = "bit";
                                        disasm.instr.operands = std::to_string(context.y)+disasm.space+old;
                                        break;
                                    case 2: // RES y, r[z]
                                        disasm.instr.opcode = "res";
                                        disasm.instr.operands = std::to_string(context.y)+disasm.space+old;
                                        break;
                                    case 3: // SET y, r[z]
                                        disasm.instr.opcode = "set";
                                        disasm.instr.operands = std::to_string(context.y)+disasm.space+old;
                                        break;
                                }
                                break;
                            case 2: // OUT (n), A
                                disasm.instr.opcode = "out";
                                disasm.instr.operands = strBind(disasm_fetch_byte())+disasm.space+"a";
                                break;
                            case 3: // IN A, (n)
                                disasm.instr.opcode = "in";
                                disasm.instr.operands = "a"+disasm.space+strBind(disasm_fetch_byte());
                                break;
                            case 4: // EX (SP), HL/I
                                disasm.instr.opcode = "ex";
                                disasm.instr.operands = "(sp)"+disasm.space+index_table[disasm.prefix];
                                break;
                            case 5: // EX DE, HL
                                disasm.instr.opcode = "ex";
                                disasm.instr.operands = "de"+disasm.space+"hl";
                                break;
                            case 6: // DI
                                disasm.instr.opcode = "di";
                                break;
                            case 7: // EI
                                disasm.instr.opcode = "ei";
                                break;
                        }
                        break;
                    case 4: // CALL cc[y], nn
                        disasm.instr.opcode = "call";
                        disasm.instr.operands = cc_table[context.y]+disasm.space+strA(disasm_fetch_word());
                        break;
                    case 5:
                        switch (context.q) {
                            case 0: // PUSH r2p[p]
                                disasm.instr.opcode = "push";
                                disasm.instr.operands = disasm_read_rp2(context.p);
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // CALL nn
                                        disasm.instr.opcode = "call";
                                        disasm.instr.operands = strA(disasm_fetch_word());
                                        break;
                                    case 1: // 0xDD prefixed opcodes
                                        disasm.prefix = 2;
                                        continue;
                                    case 2: // 0xED prefixed opcodes
                                        disasm.prefix = 0; // ED cancels effect of DD/FD prefix
                                        context.opcode = disasm_fetch_byte();
                                        switch (context.x) {
                                            case 0:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP
                                                            disasm.instr.opcode = "OPCODETRAP";
                                                            disasm.instr.operands.clear();
                                                        } else { // IN0 r[y], (n)
                                                            disasm.instr.opcode = "in0";
                                                            disasm.instr.operands = disasm_read_reg(context.y)+disasm.space+strBind(disasm_fetch_byte());
                                                        }
                                                        break;
                                                     case 1:
                                                        if (context.y == 6) { // LD IY, (HL)
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = "iy"+disasm.space+"(hl)";
                                                        } else { // OUT0 (n), r[y]
                                                            disasm.instr.opcode = "out0";
                                                            disasm.instr.operands = strBind(disasm_fetch_byte())+disasm.space+disasm_read_reg(context.y);
                                                        }
                                                        break;
                                                    case 2: // LEA rp3[p], IX + d
                                                    case 3: // LEA rp3[p], IY + d
                                                        if (context.q) { // OPCODETRAP
                                                            disasm.instr.opcode = "OPCODETRAP";
                                                            disasm.instr.operands.clear();
                                                        } else {
                                                            disasm.prefix = context.z;
                                                            disasm.instr.opcode = "lea";
                                                            disasm.instr.operands = disasm_read_rp3(context.p)+disasm.space+index_table[context.z]+strOffset(disasm_fetch_offset());
                                                        }
                                                        break;
                                                    case 4: // TST A, r[y]
                                                        disasm.instr.opcode = "tst";
                                                        disasm.instr.operands = "a"+disasm.space+disasm_read_reg(context.y);
                                                        break;
                                                    case 6:
                                                        if (context.y == 7) { // LD (HL), IY
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = "(hl)"+disasm.space+"iy";
                                                            break;
                                                        }
                                                        // fallthrough
                                                    case 5: // OPCODETRAP
                                                        disasm.instr.opcode = "OPCODETRAP";
                                                        disasm.instr.operands.clear();
                                                        break;
                                                    case 7:
                                                        disasm.prefix = 2;
                                                        if (context.q) { // LD (HL), rp3[p]
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = "(hl)"+disasm.space+disasm_read_rp3(context.p);
                                                        } else { // LD rp3[p], (HL)
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = disasm_read_rp3(context.p)+disasm.space+"(hl)";
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 1:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instr.opcode = "OPCODETRAP";
                                                            disasm.instr.operands.clear();
                                                        } else { // IN r[y], (BC)
                                                            disasm.instr.opcode = "in";
                                                            disasm.instr.operands = disasm_read_reg(context.y)+disasm.space+"(bc)";
                                                        }
                                                        break;
                                                    case 1:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instr.opcode = "OPCODETRAP";
                                                            disasm.instr.operands.clear();
                                                        } else { // OUT (BC), r[y]
                                                            disasm.instr.opcode = "out";
                                                            disasm.instr.operands = "(bc)"+disasm.space+disasm_read_reg(context.y);
                                                        }
                                                        break;
                                                    case 2:
                                                        if (context.q == 0) { // SBC HL, rp[p]
                                                            disasm.instr.opcode = "sbc";
                                                            disasm.instr.operands = "hl"+disasm.space+disasm_read_rp(context.p);
                                                        } else { // ADC HL, rp[p]
                                                            disasm.instr.opcode = "adc";
                                                            disasm.instr.operands = "hl"+disasm.space+disasm_read_rp(context.p);
                                                        }
                                                        break;
                                                    case 3:
                                                        if (context.q == 0) { // LD (nn), rp[p]
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = strWind(disasm_fetch_word())+disasm.space+disasm_read_rp(context.p);
                                                        } else { // LD rp[p], (nn)
                                                            disasm.instr.opcode = "ld";
                                                            disasm.instr.operands = disasm_read_rp(context.p)+disasm.space+strWind(disasm_fetch_word());
                                                        }
                                                        break;
                                                    case 4:
                                                        if (context.q == 0) {
                                                            switch (context.p) {
                                                                case 0:  // NEG
                                                                    disasm.instr.opcode = "neg";
                                                                    break;
                                                                case 1:  // LEA IX, IY + d
                                                                    disasm.instr.opcode = "lea";
                                                                    disasm.instr.operands = "ix"+disasm.space+"iy"+strOffset(disasm_fetch_byte());
                                                                    break;
                                                                case 2:  // TST A, n
                                                                    disasm.instr.opcode = "tst";
                                                                    disasm.instr.operands = "a"+disasm.space+strB(disasm_fetch_byte());
                                                                    break;
                                                                case 3:  // TSTIO n
                                                                    disasm.instr.opcode = "tstio";
                                                                    disasm.instr.operands = strB(disasm_fetch_byte());
                                                                    break;
                                                            }
                                                        }
                                                        else { // MLT rp[p]
                                                            disasm.instr.opcode = "mlt";
                                                            disasm.instr.operands = disasm_read_rp(context.p);
                                                            break;
                                                        }
                                                        break;
                                                    case 5:
                                                        switch (context.y) {
                                                            case 0: // RETN
                                                                disasm.instr.opcode = "retn";
                                                                break;
                                                            case 1: // RETI
                                                                disasm.instr.opcode = "reti";
                                                                break;
                                                            case 2: // LEA IY, IX + d
                                                                disasm.instr.opcode = "lea";
                                                                disasm.instr.operands = "iy"+disasm.space+"ix"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 3:
                                                            case 6: // OPCODETRAP
                                                                disasm.instr.opcode = "OPCODETRAP";
                                                                disasm.instr.operands.clear();
                                                                break;
                                                            case 4: // PEA IX + d
                                                                disasm.instr.opcode = "pea";
                                                                disasm.instr.operands = "ix"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 5: // LD MB, A
                                                                if (disasm.il) {
                                                                    disasm.instr.opcode = "ld";
                                                                    disasm.instr.operands = "mb"+disasm.space+"a";
                                                                } else { // OPCODETRAP
                                                                    disasm.instr.opcode = "OPCODETRAP";
                                                                    disasm.instr.operands.clear();
                                                                }
                                                                break;
                                                            case 7: // STMIX
                                                                disasm.instr.opcode = "stmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 6: // IM im[y]
                                                        switch (context.y) {
                                                            case 0:
                                                            case 2:
                                                            case 3: // IM im[y]
                                                                disasm.instr.opcode = "im";
                                                                disasm.instr.operands = im_table[context.y];
                                                                break;
                                                            case 1: // OPCODETRAP
                                                                disasm.instr.opcode = "OPCODETRAP";
                                                                disasm.instr.operands.clear();
                                                                break;
                                                            case 4: // PEA IY + d
                                                                disasm.instr.opcode = "pea";
                                                                disasm.instr.operands = "iy"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 5: // LD A, MB
                                                                if (disasm.il) {
                                                                    disasm.instr.opcode = "ld";
                                                                    disasm.instr.operands = "a"+disasm.space+"mb";
                                                                } else { // OPCODETRAP
                                                                    disasm.instr.opcode = "OPCODETRAP";
                                                                    disasm.instr.operands.clear();
                                                                }
                                                                break;
                                                            case 6: // SLP
                                                                disasm.instr.operands = "slp";
                                                                break;
                                                            case 7: // RSMIX
                                                                disasm.instr.opcode = "rsmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 7:
                                                        switch (context.y) {
                                                            case 0: // LD I, A
                                                                disasm.instr.opcode = "ld";
                                                                disasm.instr.operands = "i"+disasm.space+"a";
                                                                break;
                                                            case 1: // LD R, A
                                                                disasm.instr.opcode = "ld";
                                                                disasm.instr.operands = "r"+disasm.space+"a";
                                                                break;
                                                            case 2: // LD A, I
                                                                disasm.instr.opcode = "ld";
                                                                disasm.instr.operands = "a"+disasm.space+"i";
                                                                break;
                                                            case 3: // LD A, R
                                                                disasm.instr.opcode = "ld";
                                                                disasm.instr.operands = "a"+disasm.space+"r";
                                                                break;
                                                            case 4: // RRD
                                                                disasm.instr.opcode = "rrd";
                                                                break;
                                                            case 5: // RLD
                                                                disasm.instr.opcode = "rld";
                                                                break;
                                                            default: // OPCODETRAP
                                                                disasm.instr.opcode = "OPCODETRAP";
                                                                disasm.instr.operands.clear();
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 2:
                                                if (context.z <= 4) { // bli[y,z]
                                                    disasm_bli(context.y, context.z);
                                                } else { // OPCODETRAP
                                                    disasm.instr.opcode = "OPCODETRAP";
                                                    disasm.instr.operands.clear();
                                                }
                                                break;
                                            case 3:  // There are only a few of these, so a simple switch for these shouldn't matter too much
                                                switch(context.opcode) {
                                                    case 0xC2: // INIRX
                                                        disasm.instr.opcode = "inirx";
                                                        break;
                                                    case 0xC3: // OTIRX
                                                        disasm.instr.opcode = "otirx";
                                                        break;
                                                    case 0xC7: // LD I, HL
                                                        disasm.instr.opcode = "ld";
                                                        disasm.instr.operands = "i"+disasm.space+"hl";
                                                        break;
                                                    case 0xD7: // LD HL, I
                                                        disasm.instr.opcode = "ld";
                                                        disasm.instr.operands = "hl"+disasm.space+"i";
                                                        break;
                                                    case 0xCA: // INDRX
                                                        disasm.instr.opcode = "indrx";
                                                        break;
                                                    case 0xCB: // OTDRX
                                                        disasm.instr.opcode = "otdrx";
                                                        break;
                                                    case 0xEE: // flash erase
                                                        disasm.instr.opcode = "FLASH_ERASE";
                                                        break;
                                                    default:   // OPCODETRAP
                                                        disasm.instr.opcode = "OPCODETRAP";
                                                        disasm.instr.operands.clear();
                                                        break;
                                                }
                                                break;
                                            default: // OPCODETRAP
                                                disasm.instr.opcode = "OPCODETRAP";
                                                disasm.instr.operands.clear();
                                                break;
                                        }
                                        break;
                                    case 3: // 0xFD prefixed opcodes
                                        disasm.prefix = 3;
                                        continue;
                                }
                                break;
                        }
                        break;
                    case 6: // alu[y] n
                        disasm.instr.opcode = alu_table[context.y];
                        disasm.instr.operands = "a"+disasm.space+strB(disasm_fetch_byte());
                        break;
                    case 7: // RST y*8
                        disasm.instr.opcode = "rst";
                        disasm.instr.operands = strA(context.y << 3);
                        break;
                }
                break;
        }
        break;
    }
    int32_t size = disasm.highlight.addr - disasm.base;
    if (size > 0) {
        int precision;
        disasm.next = disasm.base;

        disasm.highlight.watchR = false;
        disasm.highlight.watchW = false;
        disasm.highlight.breakP = false;
        disasm.highlight.pc = false;
        disasm.highlight.addr = -1;

        disasm.instr.data = "";
        if (size % 3 == 0) {
            size /= 3;
            precision = 6;
            disasm.instr.opcode = ".dl";
            disasm.il = true;
        } else if (size % 2 == 0) {
            size /= 2;
            precision = 4;
            disasm.instr.opcode = ".dw";
            disasm.il = false;
        } else {
            precision = 2;
            disasm.instr.opcode = ".db";
            disasm.iw = false;
        }
        disasm.instr.suffix = " ";
        disasm.instr.operands = "";
        disasm.instr.size = 0;

        do {
            sprintf(tmpbuf,"$%0*X", precision, disasm_fetch_word());
            disasm.instr.operands += tmpbuf;
            if (--size) {
                disasm.instr.operands += ',';
            }
        } while (size);
    }
}

#include <string.h>

#include "disasm.h"

disasm_state_t disasm;

static char tmpbuf[20];

static const std::string reg_table[] = {
    "b",
    "c",
    "d",
    "e",
    "h",
    "l",
    "(hl)",
    "a",
};

static const std::string index_h[] = {
    "h",
    "",
    "ixh",
    "iyh",
};

static const std::string index_l[] = {
    "l",
    "",
    "ixl",
    "iyl",
};

static const std::string index_table[] = {
    "hl",
    "i",
    "ix",
    "iy",
};

static const std::string rp_table[] = {
    "bc",
    "de",
    "hl",
    "sp",
};

static const std::string rp2_table[] = {
    "bc",
    "de",
    "hl",
    "af",
};

static const std::string rp3_table[] = {
    "bc",
    "de",
    "hl",
    "af",
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
    "0",
    "?",
    "1",
    "2",
};

static std::string strW(uint32_t data) {
    if(disasm.IL) {
        sprintf(tmpbuf,"%06X",data);
    } else {
        sprintf(tmpbuf,"%04X",data);
    }
    return std::string(tmpbuf);
}

static std::string strWind(uint32_t data) {
    if(disasm.IL) {
        sprintf(tmpbuf,"(%06X)",data);
    } else {
        sprintf(tmpbuf,"(%04X)",data);
    }
    return std::string(tmpbuf);
}

static std::string strS(uint8_t data) {
    sprintf(tmpbuf,"%02X",data);
    return std::string(tmpbuf);
}

static std::string strSind(uint8_t data) {
    sprintf(tmpbuf,"(%02X)",data);
    return std::string(tmpbuf);
}


static std::string strOffset(uint8_t data) {
    if (data & 128) {
        sprintf(tmpbuf,"-%02X",(0-data)&0xFF);
    } else {
        sprintf(tmpbuf,"+%02X",data);
    }
    return std::string(tmpbuf);
}

static uint8_t disasm_fetch_byte(void) {
    return disasm.data[disasm.new_address++];
}

static int8_t disasm_fetch_offset(void) {
    return (int8_t)disasm_fetch_byte();
}

static uint32_t disasm_fetch_word(void) {
    uint32_t value = disasm_fetch_byte();
    value |= disasm_fetch_byte() << 8;
    if (disasm.IL) {
        value |= disasm_fetch_byte() << 16;
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
        case 6: value = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+")"; break;
        case 7: value = "a"; break;
        default: break;
    }
    return value;
}
static void disasm_write_reg(int i, std::string value) {
    switch (i) {
        case 0: disasm.instruction.arguments = "b,"+value; break;
        case 1: disasm.instruction.arguments = "c,"+value; break;
        case 2: disasm.instruction.arguments = "d,"+value; break;
        case 3: disasm.instruction.arguments = "e,"+value; break;
        case 4: disasm.instruction.arguments = index_h[disasm.prefix]+","+value; break;
        case 5: disasm.instruction.arguments = index_l[disasm.prefix]+","+value; break;
        case 6: disasm.instruction.arguments = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+"),"+value; break;
        case 7: disasm.instruction.arguments = "a,"+value; break;
        default: break;
    }
}

static void disasm_read_write_reg(uint8_t read, uint8_t write) {
    std::string value;
    uint8_t old_prefix = disasm.prefix;
    disasm.prefix = (write != 6) ? old_prefix : 0;
    value = disasm_read_reg(read);
    disasm.prefix = (read != 6) ? old_prefix : 0;
    disasm_write_reg(write, value);
    disasm.instruction.opcode = "ld";
}

static std::string disasm_read_reg_prefetched(int i, std::string address) {
    std::string value;
    switch (i) {
        case 0: value = "b"; break;
        case 1: value = "c"; break;
        case 2: value = "d"; break;
        case 3: value = "e"; break;
        case 4: value = index_h[disasm.prefix]; break;
        case 5: value = index_l[disasm.prefix]; break;
        //case 6: value = disasm_read_byte(address); break;
        case 7: value = "a"; break;
        default: abort();
    }
    return value;
}

static void disasm_write_reg_prefetched(int i, std::string address, std::string value) {
    switch (i) {
        case 0: disasm.instruction.arguments = "b,"+value; break;
        case 1: disasm.instruction.arguments = "c,"+value; break;
        case 2: disasm.instruction.arguments = "d,"+value; break;
        case 3: disasm.instruction.arguments = "e,"+value; break;
        case 4: disasm.instruction.arguments = index_h[disasm.prefix]+","+value; break;
        case 5: disasm.instruction.arguments = index_l[disasm.prefix]+","+value; break;
        //case 6: disasm.instruction.arguments = value; break;
        case 7: disasm.instruction.arguments = "a,"+value; break;
        default: abort();
    }
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

static void disasm_write_rp(int i, std::string value) {
    switch (i) {
        case 0: disasm.instruction.arguments = "bc,"+value; break;
        case 1: disasm.instruction.arguments = "de,"+value; break;
        case 2: disasm.instruction.arguments = index_table[disasm.prefix]+","+value; break;
        case 3: disasm.instruction.arguments = "sp,"+value; break;
        default: abort();
    }
}

static std::string disasm_read_rp2(int i) {
    if (i == 3) {
        return "af";
    } else {
        return disasm_read_rp(i);
    }
}

static void disasm_write_rp2(int i, std::string value) {
    if (i == 3) {
        disasm.instruction.arguments = "af,"+value;
    } else {
        disasm_write_rp(i, value);
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
static void cpu_write_rp3(int i, std::string value) {
    switch (i) {
        case 0: disasm.instruction.arguments = "bc,"+value; break;
        case 1: disasm.instruction.arguments = "de,"+value; break;
        case 2: disasm.instruction.arguments = "hl,"+value; break;
        case 3: disasm.instruction.arguments = index_table[disasm.prefix]+","+value; break;
        default: abort();
    }
}

static void disasm_bli(int y, int z) {
    switch (y) {
        case 0:
            switch (z) {
                case 2: // INIM
                    disasm.instruction.opcode = "inim";
                    break;
                case 3: // OTIM
                    disasm.instruction.opcode = "otim";
                    break;
                case 4: // INI2
                    disasm.instruction.opcode = "ini2";
                    break;
            }
            break;
        case 1:
            switch (z) {
                case 2: // INDM
                    disasm.instruction.opcode = "indm";
                    break;
                case 3: // OTDM
                    disasm.instruction.opcode = "otdm";
                    break;
                case 4: // IND2
                    disasm.instruction.opcode = "ind2";
                    break;
            }
            break;
        case 2:
            switch (z) {
                case 2: // INIMR
                    disasm.instruction.opcode = "inimr";
                    break;
                case 3: // OTIMR
                    disasm.instruction.opcode = "otimr";
                    break;
                case 4: // INI2R
                    disasm.instruction.opcode = "ini2r";
                    break;
            }
            break;
        case 3:
            switch (z) {
                case 2: // INDMR
                    disasm.instruction.opcode = "indmr";
                    break;
                case 3: // OTDMR
                    disasm.instruction.opcode = "otdmr";
                    break;
                case 4: // IND2R
                    disasm.instruction.opcode = "ind2r";
                    break;
            }
            break;
        case 4:
            switch (z) {
                case 0: // LDI
                    disasm.instruction.opcode = "ldi";
                    break;
                case 1: // CPI
                    disasm.instruction.opcode = "cpi";
                    break;
                case 2: // INI
                    disasm.instruction.opcode = "ini";
                    break;
                case 3: // OUTI
                    disasm.instruction.opcode = "outi";
                    break;
                case 4: // OUTI2
                    disasm.instruction.opcode = "outi2";
                    break;
            }
            break;
        case 5:
            switch (z) {
                case 0: // LDD
                    disasm.instruction.opcode = "ldd";
                    break;
                case 1: // CPD
                   disasm.instruction.opcode = "cpd";
                    break;
                case 2: // IND
                   disasm.instruction.opcode = "ind";
                    break;
                case 3: // OUTD
                    disasm.instruction.opcode = "outd";
                    break;
                case 4: // OUTD2
                    disasm.instruction.opcode = "outd2";
                    break;
            }
            break;
        case 6:
            switch (z) {
                case 0: // LDIR
                    disasm.instruction.opcode = "ldir";
                    break;
                case 1: // CPIR
                    disasm.instruction.opcode = "cpir";
                    break;
                case 2: // INIR
                    disasm.instruction.opcode = "inir";
                    break;
                case 3: // OTIR
                    disasm.instruction.opcode = "otir";
                    break;
                case 4: // OTI2R
                    disasm.instruction.opcode = "oti2r";
                    break;
            }
            break;
        case 7:
            switch (z) {
                case 0: // LDDR
                    disasm.instruction.opcode = "lddr";
                    break;
                case 1: // CPDR
                    disasm.instruction.opcode = "cpdr";
                    break;
                case 2: // INDR
                    disasm.instruction.opcode = "indr";
                    break;
                case 3: // OTDR
                    disasm.instruction.opcode = "otdr";
                    break;
                case 4: // OTD2R
                    disasm.instruction.opcode = "otdr2";
                    break;
            }
            break;
    }
}

void disassembleInstruction(void) {
    std::string tmpstr;

    disasm.new_address = disasm.base_address;

    disasm.instruction.opcode = "";
    disasm.instruction.mode_suffix = " ";
    disasm.instruction.arguments = "";

    disasm.IL = 1;
    disasm.prefix = 0;
    disasm.suffix = 0;

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

    do {
        // fetch opcode
        context.opcode = disasm.data[disasm.new_address++];

        switch (context.x) {
            case 0:
                switch (context.z) {
                    case 0:
                        switch (context.y) {
                            case 0:  // NOP
                                disasm.instruction.opcode = "nop";
                                break;
                            case 1:  // EX af,af'
                                disasm.instruction.opcode = "ex";
                                disasm.instruction.arguments = "af,af'";
                                break;
                            case 2: // DJNZ d
                                disasm.instruction.opcode = "djnz";
                                disasm.instruction.arguments = strW(disasm.new_address+1+disasm_fetch_offset());
                                break;
                            case 3: // JR d
                                disasm.instruction.opcode = "jr";
                                disasm.instruction.arguments = strW(disasm.new_address+1+disasm_fetch_offset());
                                break;
                            case 4:
                            case 5:
                            case 6:
                            case 7: // JR cc[y-4], d
                                disasm.instruction.opcode = "jr";
                                disasm.instruction.arguments = cc_table[context.y-4]+","+ strW(disasm.new_address+1+disasm_fetch_offset());
                                break;
                        }
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // LD rr, Mmn
                                if (context.p == 3 && disasm.prefix) { // LD IY/IX, (IX/IY + d)
                                    disasm.instruction.opcode = "ld";
                                    disasm.instruction.arguments = index_table[disasm.prefix] + "(" + index_table[disasm.prefix ^ 1] + strOffset(disasm_fetch_offset()) + ")" ;
                                    break;
                                }
                                disasm.instruction.opcode = "ld";
                                disasm.instruction.arguments = rp_table[context.p]+","+strW(disasm_fetch_word());
                                break;
                            case 1: // ADD HL,rr
                                disasm.instruction.opcode = "add";
                                disasm.instruction.arguments = index_table[disasm.prefix]+","+disasm_read_rp(context.p);
                                break;
                        }
                        break;
                    case 2:
                        switch (context.q) {
                            case 0:
                                switch (context.p) {
                                    case 0: // LD (BC), A
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "(bc),a";
                                        break;
                                    case 1: // LD (DE), A
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "(de),a";
                                        break;
                                    case 2: // LD (Mmn), HL
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = strWind(disasm_fetch_word())+",hl";
                                        break;
                                    case 3: // LD (Mmn), A
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = strWind(disasm_fetch_word())+",a";
                                        break;
                                }
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // LD A, (BC)
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "a,(bc)";
                                        break;
                                    case 1: // LD A, (DE)
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "a,(de)";
                                        break;
                                    case 2: // LD HL, (Mmn)
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "hl,"+strWind(disasm_fetch_word());
                                        break;
                                    case 3: // LD A, (Mmn)
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "a,"+strWind(disasm_fetch_word());
                                        break;
                                }
                                break;
                        }
                        break;
                    case 3:
                        switch (context.q) {
                            case 0: // INC rp[p]
                                disasm.instruction.opcode = "inc";
                                disasm.instruction.arguments = disasm_read_rp(context.p);
                                break;
                            case 1: // DEC rp[p]
                                disasm.instruction.opcode = "dec";
                                disasm.instruction.arguments = disasm_read_rp(context.p);
                                break;
                        }
                        break;
                    case 4: // INC r[y]
                        disasm.instruction.opcode = "inc";
                        disasm.instruction.arguments = disasm_read_reg(context.y);
                        break;
                    case 5: // DEC r[y]
                        disasm.instruction.opcode = "dec";
                        disasm.instruction.arguments = disasm_read_reg(context.y);
                        break;
                    case 6: // LD r[y], n
                        if (context.y == 7 && disasm.prefix) { // LD (IX/IY + d), IY/IX
                            disasm.instruction.opcode = "ld";
                            disasm.instruction.arguments = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+"),"+index_table[disasm.prefix ^ 1];
                            break;
                        }
                        break;
                    case 7:
                        if (disasm.prefix) {
                            if (context.q) { // LD (IX/IY + d), rp3[p]
                                disasm.instruction.opcode = "ld";
                                disasm.instruction.arguments = "("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+"),"+disasm_read_rp3(context.p);
                            } else { // LD rp3[p], (IX/IY + d)
                                disasm.instruction.opcode = "ld";
                                disasm.instruction.arguments = disasm_read_rp3(context.p)+",("+index_table[disasm.prefix]+strOffset(disasm_fetch_offset())+")";
                            }

                        } else {
                            disasm.instruction.opcode = rot_acc_table[context.y];
                        }
                        break;
                }
                break;
            case 1: // ignore prefixed prefixes
                if (context.z == context.y) {
                    switch (context.z) {
                        case 0: // .SIS
                            disasm.suffix = 1;
                            disasm.instruction.mode_suffix = ".sis ";
                            disasm.S = 1; disasm.IS = 1;
                            disasm.L = 0; disasm.IL = 0;
                            goto exit_loop;
                        case 1: // .LIS
                            disasm.suffix = 1;
                            disasm.instruction.mode_suffix = ".lis ";
                            disasm.S = 0; disasm.IS = 1;
                            disasm.L = 1; disasm.IL = 0;
                            goto exit_loop;
                        case 2: // .SIL
                            disasm.suffix = 1;
                            disasm.instruction.mode_suffix = ".sil ";
                            disasm.S = 1; disasm.IS = 0;
                            disasm.L = 0; disasm.IL = 1;
                            goto exit_loop;
                        case 3: // .LIL
                            disasm.suffix = 1;
                            disasm.instruction.mode_suffix = ".lil ";
                            disasm.S = 0; disasm.IS = 0;
                            disasm.L = 1; disasm.IL = 1;
                            goto exit_loop;
                        case 6: // HALT
                            disasm.instruction.opcode = "halt";
                            break;
                        case 4: // LD H, H
                            disasm.instruction.opcode = "ld";
                            disasm.instruction.arguments = index_h[disasm.prefix]+","+index_h[disasm.prefix];
                            break;
                        case 5: // LD L, L
                            disasm.instruction.opcode = "ld";
                            disasm.instruction.arguments = index_l[disasm.prefix]+","+index_l[disasm.prefix];
                            break;
                        case 7: // LD A, A
                            disasm.instruction.opcode = "ld";
                            disasm.instruction.opcode = "a,a";
                            break;
                        default:
                            abort();
                    }
                } else {
                    disasm_read_write_reg(context.z, context.y);
                }
                break;
            case 2: // ALU[y] r[z]
                disasm.instruction.opcode = alu_table[context.y];
                disasm.instruction.arguments = "a,"+disasm_read_reg(context.z);
                break;
            case 3:
                switch (context.z) {
                    case 0: // RET cc[y]
                        disasm.instruction.opcode = "ret";
                        disasm.instruction.arguments = cc_table[context.y];
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // POP rp2[p]
                                disasm.instruction.opcode = "pop";
                                disasm.instruction.arguments = disasm_read_rp2(context.p);
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // RET
                                        disasm.instruction.opcode = "ret";
                                        break;
                                    case 1: // EXX
                                        disasm.instruction.opcode = "exx";
                                        break;
                                    case 2: // JP (rr)
                                        disasm.instruction.opcode = "jp";
                                        disasm.instruction.arguments = "("+index_table[disasm.prefix]+")";
                                        break;
                                    case 3: // LD SP, INDEX
                                        disasm.instruction.opcode = "ld";
                                        disasm.instruction.arguments = "sp,"+index_table[disasm.prefix];
                                        break;
                                }
                                break;
                        }
                        break;
                    case 2: // JP cc[y], nn
                        disasm.instruction.opcode = "jp";
                        disasm.instruction.arguments = cc_table[context.y]+","+strW(disasm_fetch_word());
                        break;
                    case 3:
                        switch (context.y) {
                            case 0: // JP nn
                                disasm.instruction.opcode = "jp";
                                disasm.instruction.arguments = strW(disasm_fetch_word());
                                break;
                            case 1: // 0xCB prefixed opcodes
                                context.opcode = disasm_fetch_byte();
                                switch (context.x) {
                                    case 0: // rot[y] r[z]
                                        disasm.instruction.opcode = rot_table[context.y];
                                        disasm.instruction.arguments = disasm_read_reg(context.z);
                                        break;
                                    case 1: // BIT y, r[z]
                                        disasm.instruction.opcode = "bit";
                                        disasm.instruction.arguments = std::to_string(context.y)+","+disasm_read_reg(context.z);
                                        break;
                                    case 2: // RES y, r[z]
                                        disasm.instruction.opcode = "res";
                                        disasm.instruction.arguments = std::to_string(context.y)+","+disasm_read_reg(context.z);
                                        break;
                                    case 3: // SET y, r[z]
                                        disasm.instruction.opcode = "set";
                                        disasm.instruction.arguments = std::to_string(context.y)+","+disasm_read_reg(context.z);
                                        break;
                                }
                                break;
                            case 2: // OUT (n), A
                                disasm.instruction.opcode = "out";
                                disasm.instruction.arguments = strSind(disasm_fetch_byte())+",a";
                                break;
                            case 3: // IN A, (n)
                                disasm.instruction.opcode = "in";
                                disasm.instruction.arguments = "a,"+strSind(disasm_fetch_byte());
                                break;
                            case 4: // EX (SP), HL/I
                                disasm.instruction.opcode = "ex";
                                disasm.instruction.arguments = "(sp),"+index_table[disasm.prefix];
                                break;
                            case 5: // EX DE, HL
                                disasm.instruction.opcode = "ex";
                                disasm.instruction.arguments = "de,hl";
                                break;
                            case 6: // DI
                                disasm.instruction.opcode = "di";
                                break;
                            case 7: // EI
                                disasm.instruction.opcode = "ei";
                                continue;
                        }
                        break;
                    case 4: // CALL cc[y], nn
                        disasm.instruction.opcode = "call";
                        disasm.instruction.arguments = cc_table[context.y]+","+strW(disasm_fetch_word());
                        break;
                    case 5:
                        switch (context.q) {
                            case 0: // PUSH r2p[p]
                                disasm.instruction.opcode = "push";
                                disasm.instruction.arguments = disasm_read_rp2(context.p);
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // CALL nn
                                        disasm.instruction.opcode = "call";
                                        disasm.instruction.arguments = strW(disasm_fetch_word());
                                        break;
                                    case 1: // 0xDD prefixed opcodes
                                        disasm.prefix = 2;
                                        goto exit_loop;
                                    case 2: // 0xED prefixed opcodes
                                        disasm.prefix = 0; // ED cancels effect of DD/FD prefix
                                        context.opcode = disasm_fetch_byte();
                                        switch (context.x) {
                                            case 0:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP
                                                            disasm.instruction.opcode = "OPCODETRAP";
                                                        } else { // IN0 r[y], (n)
                                                            disasm.instruction.opcode = "in0";
                                                            disasm.instruction.arguments = disasm_read_reg(context.y)+","+strSind(disasm_fetch_byte());
                                                        }
                                                        break;
                                                     case 1:
                                                        if (context.y == 6) { // LD IY, (HL)
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = "iy,(hl)";
                                                        } else { // OUT0 (n), r[y]
                                                            disasm.instruction.opcode = "out0";
                                                            disasm.instruction.arguments = strSind(disasm_fetch_byte())+","+disasm_read_reg(context.y);
                                                        }
                                                        break;
                                                    case 2: // LEA rp3[p], IX + d
                                                    case 3: // LEA rp3[p], IY + d
                                                        if (context.q) { // OPCODETRAP
                                                            disasm.instruction.opcode = "OPCODETRAP";
                                                        } else {
                                                            disasm.prefix = context.z;
                                                            disasm.instruction.opcode = "lea";
                                                            disasm.instruction.arguments = disasm_read_rp3(context.p)+","+index_table[context.z]+strOffset(disasm_fetch_offset());
                                                        }
                                                        break;
                                                    case 4: // TST A, r[y]
                                                        disasm.instruction.opcode = "tst";
                                                        disasm.instruction.arguments = "a,"+disasm_read_reg(context.y);
                                                        break;
                                                    case 6:
                                                        if (context.y == 7) { // LD (HL), IY
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = "(hl),iy";
                                                            break;
                                                        }
                                                    case 5: // OPCODETRAP
                                                        disasm.instruction.opcode = "OPCODETRAP";
                                                        break;
                                                    case 7:
                                                        if (context.q) { // LD (HL), rp3[p]
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = "(hl),"+disasm_read_rp3(context.p);
                                                        } else { // LD rp3[p], (HL)
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = disasm_read_rp3(context.p)+",(hl)";
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 1:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instruction.opcode = "OPCODETRAP";
                                                        } else { // IN r[y], (BC)
                                                            disasm.instruction.opcode = "in";
                                                            disasm.instruction.arguments = disasm_read_reg(context.y)+",(bc)";
                                                        }
                                                        break;
                                                    case 1:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instruction.opcode = "OPCODETRAP";
                                                        } else { // OUT (BC), r[y]
                                                            disasm.instruction.opcode = "out";
                                                            disasm.instruction.arguments = "(bc),"+disasm_read_reg(context.y);
                                                        }
                                                        break;
                                                    case 2:
                                                        if (context.q == 0) { // SBC HL, rp[p]
                                                            disasm.instruction.opcode = "sbc";
                                                            disasm.instruction.arguments = "hl,"+disasm_read_rp(context.p);
                                                        } else { // ADC HL, rp[p]
                                                            disasm.instruction.opcode = "adc";
                                                            disasm.instruction.arguments = "hl,"+disasm_read_rp(context.p);
                                                        }
                                                        break;
                                                    case 3:
                                                        if (context.q == 0) { // LD (nn), rp[p]
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = strWind(disasm_fetch_word())+","+disasm_read_rp(context.p);
                                                        } else { // LD rp[p], (nn)
                                                            disasm.instruction.opcode = "ld";
                                                            disasm.instruction.arguments = disasm_read_rp(context.p)+","+strWind(disasm_fetch_word());
                                                        }
                                                        break;
                                                    case 4:
                                                        if (context.q == 0) {
                                                            switch (context.p) {
                                                                case 0:  // NEG
                                                                    disasm.instruction.opcode = "neg";
                                                                    break;
                                                                case 1:  // LEA IX, IY + d
                                                                    disasm.instruction.opcode = "lea";
                                                                    disasm.instruction.arguments = "ix,iy"+strOffset(disasm_fetch_byte());
                                                                    break;
                                                                case 2:  // TST A, n
                                                                    disasm.instruction.opcode = "tst";
                                                                    disasm.instruction.arguments = "a,"+strS(disasm_fetch_byte());
                                                                    break;
                                                                case 3:  // TSTIO n
                                                                    disasm.instruction.opcode = "tstio";
                                                                    break;
                                                            }
                                                        }
                                                        else { // MLT rp[p]
                                                            disasm.instruction.opcode = "mlt";
                                                            disasm.instruction.arguments = rp_table[context.p];
                                                            break;
                                                        }
                                                        break;
                                                    case 5:
                                                        switch (context.y) {
                                                            case 0: // RETN
                                                                disasm.instruction.opcode = "retn";
                                                                break;
                                                            case 1: // RETI
                                                                disasm.instruction.opcode = "reti";
                                                                break;
                                                            case 2: // LEA IY, IX + d
                                                                disasm.instruction.opcode = "lea";
                                                                disasm.instruction.arguments = "iy,ix"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 3:
                                                            case 6: // OPCODETRAP
                                                                disasm.instruction.opcode = "OPCODETRAP";
                                                                break;
                                                            case 4: // PEA IX + d
                                                                disasm.instruction.opcode = "pea";
                                                                disasm.instruction.arguments = "ix"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 5: // LD MB, A
                                                                if (disasm.IL) {
                                                                    disasm.instruction.opcode = "ld";
                                                                    disasm.instruction.arguments = "mb,a";
                                                                } else { // OPCODETRAP
                                                                    disasm.instruction.opcode = "OPCODETRAP";
                                                                }
                                                                break;
                                                            case 7: // STMIX
                                                                disasm.instruction.opcode = "stmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 6: // IM im[y]
                                                        switch (context.y) {
                                                            case 0:
                                                            case 2:
                                                            case 3: // IM im[y]
                                                                disasm.instruction.opcode = "im";
                                                                disasm.instruction.arguments = im_table[context.y];
                                                                break;
                                                            case 1: // OPCODETRAP
                                                                disasm.instruction.opcode = "OPCODETRAP";
                                                                break;
                                                            case 4: // PEA IY + d
                                                                disasm.instruction.opcode = "pea";
                                                                disasm.instruction.arguments = "iy"+strOffset(disasm_fetch_offset());
                                                                break;
                                                            case 5: // LD A, MB
                                                                if (disasm.IL) {
                                                                    disasm.instruction.opcode = "ld";
                                                                    disasm.instruction.arguments = "a,mb";
                                                                } else { // OPCODETRAP
                                                                    disasm.instruction.opcode = "OPCODETRAP";
                                                                }
                                                                break;
                                                            case 6: // SLP
                                                                disasm.instruction.arguments = "slp";
                                                                break;
                                                            case 7: // RSMIX
                                                                disasm.instruction.opcode = "rsmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 7:
                                                        switch (context.y) {
                                                            case 0: // LD I, A
                                                                disasm.instruction.opcode = "ld";
                                                                disasm.instruction.arguments = "i,a";
                                                                break;
                                                            case 1: // LD R, A
                                                                disasm.instruction.opcode = "ld";
                                                                disasm.instruction.arguments = "r,a";
                                                                break;
                                                            case 2: // LD A, I
                                                                disasm.instruction.opcode = "ld";
                                                                disasm.instruction.arguments = "a,i";
                                                                break;
                                                            case 3: // LD A, R
                                                                disasm.instruction.opcode = "ld";
                                                                disasm.instruction.arguments = "a,r";
                                                                break;
                                                            case 4: // RRD
                                                                disasm.instruction.opcode = "rrd";
                                                                break;
                                                            case 5: // RLD
                                                                disasm.instruction.opcode = "rld";
                                                                break;
                                                            default: // OPCODETRAP
                                                                disasm.instruction.opcode = "OPCODETRAP";
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 2:
                                                if (context.y >= 0 && context.z <= 4) { // bli[y,z]
                                                    disasm_bli(context.y, context.z);
                                                } else { // OPCODETRAP
                                                    disasm.instruction.opcode = "OPCODETRAP";
                                                }
                                                break;
                                            case 3:  // There are only a few of these, so a simple switch for these shouldn't matter too much
                                                switch(context.opcode) {
                                                    case 0xC2: // INIRX
                                                        disasm.instruction.opcode = "inirx";
                                                        break;
                                                    case 0xC3: // OTIRX
                                                        disasm.instruction.opcode = "otirx";
                                                        break;
                                                    case 0xC7: // LD I, HL
                                                        disasm.instruction.opcode = "ld";
                                                        disasm.instruction.arguments = "i,hl";
                                                        break;
                                                    case 0xD7: // LD HL, I
                                                        disasm.instruction.opcode = "ld";
                                                        disasm.instruction.arguments = "hl,i";
                                                        break;
                                                    case 0xCA: // INDRX
                                                        disasm.instruction.opcode = "indrx";
                                                        break;
                                                    case 0xCB: // OTDRX
                                                        disasm.instruction.opcode = "otdrx";
                                                        break;
                                                    case 0xEE: // flash erase
                                                        disasm.instruction.opcode = "FLASH_ERASE";
                                                        break;
                                                    default:   // OPCODETRAP
                                                        disasm.instruction.opcode = "OPCODETRAP";
                                                        break;
                                                }
                                                break;
                                            default: // OPCODETRAP
                                                disasm.instruction.opcode = "OPCODETRAP";
                                                break;
                                        }
                                        break;
                                    case 3: // 0xFD prefixed opcodes
                                        disasm.prefix = 3;
                                        goto exit_loop;
                                }
                                break;
                        }
                        break;
                    case 6: // alu[y] n
                        disasm.instruction.opcode = alu_table[context.y];
                        disasm.instruction.arguments = "a,"+strS(disasm_fetch_byte());
                        break;
                    case 7: // RST y*8
                        disasm.instruction.opcode = "rst";
                        disasm.instruction.arguments = strS(context.y << 3);
                        break;
                }
                break;
        }
        disasm.suffix = disasm.prefix = 0;
exit_loop:
      continue;
    } while (disasm.prefix || disasm.suffix);
}

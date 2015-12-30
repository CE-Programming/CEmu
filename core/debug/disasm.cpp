#include <string>

#include "disasm.h"

disasm_state_t disasm;

static void disasm_alu(int i, uint8_t v) {
    switch (i) {
        case 0: // ADD A, v
            disasm.instruction = "add a,";
            break;
        case 1: // ADC A, v
            disasm.instruction = "adc a,";
            break;
        case 2: // SUB A,v
            disasm.instruction = "sub a,";
            break;
        case 3: // SBC A,v
            disasm.instruction = "sbc a,";
            break;
        case 4: // AND A,v
            disasm.instruction = "and a,";
            break;
        case 5: // XOR A,v
            disasm.instruction = "xor a,";
            break;
        case 6: // OR A,v
            disasm.instruction = "or a,";
            break;
        case 7: // CP A,v
            disasm.instruction = "cp a,";
            break;
    }
}

static void disasm_rot(int y, int z) {
    switch (y) {
        case 0: // RLC value[z]
            disasm.instruction = "rlc ";
            break;
        case 1: // RRC value[z]
            disasm.instruction = "rrc ";
            break;
        case 2: // RL value[z]
            disasm.instruction = "rl ";
            break;
        case 3: // RR value[z]
            disasm.instruction = "rr";
            break;
        case 4: // SLA value[z]
            disasm.instruction = "sla ";
            break;
        case 5: // SRA value[z]
            disasm.instruction = "sra ";
            break;
        case 6: // OPCODETRAP
            disasm.instruction = "OPCODETRAP";
            return;
        case 7: // SRL value[z]
            disasm.instruction = "srl ";
            break;
        default:
            abort();
    }
    // TODO: Add value[z]
}

static void disasm_rot_acc(int y)
{
    switch (y) {
        case 0: // RLCA
            disasm.instruction = "rlca";
            break;
        case 1: // RRCA
            disasm.instruction = "rrca";
            break;
        case 2: // RLA
            disasm.instruction = "rla";
            break;
        case 3: // RRA
            disasm.instruction = "rra";
            break;
        case 4: // DAA
            disasm.instruction = "daa";
            break;
        case 5: // CPL
            disasm.instruction = "cpl";
            break;
        case 6: // SCF
            disasm.instruction = "scf";
            break;
        case 7: // CCF
            disasm.instruction = "ccf";
            break;
    }
}

static void disasm_bli(int y, int z) {
    switch (y) {
        case 0:
            switch (z) {
                case 2: // INIM
                    disasm.instruction = "inim";
                    break;
                case 3: // OTIM
                    disasm.instruction = "otim";
                    break;
                case 4: // INI2
                    disasm.instruction = "ini2";
                    break;
            }
            break;
        case 1:
            switch (z) {
                case 2: // INDM
                    disasm.instruction = "indm";
                    break;
                case 3: // OTDM
                    disasm.instruction = "otdm";
                    break;
                case 4: // IND2
                    disasm.instruction = "ind2";
                    break;
            }
            break;
        case 2:
            switch (z) {
                case 2: // INIMR
                    disasm.instruction = "inimr";
                    break;
                case 3: // OTIMR
                    disasm.instruction = "otimr";
                    break;
                case 4: // INI2R
                    disasm.instruction = "ini2r";
                    break;
            }
            break;
        case 3:
            switch (z) {
                case 2: // INDMR
                    disasm.instruction = "indmr";
                    break;
                case 3: // OTDMR
                    disasm.instruction = "otdmr";
                    break;
                case 4: // IND2R
                    disasm.instruction = "ind2r";
                    break;
            }
            break;
        case 4:
            switch (z) {
                case 0: // LDI
                    disasm.instruction = "ldi";
                    break;
                case 1: // CPI
                    disasm.instruction = "cpi";
                    break;
                case 2: // INI
                    disasm.instruction = "ini";
                    break;
                case 3: // OUTI
                    disasm.instruction = "outi";
                    break;
                case 4: // OUTI2
                    disasm.instruction = "outi2";
                    break;
            }
            break;
        case 5:
            switch (z) {
                case 0: // LDD
                    disasm.instruction = "ldd";
                    break;
                case 1: // CPD
                   disasm.instruction = "cpd";
                    break;
                case 2: // IND
                   disasm.instruction = "ind";
                    break;
                case 3: // OUTD
                    disasm.instruction = "outd";
                    break;
                case 4: // OUTD2
                    disasm.instruction = "outd2";
                    break;
            }
            break;
        case 6:
            switch (z) {
                case 0: // LDIR
                    disasm.instruction = "ldir";
                    break;
                case 1: // CPIR
                    disasm.instruction = "cpir";
                    break;
                case 2: // INIR
                    disasm.instruction = "inir";
                    break;
                case 3: // OTIR
                    disasm.instruction = "otir";
                    break;
                case 4: // OTI2R
                    disasm.instruction = "oti2r";
                    break;
            }
            break;
        case 7:
            switch (z) {
                case 0: // LDDR
                    disasm.instruction = "lddr";
                    break;
                case 1: // CPDR
                    disasm.instruction = "cpdr";
                    break;
                case 2: // INDR
                    disasm.instruction = "indr";
                    break;
                case 3: // OTDR
                    disasm.instruction = "otdr";
                    break;
                case 4: // OTD2R
                    disasm.instruction = "otdr2";
                    break;
            }
            break;
    }
}

void disassembleInstruction(void) {

    bool done = false;

    disasm.new_address = disasm.base_address;

    union {
        uint8_t opcode;
        struct {
            uint8_t z : 3;
            uint8_t y : 3;
            uint8_t x : 2;
        };
        struct {
            uint8_t r : 1;
            uint8_t   : 2;
            uint8_t q : 1;
            uint8_t p : 2;
        };
    } context;

    while (!done || disasm.prefix || disasm.suffix) {
        // fetch opcode
        context.opcode = disasm.data[disasm.new_address++];

        switch (context.x) {
            case 0:
                switch (context.z) {
                    case 0:
                        switch (context.y) {
                            case 0:  // NOP
                                disasm.instruction = "nop";
                                break;
                            case 1:  // EX af,af'
                                disasm.instruction = "ex af,af'";
                                break;
                            case 2: // DJNZ d
                                disasm.instruction = "djnz ";
                                break;
                            case 3: // JR d
                                disasm.instruction = "jr ";
                                break;
                            case 4:
                            case 5:
                            case 6:
                            case 7: // JR cc[y-4], d
                                disasm.instruction = "jr ";
                                break;
                        }
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // LD rr, Mmn
                                if (context.p == 3 && disasm.prefix) { // LD IY/IX, (IX/IY + d)
                                    disasm.instruction = "ld ";
                                    break;
                                }
                                disasm.instruction = "ld ";
                                break;
                            case 1: // ADD HL,rr
                                disasm.instruction = "add hl,";
                                break;
                        }
                        break;
                    case 2:
                        switch (context.q) {
                            case 0:
                                switch (context.p) {
                                    case 0: // LD (BC), A
                                        disasm.instruction = "ld (bc),a";
                                        break;
                                    case 1: // LD (DE), A
                                        disasm.instruction = "ld (de),a";
                                        break;
                                    case 2: // LD (Mmn), HL
                                        disasm.instruction = "ld";
                                        break;
                                    case 3: // LD (Mmn), A
                                        disasm.instruction = "ld";
                                        break;
                                }
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // LD A, (BC)
                                        disasm.instruction = "ld a,(bc)";
                                        break;
                                    case 1: // LD A, (DE)
                                        disasm.instruction = "ld a,(de)";
                                        break;
                                    case 2: // LD HL, (Mmn)
                                        disasm.instruction = "ld hl,";
                                        break;
                                    case 3: // LD A, (Mmn)
                                        disasm.instruction = "ld a,";
                                        break;
                                }
                                break;
                        }
                        break;
                    case 3:
                        switch (context.q) {
                            case 0: // INC rp[p]
                                disasm.instruction = "inc ";
                                break;
                            case 1: // DEC rp[p]
                                disasm.instruction = "dec ";
                                break;
                        }
                        break;
                    case 4: // INC r[y]
                        disasm.instruction = "inc ";
                        break;
                    case 5: // DEC r[y]
                        disasm.instruction = "dec ";
                        break;
                    case 6: // LD r[y], n
                        if (context.y == 7 && disasm.prefix) { // LD (IX/IY + d), IY/IX
                            disasm.instruction = "ld ";
                            break;
                        }
                        disasm.instruction = "ld ";
                        break;
                    case 7:
                        if (disasm.prefix) {
                            if (context.q) { // LD (IX/IY + d), rp3[p]
                                disasm.instruction = "ld ";
                            } else { // LD rp3[p], (IX/IY + d)
                                disasm.instruction = "ld ";
                            }
                        } else {
                            disasm_rot_acc(context.y);
                        }
                        break;
                }
                break;
            case 1: // ignore prefixed prefixes
                if (context.z == context.y) {
                    switch (context.z) {
                        case 0: // .SIS
                            disasm.suffix = 1;
                            disasm.mode_suffix = ".sis";
                            goto exit_loop;
                        case 1: // .LIS
                            disasm.suffix = 1;
                            disasm.mode_suffix = ".lis";
                            goto exit_loop;
                        case 2: // .SIL
                            disasm.suffix = 1;
                            disasm.mode_suffix = ".sil";
                            goto exit_loop;
                        case 3: // .LIL
                            disasm.suffix = 1;
                            disasm.mode_suffix = ".lil";
                            goto exit_loop;
                        case 6: // HALT
                            disasm.instruction = "halt";
                            break;
                        case 4: // LD H, H
                            disasm.instruction = "ld h,h";
                            break;
                        case 5: // LD L, L
                            disasm.instruction = "ld l,l";
                            break;
                        case 7: // LD A, A
                            disasm.instruction = "ld a,a";
                            break;
                        default:
                            abort();
                    }
                } else {
                    disasm.instruction = "ld ";
                    //cpu_read_write_reg(context.z, context.y);
                }
                break;
            case 2: // ALU[y] r[z]
                //disasm_alu(context.y, cpu_read_reg(context.z));
                break;
            case 3:
                switch (context.z) {
                    case 0: // RET cc[y]
                        disasm.instruction = "ret ";
                        break;
                    case 1:
                        switch (context.q) {
                            case 0: // POP rp2[p]
                                disasm.instruction = "pop ";
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // RET
                                        disasm.instruction = "ret";
                                        break;
                                    case 1: // EXX
                                        disasm.instruction = "exx";
                                        break;
                                    case 2: // JP (rr)
                                        disasm.instruction = "jp ";
                                        break;
                                    case 3: // LD SP, HL
                                        disasm.instruction = "ld sp,hl";
                                        break;
                                }
                                break;
                        }
                        break;
                    case 2: // JP cc[y], nn
                        disasm.instruction = "jp ";
                        break;
                    case 3:
                        switch (context.y) {
                            case 0: // JP nn
                                disasm.instruction = "jp";
                                break;
                            case 1: // 0xCB prefixed opcodes
                                context.opcode = disasm.data[disasm.new_address++];
                                switch (context.x) {
                                    case 0: // rot[y] r[z]
                                        disasm_rot(context.y, context.z);
                                        break;
                                    case 1: // BIT y, r[z]
                                        disasm.instruction = "bit ";
                                        break;
                                    case 2: // RES y, r[z]
                                        disasm.instruction = "res ";
                                        break;
                                    case 3: // SET y, r[z]
                                        disasm.instruction = "set ";
                                        break;
                                }
                                break;
                            case 2: // OUT (n), A
                                disasm.instruction = "out ";
                                break;
                            case 3: // IN A, (n)
                                disasm.instruction = "in a,";
                                break;
                            case 4: // EX (SP), HL/I
                                disasm.instruction = "ex (sp),";
                                break;
                            case 5: // EX DE, HL
                                disasm.instruction = "ex de,hl";
                                break;
                            case 6: // DI
                                disasm.instruction = "di";
                                break;
                            case 7: // EI
                                disasm.instruction = "ei";
                                continue;
                        }
                        break;
                    case 4: // CALL cc[y], nn
                        disasm.instruction = "call ";
                        break;
                    case 5:
                        switch (context.q) {
                            case 0: // PUSH r2p[p]
                                disasm.instruction = "push ";
                                break;
                            case 1:
                                switch (context.p) {
                                    case 0: // CALL nn
                                        disasm.instruction = "call ";
                                        break;
                                    case 1: // 0xDD prefixed opcodes
                                        disasm.prefix = 2;
                                        goto exit_loop;
                                    case 2: // 0xED prefixed opcodes
                                        disasm.prefix = 0; // ED cancels effect of DD/FD prefix
                                        context.opcode = disasm.data[disasm.new_address++];
                                        switch (context.x) {
                                            case 0:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP
                                                            disasm.instruction = "OPCODETRAP";
                                                        } else { // IN0 r[y], (n)
                                                            disasm.instruction = "in0 ";
                                                        }
                                                        break;
                                                     case 1:
                                                        if (context.y == 6) { // LD IY, (HL)
                                                            disasm.instruction = "ld iy,(hl)";
                                                        } else { // OUT0 (n), r[y]
                                                            disasm.instruction = "out0 ";
                                                        }
                                                        break;
                                                    case 2: // LEA rp3[p], IX
                                                    case 3: // LEA rp3[p], IY
                                                        if (context.q) { // OPCODETRAP
                                                            disasm.instruction = "OPCODETRAP";
                                                        } else {
                                                            disasm.prefix = context.z;
                                                            disasm.instruction = "lea ";
                                                        }
                                                        break;
                                                    case 4: // TST A, r[y]
                                                        disasm.instruction = "tst a,";
                                                        break;
                                                    case 6:
                                                        if (context.y == 7) { // LD (HL), IY
                                                            disasm.instruction = "ld (hl),iy";
                                                            break;
                                                        }
                                                    case 5: // OPCODETRAP
                                                        disasm.instruction = "OPCODETRAP";
                                                        break;
                                                    case 7:
                                                        disasm.prefix = 2;
                                                        if (context.q) { // LD (HL), rp3[p]
                                                            disasm.instruction = "ld (hl),";
                                                        } else { // LD rp3[p], (HL)
                                                            disasm.instruction = "ld ";
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 1:
                                                switch (context.z) {
                                                    case 0:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instruction = "OPCODETRAP";
                                                        } else { // IN r[y], (BC)
                                                            disasm.instruction = "in ";
                                                        }
                                                        break;
                                                    case 1:
                                                        if (context.y == 6) { // OPCODETRAP (ADL)
                                                            disasm.instruction = "OPCODETRAP";
                                                        } else { // OUT (BC), r[y]
                                                            disasm.instruction = "out (bc),";
                                                        }
                                                        break;
                                                    case 2:
                                                        if (context.q == 0) { // SBC HL, rp[p]
                                                            disasm.instruction = "sbc hl,";
                                                        } else { // ADC HL, rp[p]
                                                            disasm.instruction = "adc hl,";
                                                        }
                                                        break;
                                                    case 3:
                                                        if (context.q == 0) { // LD (nn), rp[p]
                                                            disasm.instruction = "ld ";
                                                        } else { // LD rp[p], (nn)
                                                            disasm.instruction = "ld ";
                                                        }
                                                        break;
                                                    case 4:
                                                        if (context.q == 0) {
                                                            switch (context.p) {
                                                                case 0:  // NEG
                                                                    disasm.instruction = "neg";
                                                                    break;
                                                                case 1:  // LEA IX, IY + d
                                                                    disasm.prefix = 3;
                                                                    disasm.instruction = "lea ix,iy+";
                                                                    break;
                                                                case 2:  // TST A, n
                                                                    disasm.instruction = "tst a,";
                                                                    break;
                                                                case 3:  // TSTIO n
                                                                    disasm.instruction = "tstio";
                                                                    break;
                                                            }
                                                        }
                                                        else { // MLT rp[p]
                                                            disasm.instruction = "mlt";
                                                            break;
                                                        }
                                                        break;
                                                    case 5:
                                                        switch (context.y) {
                                                            case 0: // RETN
                                                                disasm.instruction = "retn";
                                                                break;
                                                            case 1: // RETI
                                                                disasm.instruction = "reti";
                                                                break;
                                                            case 2: // LEA IY, IX + d
                                                                disasm.prefix = 2;
                                                                disasm.instruction = "adc hl,";
                                                                break;
                                                            case 3:
                                                            case 6: // OPCODETRAP
                                                                disasm.instruction = "OPCODETRAP";
                                                                break;
                                                            case 4: // PEA IX + d
                                                                disasm.instruction = "pea ix+";
                                                                break;
                                                            case 5: // LD MB, A
                                                                if (disasm.mode) {
                                                                    disasm.instruction = "ld mb,a";
                                                                } else { // OPCODETRAP
                                                                    disasm.instruction = "OPCODETRAP";
                                                                }
                                                                break;
                                                            case 7: // STMIX
                                                                disasm.instruction = "stmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 6: // IM im[y]
                                                        switch (context.y) {
                                                            case 0:
                                                            case 2:
                                                            case 3: // IM im[y]
                                                                disasm.instruction = "im ";
                                                                break;
                                                            case 1: // OPCODETRAP
                                                                disasm.instruction = "OPCODETRAP";
                                                                break;
                                                            case 4: // PEA IY + d
                                                                disasm.instruction = "pea iy+";
                                                                break;
                                                            case 5: // LD A, MB
                                                                if (disasm.mode) {
                                                                    disasm.instruction = "ld a,mb";
                                                                } else { // OPCODETRAP
                                                                    disasm.instruction = "OPCODETRAP";
                                                                }
                                                                break;
                                                            case 6: // SLP -- NOT IMPLEMENTED
                                                                break;
                                                            case 7: // RSMIX
                                                                disasm.instruction = "rsmix";
                                                                break;
                                                        }
                                                        break;
                                                    case 7:
                                                        switch (context.y) {
                                                            case 0: // LD I, A
                                                                disasm.instruction = "ld i,a";
                                                                break;
                                                            case 1: // LD R, A
                                                                disasm.instruction = "ld r,a";
                                                                break;
                                                            case 2: // LD A, I
                                                                disasm.instruction = "ld a,i";
                                                                break;
                                                            case 3: // LD A, R
                                                                disasm.instruction = "ld a,r";
                                                                break;
                                                            case 4: // RRD
                                                                disasm.instruction = "rrd";
                                                                break;
                                                            case 5: // RLD
                                                                disasm.instruction = "rld";
                                                                break;
                                                            default: // OPCODETRAP
                                                                disasm.instruction = "OPCODETRAP";
                                                                break;
                                                        }
                                                        break;
                                                }
                                                break;
                                            case 2:
                                                if (context.y >= 0 && context.z <= 4) { // bli[y,z]
                                                    disasm_bli(context.y, context.z);
                                                } else { // OPCODETRAP
                                                    disasm.instruction = "OPCODETRAP";
                                                }
                                                break;
                                            case 3:  // There are only a few of these, so a simple switch for these shouldn't matter too much
                                                switch(context.opcode) {
                                                    case 0xC2: // INIRX
                                                        disasm.instruction = "inirx";
                                                        break;
                                                    case 0xC3: // OTIRX
                                                        disasm.instruction = "otirx";
                                                        break;
                                                    case 0xC7: // LD I, HL
                                                        disasm.instruction = "ld i,hl";
                                                        break;
                                                    case 0xD7: // LD HL, I
                                                        disasm.instruction = "ld hl,i";
                                                        break;
                                                    case 0xCA: // INDRX
                                                        disasm.instruction = "indrx";
                                                        break;
                                                    case 0xCB: // OTDRX
                                                        disasm.instruction = "otdrx";
                                                        break;
                                                    case 0xEE: // flash erase
                                                        disasm.instruction = "FLASH_ERASE";
                                                        break;
                                                    default:   // OPCODETRAP
                                                        disasm.instruction = "OPCODETRAP";
                                                        break;
                                                }
                                                break;
                                            default: // OPCODETRAP
                                                disasm.instruction = "OPCODETRAP";
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
                        disasm_alu(context.y, disasm.data[disasm.new_address++]);
                        break;
                    case 7: // RST y*8
                        disasm.instruction = "rst ";
                        break;
                }
                break;
        }
exit_loop:
        continue;
    }
}

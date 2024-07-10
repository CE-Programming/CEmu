/*
 * Copyright (c) 2014 The KnightOS Group
 * Modified to support the eZ80 processor by CEmu developers
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated
 * documentation files (the "Software"), to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software,
 * and to permit persons to whom the Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all copies or substantial portions
 * of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED
 * TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
*/

#include "cpu.h"
#include "emu.h"
#include "mem.h"
#include "bus.h"
#include "atomics.h"
#include "defines.h"
#include "control.h"
#include "registers.h"
#include "schedule.h"
#include "interrupt.h"
#include "debug/debug.h"

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

/* Global CPU state */
eZ80cpu_t cpu;

typedef struct eZ80atomics {
    _Atomic(uint8_t) signals;
} eZ80atomics_t;

static eZ80atomics_t cpu_atomics;

static void cpu_clear_context(void) {
    cpu.PREFIX = cpu.SUFFIX = 0;
    cpu.L = cpu.ADL;
    cpu.IL = cpu.ADL;
}

static void cpu_inst_start(void) {
    cpu_clear_context();
#ifdef DEBUG_SUPPORT
    debug_inst_start();
#endif
}

uint32_t cpu_address_mode(uint32_t address, bool mode) {
    if (mode) {
        return address & 0xFFFFFF;
    }
    return (cpu.registers.MBASE << 16) | (address & 0xFFFF);
}
static void cpu_prefetch(uint32_t address, bool mode) {
    cpu.ADL = mode;
    cpu.registers.PC = cpu_address_mode(address, mode);
    cpu.prefetch = mem_read_cpu(cpu.registers.PC, true);
}
static uint8_t cpu_fetch_byte(void) {
    uint8_t value;
#ifdef DEBUG_SUPPORT
    debug_inst_fetch();
#endif
    value = cpu.prefetch;
    cpu_prefetch(cpu.registers.PC + 1, cpu.ADL);
    return value;
}
static void cpu_prefetch_discard(void) {
    mem_read_cpu(cpu_address_mode(cpu.registers.PC + 1, cpu.ADL), true);
}
static int8_t cpu_fetch_offset(void) {
    return (int8_t)cpu_fetch_byte();
}
static uint32_t cpu_fetch_word(void) {
    uint32_t value = cpu_fetch_byte();
    value |= cpu_fetch_byte() << 8;
    if (cpu.IL) {
        value |= cpu_fetch_byte() << 16;
    }
    return value;
}
static uint32_t cpu_fetch_word_no_prefetch(void) {
    uint32_t value = cpu_fetch_byte();
    value |= cpu.prefetch << 8;
    if (cpu.IL) {
        cpu_fetch_byte();
        value |= cpu.prefetch << 16;
    }
    cpu.registers.PC++;
    return value;
}

static uint8_t cpu_read_byte(uint32_t address) {
    uint32_t cpuAddress = cpu_address_mode(address, cpu.L);
    return mem_read_cpu(cpuAddress, false);
}
static void cpu_write_byte(uint32_t address, uint8_t value) {
    uint32_t cpuAddress = cpu_address_mode(address, cpu.L);
    mem_write_cpu(cpuAddress, value);
}

static uint32_t cpu_read_word(uint32_t address) {
    uint32_t value = cpu_read_byte(address);
    value |= cpu_read_byte(address + 1) << 8;
    if (cpu.L) {
        value |= cpu_read_byte(address + 2) << 16;
    }
    return value;
}
static void cpu_write_word(uint32_t address, uint32_t value) {
    cpu_write_byte(address, value);
    cpu_write_byte(address + 1, value >> 8);
    if (cpu.L) {
        cpu_write_byte(address + 2, value >> 16);
    }
}

static uint8_t cpu_pop_byte_mode(bool mode) {
    return mem_read_cpu(cpu_address_mode(cpu.registers.stack[mode].hl++, mode), false);
}
static uint8_t cpu_pop_byte(void) {
    return cpu_pop_byte_mode(cpu.L);
}
static void cpu_push_byte_mode(uint8_t value, bool mode) {
    mem_write_cpu(cpu_address_mode(--cpu.registers.stack[mode].hl, mode), value);
}
static void cpu_push_byte(uint8_t value) {
    cpu_push_byte_mode(value, cpu.L);
}

static void cpu_push_word(uint32_t value) {
    if (cpu.L) {
        cpu_push_byte(value >> 16);
    }
    cpu_push_byte(value >> 8);
    cpu_push_byte(value);
}

static uint32_t cpu_pop_word(void) {
    uint32_t value = cpu_pop_byte();
    value |= cpu_pop_byte() << 8;
    if (cpu.L) {
        value |= cpu_pop_byte() << 16;
    }
    return value;
}

static uint8_t cpu_read_in(uint16_t pio) {
    if (unprivileged_code()) {
        return 0; /* in returns 0 in unprivileged code */
    }
    return port_read_byte(pio);
}

static void cpu_write_out(uint16_t pio, uint8_t value) {
    if (unprivileged_code()) {
        control.protectionStatus |= 2;
        gui_console_printf("[CEmu] NMI reset cause by an out instruction in unpriviledged code.\n");
        cpu_nmi();
    }
    port_write_byte(pio, value);
}

static uint32_t cpu_read_sp(void) {
    return cpu.registers.stack[cpu.L].hl;
}
static void cpu_write_sp(uint32_t value) {
    cpu.registers.stack[cpu.L].hl = value;
}

static uint8_t cpu_read_index_low(void) {
    return cpu.registers.index[cpu.PREFIX].l;
}
static void cpu_write_index_low(uint8_t value) {
    cpu.registers.index[cpu.PREFIX].l = value;
}

static uint8_t cpu_read_index_high(void) {
    return cpu.registers.index[cpu.PREFIX].h;
}
static void cpu_write_index_high(uint8_t value) {
    cpu.registers.index[cpu.PREFIX].h = value;
}

static uint32_t cpu_read_index(void) {
    return cpu.registers.index[cpu.PREFIX].hl;
}
static void cpu_write_index(uint32_t value) {
    cpu.registers.index[cpu.PREFIX].hl = value;
}
static void cpu_write_index_partial_mode(uint32_t value) {
    value = cpu_mask_mode(value, cpu.L);
    if (cpu.L) {
        cpu.registers.index[cpu.PREFIX].hl = value;
    } else {
        cpu.registers.index[cpu.PREFIX].hls = value;
    }
}

static uint32_t cpu_read_other_index(void) {
    return cpu.registers.index[cpu.PREFIX ^ 1].hl;
}
static void cpu_write_other_index(uint32_t value) {
    cpu.registers.index[cpu.PREFIX ^ 1].hl = value;
}

static uint32_t cpu_index_address(void) {
    uint32_t value = cpu_read_index();
    if (cpu.PREFIX) {
        value += cpu_fetch_offset();
    }
    return cpu_mask_mode(value, cpu.L);
}

static uint8_t cpu_read_reg(int i) {
    uint8_t value;
    switch (i) {
        case 0: value = cpu.registers.B; break;
        case 1: value = cpu.registers.C; break;
        case 2: value = cpu.registers.D; break;
        case 3: value = cpu.registers.E; break;
        case 4: value = cpu_read_index_high(); break;
        case 5: value = cpu_read_index_low(); break;
        case 6: value = cpu_read_byte(cpu_index_address()); break;
        case 7: value = cpu.registers.A; break;
        default: unreachable();
    }
    return value;
}
static void cpu_write_reg(int i, uint8_t value) {
    switch (i) {
        case 0: cpu.registers.B = value; break;
        case 1: cpu.registers.C = value; break;
        case 2: cpu.registers.D = value; break;
        case 3: cpu.registers.E = value; break;
        case 4: cpu_write_index_high(value); break;
        case 5: cpu_write_index_low(value); break;
        case 6: cpu_write_byte(cpu_index_address(), value); break;
        case 7: cpu.registers.A = value; break;
        default: unreachable();
    }
}
static void cpu_read_write_reg(int read, int write) {
    uint8_t value;
    int old_prefix = cpu.PREFIX;
    cpu.PREFIX = write != 6 ? old_prefix : 0;
    value = cpu_read_reg(read);
    cpu.PREFIX = read  != 6 ? old_prefix : 0;
    cpu_write_reg(write, value);
}

static uint8_t cpu_read_reg_prefetched(int i, uint32_t address) {
    uint8_t value;
    switch (i) {
        case 0: value = cpu.registers.B; break;
        case 1: value = cpu.registers.C; break;
        case 2: value = cpu.registers.D; break;
        case 3: value = cpu.registers.E; break;
        case 4: value = cpu_read_index_high(); break;
        case 5: value = cpu_read_index_low(); break;
        case 6: value = cpu_read_byte(address); break;
        case 7: value = cpu.registers.A; break;
        default: unreachable();
    }
    return value;
}
static void cpu_write_reg_prefetched(int i, uint32_t address, uint8_t value) {
    switch (i) {
        case 0: cpu.registers.B = value; break;
        case 1: cpu.registers.C = value; break;
        case 2: cpu.registers.D = value; break;
        case 3: cpu.registers.E = value; break;
        case 4: cpu_write_index_high(value); break;
        case 5: cpu_write_index_low(value); break;
        case 6: cpu_write_byte(address, value); break;
        case 7: cpu.registers.A = value; break;
        default: unreachable();
    }
}

static uint32_t cpu_read_rp(int i) {
    uint32_t value;
    switch (i) {
        case 0: value = cpu.registers.BC; break;
        case 1: value = cpu.registers.DE; break;
        case 2: value = cpu_read_index(); break;
        case 3: value = cpu_read_sp(); break;
        default: unreachable();
    }
    return cpu_mask_mode(value, cpu.L);
}
static void cpu_write_rp(int i, uint32_t value) {
    value = cpu_mask_mode(value, cpu.L);
    switch (i) {
        case 0: cpu.registers.BC = value; break;
        case 1: cpu.registers.DE = value; break;
        case 2: cpu_write_index(value); break;
        case 3: cpu_write_sp(value); break;
        default: unreachable();
    }
}

static uint32_t cpu_read_rp2(int i) {
    if (i == 3) {
        return cpu.registers.AF;
    } else {
        return cpu_read_rp(i);
    }
}
static void cpu_write_rp2(int i, uint32_t value) {
    if (i == 3) {
        cpu.registers.AF = value;
    } else {
        cpu_write_rp(i, value);
    }
}

static uint32_t cpu_read_rp3(int i) {
    uint32_t value;
    switch (i) {
        case 0: value = cpu.registers.BC; break;
        case 1: value = cpu.registers.DE; break;
        case 2: value = cpu.registers.HL; break;
        case 3: value = cpu_read_index(); break;
        default: unreachable();
    }
    return cpu_mask_mode(value, cpu.L);
}
static void cpu_write_rp3(int i, uint32_t value) {
    value = cpu_mask_mode(value, cpu.L);
    switch (i) {
        case 0: cpu.registers.BC = value; break;
        case 1: cpu.registers.DE = value; break;
        case 2: cpu.registers.HL = value; break;
        case 3: cpu_write_index(value); break;
        default: unreachable();
    }
}

static bool cpu_read_cc(const int i) {
    switch (i) {
        case 0: return !cpu.registers.flags.Z;
        case 1: return  cpu.registers.flags.Z;
        case 2: return !cpu.registers.flags.C;
        case 3: return  cpu.registers.flags.C;
        case 4: return !cpu.registers.flags.PV;
        case 5: return  cpu.registers.flags.PV;
        case 6: return !cpu.registers.flags.S;
        case 7: return  cpu.registers.flags.S;
        default: unreachable();
    }
    return true;
}

static void cpu_execute_daa(void) {
    eZ80registers_t *r = &cpu.registers;
    uint8_t old = r->A;
    uint8_t v = 0;
    if ((r->A & 0xF) > 9 || r->flags.H) {
        v += 6;
    }
    if (((r->A + v) >> 4) > 9 || cpuflag_carry_b(r->A + v) || r->flags.C) {
        v += 0x60;
    }
    if (r->flags.N) {
        r->A -= v;
        r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
            | cpuflag_undef(r->F) | cpuflag_parity(r->A)
            | cpuflag_subtract(r->flags.N) | cpuflag_c(v >= 0x60)
            | cpuflag_halfcarry_b_sub(old, v, 0);
    } else {
        r->A += v;
        r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
            | cpuflag_undef(r->F) | cpuflag_parity(r->A)
            | cpuflag_subtract(r->flags.N) | cpuflag_c(v >= 0x60)
            | cpuflag_halfcarry_b_add(old, v, 0);
    }
}

static uint32_t cpu_dec_bc_partial_mode() {
    uint32_t value = cpu_mask_mode(cpu.registers.BC - 1, cpu.L);
    if (cpu.L) {
        cpu.registers.BC = value;
    } else {
        cpu.registers.BCS = value;
    }
    return value;
}

static void cpu_rst(uint32_t address, bool stack, bool mode, bool mixed) {
#ifdef DEBUG_SUPPORT
    debug_record_call(cpu.registers.PC, cpu.L);
#endif
    cpu.cycles++;
    if (mixed) {
        if (cpu.ADL) {
            cpu_push_byte_mode(cpu.registers.PCU, true);
            cpu_push_byte_mode((cpu.MADL << 1) | cpu.ADL, true);
        }
        cpu_push_byte_mode(cpu.registers.PCH, stack);
        cpu_push_byte_mode(cpu.registers.PCL, stack);
        if (!cpu.ADL) {
            cpu_push_byte_mode((cpu.MADL << 1) | cpu.ADL, true);
        }
    } else {
        cpu_push_word(cpu.registers.PC);
    }
    cpu_prefetch(address, mode);
}

static void cpu_call(uint32_t address, bool mode, bool mixed) {
#ifdef DEBUG_SUPPORT
    debug_record_call(cpu.registers.PC, cpu.L);
#endif
    if (mixed) {
        bool stack = cpu.IL || (cpu.L && !cpu.ADL);
        cpu.registers.R += cpu.IL << 1;
        if (cpu.ADL) {
            cpu_push_byte_mode(cpu.registers.PCU, true);
            if (!cpu.IL) {
                cpu_push_byte_mode((cpu.MADL << 1) | cpu.ADL, true);
            }
        }
        cpu_push_byte_mode(cpu.registers.PCH, stack);
        cpu_push_byte_mode(cpu.registers.PCL, stack);
        if (cpu.IL || !cpu.ADL) {
            cpu_push_byte_mode((cpu.MADL << 1) | cpu.ADL, true);
        }
    } else {
        cpu_push_word(cpu.registers.PC);
    }
    cpu_prefetch(address, mode);
}

static void cpu_interrupt(uint32_t address) {
    cpu_rst(address, cpu.ADL, cpu.ADL | cpu.MADL, cpu.MADL);
}

static void cpu_trap_rewind(uint_fast8_t rewind) {
    eZ80registers_t *r = &cpu.registers;
    cpu_prefetch_discard();
    cpu.cycles++;
    r->PC = cpu_mask_mode(r->PC - rewind, cpu.ADL);
    cpu_clear_context();
    cpu_interrupt(0x00);
}

static void cpu_trap(void) {
    cpu_trap_rewind(1);
}

static void cpu_jump(uint32_t address, bool mode) {
    cpu_prefetch(address, mode);
#ifdef DEBUG_SUPPORT
    debug_record_ret(cpu.registers.PC, mode);
#endif
}

static void cpu_return(void) {
    uint32_t address;
    bool mode = cpu.ADL;
    cpu.cycles++;
    if (cpu.SUFFIX) {
        mode = cpu_pop_byte_mode(true) & 1;
        address = cpu_pop_byte_mode(cpu.ADL);
        address |= cpu_pop_byte_mode(cpu.ADL) << 8;
        if (mode) {
            address |= cpu_mask_mode(cpu_pop_byte_mode(true) << 16, cpu.ADL || cpu.L);
        }
    } else {
        address = cpu_pop_word();
    }
    cpu_jump(address, mode);
}

static void cpu_execute_alu(int i, uint8_t v) {
    uint8_t old;
    eZ80registers_t *r = &cpu.registers;
    switch (i) {
        case 0: /* ADD A, v */
            old = r->A;
            r->A += v;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_overflow_b_add(old, v, r->A)
                | cpuflag_subtract(0) | cpuflag_carry_b(old + v)
                | cpuflag_halfcarry_b_add(old, v, 0);
            break;
        case 1: /* ADC A, v */
            old = r->A;
            r->A += v + r->flags.C;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_overflow_b_add(old, v, r->A)
                | cpuflag_subtract(0) | cpuflag_carry_b(old + v + r->flags.C)
                | cpuflag_halfcarry_b_add(old, v, r->flags.C);
            break;
        case 2: /* SUB v */
            old = r->A;
            r->A -= v;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_overflow_b_sub(old, v, r->A)
                | cpuflag_subtract(1) | cpuflag_carry_b(old - v)
                | cpuflag_halfcarry_b_sub(old, v, 0);
            break;
        case 3: /* SBC v */
            old = r->A;
            r->A -= v + r->flags.C;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_overflow_b_sub(old, v, r->A)
                | cpuflag_subtract(1) | cpuflag_carry_b(old - v - r->flags.C)
                | cpuflag_halfcarry_b_sub(old, v, r->flags.C);
            break;
        case 4: /* AND v */
            r->A &= v;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_parity(r->A)
                | FLAG_H;
            break;
        case 5: /* XOR v */
            r->A ^= v;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_parity(r->A);
            break;
        case 6: /* OR v */
            r->A |= v;
            r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                | cpuflag_undef(r->F) | cpuflag_parity(r->A);
            break;
        case 7: /* CP v */
            old = r->A - v;
            r->F = cpuflag_sign_b(old) | cpuflag_zero(old)
                | cpuflag_undef(r->F) | cpuflag_subtract(1)
                | cpuflag_carry_b(r->A - v)
                | cpuflag_overflow_b_sub(r->A, v, old)
                | cpuflag_halfcarry_b_sub(r->A, v, 0);
            break;
    }
}

static void cpu_execute_rot(int y, int z, uint32_t address, uint8_t value) {
    eZ80registers_t *r = &cpu.registers;
    uint8_t old_7 = (value & 0x80) != 0;
    uint8_t old_0 = (value & 0x01) != 0;
    uint8_t old_c = r->flags.C;
    uint8_t new_c;
    cpu.cycles += z == 6;
    switch (y) {
        case 0: /* RLC value[z] */
            value <<= 1;
            value |= old_7;
            new_c = old_7;
            break;
        case 1: /* RRC value[z] */
            value >>= 1;
            value |= old_0 << 7;
            new_c = old_0;
            break;
        case 2: /* RL value[z] */
            value <<= 1;
            value |= old_c;
            new_c = old_7;
            break;
        case 3: /* RR value[z] */
            value >>= 1;
            value |= old_c << 7;
            new_c = old_0;
            break;
        case 4: /* SLA value[z] */
            value <<= 1;
            new_c = old_7;
            break;
        case 5: /* SRA value[z] */
            value >>= 1;
            value |= old_7 << 7;
            new_c = old_0;
            break;
        case 7: /* SRL value[z] */
            value >>= 1;
            new_c = old_0;
            break;
        default:
            unreachable();
    }
    cpu_write_reg_prefetched(z, address, value);
    r->F = cpuflag_c(new_c) | cpuflag_sign_b(value) | cpuflag_parity(value)
        | cpuflag_undef(r->F) | cpuflag_zero(value);
}

static void cpu_execute_rot_acc(int y)
{
    eZ80registers_t *r = &cpu.registers;
    uint8_t old;
    switch (y) {
        case 0: /* RLCA */
            old = (r->A & 0x80) > 0;
            r->flags.C = old;
            r->A <<= 1;
            r->A |= old;
            r->flags.N = r->flags.H = 0;
            break;
        case 1: /* RRCA */
            old = (r->A & 1) > 0;
            r->flags.C = old;
            r->A >>= 1;
            r->A |= old << 7;
            r->flags.N = r->flags.H = 0;
            break;
        case 2: /* RLA */
            old = r->flags.C;
            r->flags.C = (r->A & 0x80) > 0;
            r->A <<= 1;
            r->A |= old;
            r->flags.N = r->flags.H = 0;
            break;
        case 3: /* RRA */
            old = r->flags.C;
            r->flags.C = (r->A & 1) > 0;
            r->A >>= 1;
            r->A |= old << 7;
            r->flags.N = r->flags.H = 0;
            break;
        case 4: /* DAA */
            cpu_execute_daa();
            break;
        case 5: /* CPL */
            r->A = ~r->A;
            r->flags.N = r->flags.H = 1;
            break;
        case 6: /* SCF */
            r->flags.C = 1;
            r->flags.N = r->flags.H = 0;
            break;
        case 7: /* CCF */
            r->flags.H = r->flags.C;
            r->flags.C = !r->flags.C;
            r->flags.N = 0;
            break;
    }
}

static void cpu_execute_bli() {
    eZ80registers_t *r = &cpu.registers;
    uint8_t old, new = 0;
    uint_fast8_t internalCycles = 1;
    uint_fast8_t xp = cpu.context.x << 2 | cpu.context.p;
    int_fast8_t delta = cpu.context.q ? -1 : 1;
    bool repeat = (cpu.context.x | cpu.context.p) & 1;
    do {
#ifdef DEBUG_SUPPORT
        if (cpu.inBlock) {
            debug_inst_repeat();
        }
#endif
        switch (cpu.context.z) {
            case 0:
                switch (xp) {
                    case 0xA: /* LDI, LDD */
                    case 0xB: /* LDIR, LDDR */
                        break;
                    default:
                        cpu_trap();
                        return;
                }
                /* LDI, LDD, LDIR, LDDR */
                cpu_write_byte(r->DE, cpu_read_byte(r->HL));
                r->DE = cpu_mask_mode(r->DE + delta, cpu.L);
                r->flags.H = 0;
                r->flags.PV = cpu_dec_bc_partial_mode() != 0; /* Do not mask BC */
                r->flags.N = 0;
                repeat &= r->flags.PV;
                break;
            case 1:
                switch (xp) {
                    case 0xA: /* CPI, CPD */
                        break;
                    case 0xB: /* CPIR, CPDR */
                        internalCycles = 2;
                        break;
                    default:
                        cpu_trap();
                        return;
                }
                /* CPI, CPD, CPIR, CPDR */
                old = cpu_read_byte(r->HL);
                new = r->A - old;
                r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                    | cpuflag_halfcarry_b_sub(r->A, old, 0)
                    | cpuflag_pv(cpu_dec_bc_partial_mode()) /* Do not mask BC */
                    | cpuflag_subtract(1) | cpuflag_c(r->flags.C)
                    | cpuflag_undef(r->F);
                repeat &= !r->flags.Z && r->flags.PV;
                if (!repeat) {
                    internalCycles--;
                }
                break;
            case 2:
                switch (xp) {
                    case 0x8: /* INIM, INDM */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->C));
                        r->C += delta;
                        old = r->B--;
                        r->F = cpuflag_sign_b(r->B) | cpuflag_zero(r->B)
                            | cpuflag_halfcarry_b_sub(old, 0, 1)
                            | cpuflag_subtract(cpuflag_sign_b(new)) | cpuflag_undef(r->F);
                        break;
                    case 0x9: /* INIMR, INDMR */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->C));
                        r->C += delta;
                        r->flags.Z = --r->B == 0;
                        r->flags.N = cpuflag_sign_b(new) != 0;
                        break;
                    case 0xA: /* INI, IND */
                    case 0xB: /* INIR, INDR */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->BC));
                        r->flags.Z = --r->B == 0;
                        r->flags.N = cpuflag_sign_b(new) != 0;
                        break;
                    case 0xC: /* INIRX, INDRX */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->DE));
                        r->flags.Z = cpu_dec_bc_partial_mode() == 0; /* Do not mask BC */
                        r->flags.N = cpuflag_sign_b(new) != 0;
                        break;
                    default:
                        cpu_trap();
                        return;
                }
                /* INIM, INDM, INIMR, INDMR, INI, IND, INIR, INDR, INIRX, INDRX */
                repeat &= !r->flags.Z;
                break;
            case 3:
                switch (xp) {
                    case 0x8: /* OTIM, OTDM */
                    case 0x9: /* OTIMR, OTDMR */
                        cpu_write_out(r->C, new = cpu_read_byte(r->HL));
                        r->C += delta;
                        old = r->B--;
                        r->F = cpuflag_sign_b(r->B) | cpuflag_zero(r->B)
                            | cpuflag_halfcarry_b_sub(old, 0, 1)
                            | cpuflag_subtract(cpuflag_sign_b(new)) | cpuflag_undef(r->F);
                        break;
                    case 0xA: /* OUTI, OUTD */
                    case 0xB: /* OTIR, OTDR */
                        cpu_write_out(r->BC, new = cpu_read_byte(r->HL));
                        r->flags.Z = --r->B == 0;
                        r->flags.N = cpuflag_sign_b(new) != 0;
                        break;
                    case 0xC: /* OTIRX, OTDRX */
                        cpu_write_out(r->DE, new = cpu_read_byte(r->HL));
                        r->flags.Z = cpu_dec_bc_partial_mode() == 0; /* Do not mask BC */
                        r->flags.N = cpuflag_sign_b(new) != 0;
                        break;
                    default:
                        cpu_trap();
                        return;
                }
                /* OTIM, OTDM, OTIMR, OTDMR, OUTI, OUTD, OTIR, OTDR, OTIRX, OTDRX */
                repeat &= !r->flags.Z;
                break;
            case 4:
                if (xp & 1) {
                    if (xp & 2) { /* OTI2R, OTD2R */
                        cpu_write_out(r->DE, new = cpu_read_byte(r->HL));
                    } else {      /* INI2R, IND2R */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->DE));
                    }
                    /* INI2R, IND2R, OTI2R, OTD2R */
                    r->DE = cpu_mask_mode(r->DE + delta, cpu.L);
                    r->flags.Z = cpu_dec_bc_partial_mode() == 0; /* Do not mask BC */
                    repeat &= !r->flags.Z;
                } else {
                    if (xp & 2) { /* OUTI2, OUTD2 */
                        cpu_write_out(r->BC, new = cpu_read_byte(r->HL));
                    } else {      /* INI2, IND2 */
                        cpu_write_byte(r->HL, new = cpu_read_in(r->BC));
                    }
                    /* INI2, IND2, OUTI2, OUTD2 */
                    r->C += delta;
                    r->flags.Z = --r->B == 0;
                }
                r->flags.N = cpuflag_sign_b(new) != 0;
                break;
            default:
                cpu_trap();
                return;
        }
        /* All block instructions */
        r->HL = cpu_mask_mode(r->HL + delta, cpu.L);
        cpu.cycles += internalCycles;
        if (repeat) {
            switch (cpu.context.opcode) {
                default:
                    r->R += 4;
                    break;
                case 0x92: /* INIMR */
                case 0x9A: /* INDMR */
                case 0x9C: /* IND2R */
                case 0xBA: /* INDR  */
                case 0xCA: /* INDRX */
                    r->R += 2;
                    break;
            }
        }
    } while ((cpu.inBlock = repeat) && cpu.cycles < cpu.next);
}

void cpu_init(void) {
    memset(&cpu, 0, sizeof(eZ80cpu_t));
    cpu_atomics.signals = CPU_SIGNAL_ON_KEY | CPU_SIGNAL_ANY_KEY;
    gui_console_printf("[CEmu] Initialized CPU...\n");
}

void cpu_reset(void) {
    memset(&cpu, 0, sizeof(cpu));
    cpu_restore_next();
    cpu_flush(0, false);
    gui_console_printf("[CEmu] CPU reset.\n");
}

void cpu_flush(uint32_t address, bool mode) {
    cpu_prefetch(address, mode);
    cpu_inst_start();
    cpu.inBlock = false;
}

void cpu_nmi(void) {
    cpu.NMI = 1;
    cpu_restore_next();
#ifdef DEBUG_SUPPORT
    if (debug_get_flags() & DBG_OPEN_ON_RESET) {
        debug_open(DBG_NMI_TRIGGERED, cpu.registers.PC);
    }
#endif
}

void cpu_set_signal(uint8_t signal) {
    atomic8_fetch_or_explicit(&cpu_atomics.signals, signal, memory_order_release);
}

uint8_t cpu_check_signals(void) {
    return atomic_load_explicit(&cpu_atomics.signals, memory_order_relaxed);
}

uint8_t cpu_clear_signals(void) {
    return atomic8_fetch_and_explicit(&cpu_atomics.signals, CPU_SIGNAL_EXIT, memory_order_acquire);
}

void cpu_crash(const char *msg) {
    cpu_set_signal(CPU_SIGNAL_RESET);
    gui_console_printf("[CEmu] Reset caused by %s.\n", msg);
#ifdef DEBUG_SUPPORT
    if (debug_get_flags() & DBG_OPEN_ON_RESET) {
        debug_open(DBG_MISC_RESET, cpu.registers.PC);
    }
#endif
}

static void cpu_halt(void) {
    cpu.halted = true;
    cpu_restore_next();
#ifdef DEBUG_SUPPORT
    while (cpu.cycles < cpu.next && !cpu.IEF1 && debug.step) {
        cpu.haltCycles++;
        cpu.cycles++;
        debug_open(DBG_FROZEN, cpu.registers.PC);
    }
#endif
    if (cpu.cycles < cpu.next) {
        cpu.haltCycles += cpu.next - cpu.cycles;
        cpu.cycles = cpu.next; /* consume all of the cycles */
    }
}

void cpu_restore_next(void) {
    if (cpu.NMI || (cpu.IEF1 && (intrpt->status & intrpt->enabled)) || cpu_check_signals() != CPU_SIGNAL_NONE) {
        cpu.next = 0; /* always applies, even after cycle rewind during port writes */
    } else if (cpu.IEF_wait) {
        cpu.next = cpu.eiDelay; /* execute one instruction */
    } else {
        cpu.next = sched_event_next_cycle();
    }
}

void cpu_execute(void) {
    /* variable declarations */
    int8_t s;
    uint32_t w = 0;

    uint8_t old = 0;
    uint32_t old_word;

    uint8_t new = 0;
    uint32_t new_word;

    uint32_t op_word;

    eZ80registers_t *r = &cpu.registers;
    eZ80context_t context;

    while (true) {
    cpu_execute_continue:
        if (cpu.IEF_wait && cpu.cycles >= cpu.eiDelay) {
            cpu.IEF_wait = false;
            cpu.IEF1 = cpu.IEF2 = true;
        }
        if (cpu.NMI || (cpu.IEF1 && (intrpt->status & intrpt->enabled))) {
            if (cpu.halted) {
                cpu.cycles++;
            } else {
                cpu_prefetch_discard();
            }
            cpu.cycles++;
            cpu.L = cpu.IL = cpu.ADL || cpu.MADL;
            cpu.IEF1 = cpu.halted = cpu.inBlock = false;
            if (cpu.NMI) {
                cpu.NMI = false;
                cpu_interrupt(0x66);
            } else {
                cpu.IEF2 = false;
                if (cpu.IM == 2) {
                    cpu_interrupt(0x38);
                } else {
                    if (asic.im2 && cpu.IM == 3) {
                        cpu.cycles++;
                        cpu_interrupt(cpu_read_word(r->I << 8 | (bus_rand() & 0xFF)));
                    } else {
                        cpu_interrupt(bus_rand() & 0x38);
                    }
                }
            }
            cpu_restore_next();
            cpu_inst_start();
        } else if (cpu.halted) {
            cpu_halt();
        } else {
            cpu_restore_next();
        }
        if (cpu.cycles >= cpu.next) {
            break;
        }
        if (cpu.inBlock) {
            goto cpu_execute_bli_continue;
        }
        do {
            /* fetch opcode */
            context.opcode = cpu_fetch_byte();
            r->R += 2;
            switch (context.x) {
                case 0:
                    switch (context.z) {
                        case 0:
                            if (cpu.PREFIX) {
                                cpu_trap();
                                break;
                            }
                            switch (context.y) {
                                case 0:  /* NOP */
                                    break;
                                case 1:  /* EX af,af' */
                                    w = r->AF;
                                    r->AF = r->_AF;
                                    r->_AF = w;
                                    break;
                                case 2: /* DJNZ d */
                                    s = cpu_fetch_offset();
                                    if (--r->B) {
                                        cpu.cycles++;
                                        cpu_prefetch(cpu_mask_mode(r->PC + s, cpu.L), cpu.ADL);
                                    }
                                    break;
                                case 3: /* JR d */
                                    s = cpu_fetch_offset();
                                    cpu_prefetch(cpu_mask_mode(r->PC + s, cpu.L), cpu.ADL);
                                    break;
                                case 4:
                                case 5:
                                case 6:
                                case 7: /* JR cc[y-4], d */
                                    s = cpu_fetch_offset();
                                    if (cpu_read_cc(context.y - 4)) {
                                        cpu.cycles++;
                                        cpu_prefetch(cpu_mask_mode(r->PC + s, cpu.L), cpu.ADL);
                                    }
                                    break;
                            }
                            break;
                        case 1:
                            switch (context.q) {
                                case 0: /* LD rr, Mmn */
                                    if (cpu.PREFIX && context.p != 2) {
                                        if (context.y == 6) { /* LD IY/IX, (IX/IY + d) */
                                            cpu_write_other_index(cpu_read_word(cpu_index_address()));
                                        } else {
                                            cpu_trap();
                                        }
                                        break;
                                    }
                                    cpu_write_rp(context.p, cpu_fetch_word());
                                    break;
                                case 1: /* ADD HL,rr */
                                    old_word = cpu_mask_mode(cpu_read_index(), cpu.L);
                                    op_word = cpu_mask_mode(cpu_read_rp(context.p), cpu.L);
                                    new_word = old_word + op_word;
                                    cpu_write_index(cpu_mask_mode(new_word, cpu.L));
                                    r->F = cpuflag_s(r->flags.S) | cpuflag_zero(!r->flags.Z)
                                        | cpuflag_undef(r->F) | cpuflag_pv(r->flags.PV)
                                        | cpuflag_subtract(0) | cpuflag_carry_w(new_word, cpu.L)
                                        | cpuflag_halfcarry_w_add(old_word, op_word, 0);
                                    break;
                            }
                            break;
                        case 2:
                            if (cpu.PREFIX && context.p != 2) {
                                cpu_trap();
                                break;
                            }
                            switch (context.q) {
                                case 0:
                                    switch (context.p) {
                                        case 0: /* LD (BC), A */
                                            cpu_write_byte(r->BC, r->A);
                                            break;
                                        case 1: /* LD (DE), A */
                                            cpu_write_byte(r->DE, r->A);
                                            break;
                                        case 2: /* LD (Mmn), HL */
                                            cpu_write_word(cpu_fetch_word(), cpu_read_index());
                                            break;
                                        case 3: /* LD (Mmn), A */
                                            cpu_write_byte(cpu_fetch_word(), r->A);
                                            break;
                                    }
                                    break;
                                case 1:
                                    switch (context.p) {
                                        case 0: /* LD A, (BC) */
                                            r->A = cpu_read_byte(r->BC);
                                            break;
                                        case 1: /* LD A, (DE) */
                                            r->A = cpu_read_byte(r->DE);
                                            break;
                                        case 2: /* LD HL, (Mmn) */
                                            cpu_write_index(cpu_read_word(cpu_fetch_word()));
                                            break;
                                        case 3: /* LD A, (Mmn) */
                                            r->A = cpu_read_byte(cpu_fetch_word());
                                            break;
                                    }
                                    break;
                            }
                            break;
                        case 3:
                            if (cpu.PREFIX && context.p != 2) {
                                cpu_trap();
                                break;
                            }
                            switch (context.q) {
                                case 0: /* INC rp[p] */
                                    cpu_write_rp(context.p, cpu_read_rp(context.p) + 1);
                                    break;
                                case 1: /* DEC rp[p] */
                                    cpu_write_rp(context.p, cpu_read_rp(context.p) - 1);
                                    break;
                            }
                            break;
                        case 4: /* INC r[y] */
                            if (cpu.PREFIX && (context.y < 4 || context.y == 7)) {
                                cpu_trap();
                                break;
                            }
                            w = context.y == 6 ? cpu_index_address() : 0;
                            old = cpu_read_reg_prefetched(context.y, w);
                            cpu.cycles += context.y == 6;
                            new = old + 1;
                            cpu_write_reg_prefetched(context.y, w, new);
                            r->F = cpuflag_c(r->flags.C) | cpuflag_sign_b(new) | cpuflag_zero(new)
                                | cpuflag_halfcarry_b_add(old, 0, 1) | cpuflag_pv(new == 0x80)
                                | cpuflag_subtract(0) | cpuflag_undef(r->F);
                            break;
                        case 5: /* DEC r[y] */
                            if (cpu.PREFIX && (context.y < 4 || context.y == 7)) {
                                cpu_trap();
                                break;
                            }
                            w = context.y == 6 ? cpu_index_address() : 0;
                            old = cpu_read_reg_prefetched(context.y, w);
                            cpu.cycles += context.y == 6;
                            new = old - 1;
                            cpu_write_reg_prefetched(context.y, w, new);
                            r->F = cpuflag_c(r->flags.C) | cpuflag_sign_b(new) | cpuflag_zero(new)
                                | cpuflag_halfcarry_b_sub(old, 0, 1) | cpuflag_pv(old == 0x80)
                                | cpuflag_subtract(1) | cpuflag_undef(r->F);
                            break;
                        case 6: /* LD r[y], n */
                            if (cpu.PREFIX) {
                                if (context.y < 4) {
                                    cpu_trap();
                                    break;
                                }
                                if (context.y == 7) { /* LD (IX/IY + d), IY/IX */
                                    cpu_write_word(cpu_index_address(), cpu_read_other_index());
                                    break;
                                }
                            }
                            if (context.y == 6) {
                                w = cpu_index_address();
                            }
                            cpu_write_reg_prefetched(context.y, w, cpu_fetch_byte());
                            break;
                        case 7:
                            if (cpu.PREFIX) {
                                if (context.q) { /* LD (IX/IY + d), rp3[p] */
                                    cpu_write_word(cpu_index_address(), cpu_read_rp3(context.p));
                                } else { /* LD rp3[p], (IX/IY + d) */
                                    cpu_write_rp3(context.p, cpu_read_word(cpu_index_address()));
                                }
                            } else {
                                cpu_execute_rot_acc(context.y);
                            }
                            break;
                    }
                    break;
                case 1: /* ignore prefixed prefixes */
                    if (context.z == context.y) {
                        if (cpu.PREFIX && context.z != 4 && context.z != 5) {
                            cpu_trap();
                            break;
                        }
                        if (context.z < 4) {
                            if (cpu.SUFFIX && (cpu_check_signals() & (CPU_SIGNAL_RESET | CPU_SIGNAL_EXIT))) { /* suffix can infinite loop */
                                return;
                            }
                            cpu.L = context.s;
                            cpu.IL = context.r;
                            cpu.SUFFIX = true;
                            continue;
                        }
                        if (context.z == 6) { /* HALT */
                            cpu.cycles++;
                            cpu_halt();
                        }
                    } else {
                        if (cpu.PREFIX &&
                            (context.z < 4 || context.z == 7) &&
                            (context.y < 4 || context.y == 7)) {
                            cpu_trap();
                            break;
                        }
                        cpu_read_write_reg(context.z, context.y);
                    }
                    break;
                case 2: /* ALU[y] r[z] */
                    if (cpu.PREFIX && (context.z < 4 || context.z == 7)) {
                        cpu_trap();
                        break;
                    }
                    cpu_execute_alu(context.y, cpu_read_reg(context.z));
                    break;
                case 3:
                    if (cpu.PREFIX && !context.s) {
                        cpu_trap();
                        break;
                    }
                    switch (context.z) {
                        case 0: /* RET cc[y] */
                            cpu.cycles++;
                            if (cpu_read_cc(context.y)) {
                                r->R += 2;
                                cpu_return();
                            }
                            break;
                        case 1:
                            switch (context.q) {
                                case 0: /* POP rp2[p] */
                                    if (cpu.PREFIX && context.p != 2) {
                                        cpu_trap();
                                        break;
                                    }
                                    cpu_write_rp2(context.p, cpu_pop_word());
                                    break;
                                case 1:
                                    if (cpu.PREFIX && context.p < 2) {
                                        cpu_trap();
                                        break;
                                    }
                                    switch (context.p) {
                                        case 0: /* RET */
                                            cpu_return();
                                            break;
                                        case 1: /* EXX */
                                            w = r->BC;
                                            r->BC = r->_BC;
                                            r->_BC = w;
                                            w = r->DE;
                                            r->DE = r->_DE;
                                            r->_DE = w;
                                            w = r->HL;
                                            r->HL = r->_HL;
                                            r->_HL = w;
                                            break;
                                        case 2: /* JP (rr) */
                                            cpu_prefetch_discard();
                                            cpu_jump(cpu_read_index(), cpu.L);
                                            break;
                                        case 3: /* LD SP, HL */
                                            cpu_write_sp(cpu_read_index());
                                            break;
                                    }
                                    break;
                            }
                            break;
                        case 2: /* JP cc[y], nn */
                            if (cpu_read_cc(context.y)) {
                                cpu.cycles++;
                                cpu_jump(cpu_fetch_word_no_prefetch(), cpu.L);
                            } else {
                                cpu_fetch_word();
                            }
                            break;
                        case 3:
                            if (cpu.PREFIX && context.y != 1 && context.y != 4) {
                                cpu_trap();
                                break;
                            }
                            switch (context.y) {
                                case 0: /* JP nn */
                                    cpu.cycles++;
                                    cpu_jump(cpu_fetch_word_no_prefetch(), cpu.L);
                                    break;
                                case 1: /* 0xCB prefixed opcodes */
                                    w = cpu_index_address();
                                    context.opcode = cpu_fetch_byte();
                                    r->R += ~cpu.PREFIX & 2;
                                    if (cpu.PREFIX && context.z != 6) { /* OPCODETRAP */
                                        cpu_trap_rewind(2);
                                        break;
                                    }
                                    if (context.x == 0 && context.y == 6) { /* OPCODETRAP */
                                        cpu_trap_rewind(1 + (cpu.PREFIX != 0));
                                        break;
                                    }
                                    old = cpu_read_reg_prefetched(context.z, w);
                                    switch (context.x) {
                                        case 0: /* rot[y] r[z] */
                                            cpu_execute_rot(context.y, context.z, w, old);
                                            break;
                                        case 1: /* BIT y, r[z] */
                                            old &= (1 << context.y);
                                            r->F = cpuflag_sign_b(old) | cpuflag_zero(old) | cpuflag_undef(r->F)
                                                | cpuflag_parity(old) | cpuflag_c(r->flags.C)
                                                | FLAG_H;
                                            break;
                                        case 2: /* RES y, r[z] */
                                            cpu.cycles += context.z == 6;
                                            old &= ~(1 << context.y);
                                            cpu_write_reg_prefetched(context.z, w, old);
                                            break;
                                        case 3: /* SET y, r[z] */
                                            cpu.cycles += context.z == 6;
                                            old |= 1 << context.y;
                                            cpu_write_reg_prefetched(context.z, w, old);
                                            break;
                                    }
                                    break;
                                case 2: /* OUT (n), A */
                                    cpu_write_out((r->A << 8) | cpu_fetch_byte(), r->A);
                                    break;
                                case 3: /* IN A, (n) */
                                    r->A = cpu_read_in((r->A << 8) | cpu_fetch_byte());
                                    break;
                                case 4: /* EX (SP), HL/I */
                                    w = cpu_read_sp();
                                    old_word = cpu_read_word(w);
                                    new_word = cpu_read_index();
                                    cpu_write_index_partial_mode(old_word);
                                    cpu_write_word(w, new_word);
                                    break;
                                case 5: /* EX DE, HL */
                                    w = cpu_mask_mode(r->DE, cpu.L);
                                    r->DE = cpu_mask_mode(r->HL, cpu.L);
                                    r->HL = w;
                                    break;
                                case 6: /* DI */
                                    cpu.IEF_wait = cpu.IEF1 = cpu.IEF2 = false;
                                    cpu_restore_next();
                                    break;
                                case 7: /* EI */
                                    cpu.IEF_wait = true;
                                    cpu.eiDelay = cpu.cycles + 1;
                                    cpu_restore_next();
                                    break;
                            }
                            break;
                        case 4: /* CALL cc[y], nn */
                            if (cpu_read_cc(context.y)) {
                                w = cpu_fetch_word_no_prefetch();
                                cpu.cycles += !cpu.SUFFIX && !cpu.ADL;
                                cpu_call(w, cpu.IL, cpu.SUFFIX);
                            } else {
                                cpu_fetch_word();
                            }
                            break;
                        case 5:
                            if (cpu.PREFIX && context.y != 4) {
                                cpu_trap();
                                break;
                            }
                            switch (context.q) {
                                case 0: /* PUSH r2p[p] */
                                    r->R += (!cpu.PREFIX && cpu.L) << 1;
                                    cpu_push_word(cpu_read_rp2(context.p));
                                    break;
                                case 1:
                                    switch (context.p) {
                                        case 0: /* CALL nn */
                                            cpu_call(cpu_fetch_word_no_prefetch(), cpu.IL, cpu.SUFFIX);
                                            break;
                                        case 1: /* 0xDD prefixed opcodes */
                                            if (cpu.PREFIX) {
                                                cpu_trap();
                                                break;
                                            }
                                            cpu.PREFIX = 2;
                                            continue;
                                        case 2: /* 0xED prefixed opcodes */
                                            cpu.PREFIX = 0; /* ED cancels effect of DD/FD prefix */
                                            context.opcode = cpu_fetch_byte();
                                            r->R += 2;
                                            switch (context.x) {
                                                case 0:
                                                    switch (context.z) {
                                                        case 0: /* IN0 r[y], (n) */
                                                            new = cpu_read_in(cpu_fetch_byte());
                                                            if (context.y != 6) {
                                                                cpu_write_reg(context.y, new);
                                                            }
                                                            r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                                                                | cpuflag_undef(r->F) | cpuflag_parity(new)
                                                                | cpuflag_c(r->flags.C);
                                                            break;
                                                         case 1:
                                                            if (context.y == 6) { /* LD IY, (HL) */
                                                                r->IY = cpu_read_word(r->HL);
                                                            } else { /* OUT0 (n), r[y] */
                                                                cpu_write_out(cpu_fetch_byte(), cpu_read_reg(context.y));
                                                            }
                                                            break;
                                                        case 2: /* LEA rp3[p], IX */
                                                        case 3: /* LEA rp3[p], IY */
                                                            if (context.q) { /* OPCODETRAP */
                                                                cpu_trap();
                                                                break;
                                                            }
                                                            cpu.PREFIX = context.z;
                                                            cpu_write_rp3(context.p, cpu_index_address());
                                                            break;
                                                        case 4: /* TST A, r[y] */
                                                            new = r->A & cpu_read_reg(context.y);
                                                            r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                                                                | cpuflag_undef(r->F) | cpuflag_parity(new)
                                                                | FLAG_H;
                                                            break;
                                                        case 6:
                                                            if (context.y == 7) { /* LD (HL), IY */
                                                                cpu_write_word(r->HL, r->IY);
                                                                break;
                                                            }
                                                            fallthrough;
                                                        case 5: /* OPCODETRAP */
                                                            cpu_trap();
                                                            break;
                                                        case 7:
                                                            cpu.PREFIX = 2;
                                                            if (context.q) { /* LD (HL), rp3[p] */
                                                                cpu_write_word(r->HL, cpu_read_rp3(context.p));
                                                            } else { /* LD rp3[p], (HL) */
                                                                cpu_write_rp3(context.p, cpu_read_word(r->HL));
                                                            }
                                                            break;
                                                    }
                                                    break;
                                                case 1:
                                                    switch (context.z) {
                                                        case 0: /* IN r[y], (BC) */
                                                            new = cpu_read_in(r->BC);
                                                            if (context.y != 6) {
                                                                cpu_write_reg(context.y, new);
                                                            }
                                                            r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                                                                | cpuflag_undef(r->F) | cpuflag_parity(new)
                                                                | cpuflag_c(r->flags.C);
                                                            break;
                                                        case 1: /* OUT (BC), r[y] */
                                                            if (context.y == 6) { /* OPCODETRAP (ADL) */
                                                                cpu_trap();
                                                                break;
                                                            }
                                                            cpu_write_out(r->BC, cpu_read_reg(context.y));
                                                            break;
                                                        case 2:
                                                            old_word = cpu_mask_mode(r->HL, cpu.L);
                                                            op_word = cpu_mask_mode(cpu_read_rp(context.p), cpu.L);
                                                            if (context.q == 0) { /* SBC HL, rp[p] */
                                                                new_word = old_word - op_word - r->flags.C;
                                                                r->HL = cpu_mask_mode(new_word, cpu.L);
                                                                r->F = cpuflag_sign_w(r->HL, cpu.L) | cpuflag_zero(r->HL)
                                                                    | cpuflag_undef(r->F) | cpuflag_overflow_w_sub(old_word, op_word, r->HL, cpu.L)
                                                                    | cpuflag_subtract(1) | cpuflag_carry_w(new_word, cpu.L)
                                                                    | cpuflag_halfcarry_w_sub(old_word, op_word, r->flags.C);
                                                            } else { /* ADC HL, rp[p] */
                                                                new_word = old_word + op_word + r->flags.C;
                                                                r->HL = cpu_mask_mode(new_word, cpu.L);
                                                                r->F = cpuflag_sign_w(r->HL, cpu.L) | cpuflag_zero(r->HL)
                                                                    | cpuflag_undef(r->F) | cpuflag_overflow_w_add(old_word, op_word, r->HL, cpu.L)
                                                                    | cpuflag_subtract(0) | cpuflag_carry_w(new_word, cpu.L)
                                                                    | cpuflag_halfcarry_w_add(old_word, op_word, r->flags.C);
                                                            }
                                                            break;
                                                        case 3:
                                                            if (context.q == 0) { /* LD (nn), rp[p] */
                                                                cpu_write_word(cpu_fetch_word(), cpu_read_rp(context.p));
                                                            } else { /* LD rp[p], (nn) */
                                                                cpu_write_rp(context.p, cpu_read_word(cpu_fetch_word()));
                                                            }
                                                            break;
                                                        case 4:
                                                            if (context.q == 0) {
                                                                switch (context.p) {
                                                                    case 0:  /* NEG */
                                                                        old = r->A;
                                                                        r->A = -r->A;
                                                                        r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                                                                            | cpuflag_undef(r->F) | cpuflag_pv(old == 0x80)
                                                                            | cpuflag_subtract(1) | cpuflag_c(old != 0)
                                                                            | cpuflag_halfcarry_b_sub(0, old, 0);
                                                                        break;
                                                                    case 1:  /* LEA IX, IY + d */
                                                                        cpu.PREFIX = 3;
                                                                        r->IX = cpu_index_address();
                                                                        break;
                                                                    case 2:  /* TST A, n */
                                                                        new = r->A & cpu_fetch_byte();
                                                                        r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                                                                            | cpuflag_undef(r->F) | cpuflag_parity(new)
                                                                            | FLAG_H;
                                                                        break;
                                                                    case 3:  /* TSTIO n */
                                                                        new = cpu_read_in(r->C) & cpu_fetch_byte();
                                                                        r->F = cpuflag_sign_b(new) | cpuflag_zero(new)
                                                                            | cpuflag_undef(r->F) | cpuflag_parity(new)
                                                                            | FLAG_H;
                                                                        break;
                                                                }
                                                            }
                                                            else { /* MLT rp[p] */
                                                                cpu.cycles += 4;
                                                                old_word = cpu_read_rp(context.p);
                                                                new_word = (old_word&0xFF) * ((old_word>>8)&0xFF);
                                                                cpu_write_rp(context.p, new_word);
                                                                break;
                                                            }
                                                            break;
                                                        case 5:
                                                            switch (context.y) {
                                                                case 0: /* RETN */
                                                                    /* This is actually identical to reti on the z80 */
                                                                case 1: /* RETI */
                                                                    cpu.IEF1 = cpu.IEF2;
                                                                    cpu_return();
                                                                    break;
                                                                case 2: /* LEA IY, IX + d */
                                                                    cpu.PREFIX = 2;
                                                                    r->IY = cpu_index_address();
                                                                    break;
                                                                case 3:
                                                                case 6: /* OPCODETRAP */
                                                                    cpu_trap();
                                                                    break;
                                                                case 4: /* PEA IX + d */
                                                                    cpu_push_word(r->IX + cpu_fetch_offset());
                                                                    break;
                                                                case 5: /* LD MB, A */
                                                                    if (cpu.ADL) {
                                                                        r->MBASE = r->A;
                                                                    }
                                                                    break;
                                                                case 7: /* STMIX */
                                                                    cpu.MADL = 1;
                                                                    break;
                                                            }
                                                            break;
                                                        case 6: /* IM im[y] */
                                                            switch (context.y) {
                                                                case 0:
                                                                case 2:
                                                                case 3: /* IM im[y] */
                                                                    cpu.IM = context.y;
                                                                    break;
                                                                case 1: /* OPCODETRAP */
                                                                    cpu_trap();
                                                                    break;
                                                                case 4: /* PEA IY + d */
                                                                    cpu_push_word(r->IY + cpu_fetch_offset());
                                                                    break;
                                                                case 5: /* LD A, MB */
                                                                    r->A = r->MBASE;
                                                                    break;
                                                                case 6: /* SLP */
                                                                    cpu.cycles++;
                                                                    cpu_halt();
                                                                    break;
                                                                case 7: /* RSMIX */
                                                                    cpu.MADL = 0;
                                                                    break;
                                                            }
                                                            break;
                                                        case 7:
                                                            switch (context.y) {
                                                                case 0: /* LD I, A */
                                                                    r->I = r->A;
                                                                    break;
                                                                case 1: /* LD R, A */
                                                                    r->R = r->A << 1 | r->A >> 7;
                                                                    break;
                                                                case 2: /* LD A, I */
                                                                    r->A = r->I;
                                                                    r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                                                                        | cpuflag_undef(r->F) | cpuflag_pv(cpu.IEF1)
                                                                        | cpuflag_subtract(0) | cpuflag_c(r->flags.C);
                                                                    break;
                                                                case 3: /* LD A, R */
                                                                    r->A = r->R >> 1 | r->R << 7;
                                                                    r->F = cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                                                                        | cpuflag_undef(r->F) | cpuflag_pv(cpu.IEF1)
                                                                        | cpuflag_subtract(0) | cpuflag_c(r->flags.C);
                                                                    break;
                                                                case 4: /* RRD */
                                                                    old = r->A;
                                                                    new = cpu_read_byte(r->HL);
                                                                    cpu.cycles++;
                                                                    r->A &= 0xF0;
                                                                    r->A |= new & 0x0F;
                                                                    new >>= 4;
                                                                    new |= old << 4;
                                                                    cpu_write_byte(r->HL, new);
                                                                    r->F = cpuflag_c(r->flags.C) | cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                                                                        | cpuflag_parity(r->A) | cpuflag_undef(r->F);
                                                                    break;
                                                                case 5: /* RLD */
                                                                    old = r->A;
                                                                    new = cpu_read_byte(r->HL);
                                                                    cpu.cycles++;
                                                                    r->A &= 0xF0;
                                                                    r->A |= new >> 4;
                                                                    new <<= 4;
                                                                    new |= old & 0x0F;
                                                                    cpu_write_byte(r->HL, new);
                                                                    r->F = cpuflag_c(r->flags.C) | cpuflag_sign_b(r->A) | cpuflag_zero(r->A)
                                                                        | cpuflag_parity(r->A) | cpuflag_undef(r->F);
                                                                    break;
                                                                default: /* OPCODETRAP */
                                                                    cpu_trap();
                                                                    break;
                                                            }
                                                            break;
                                                    }
                                                    break;
                                                case 2:
                                                    switch (context.opcode) {
                                                        case 0x92: /* INIMR */
                                                        case 0x9A: /* INDMR */
                                                            r->R -= 2; // I guess the cpu forgot to increment, so undo
                                                            break;
                                                    }
                                                cpu_execute_bli_start:
                                                    r->PC = cpu_address_mode(r->PC - 2 - cpu.SUFFIX, cpu.ADL);
                                                    cpu.context = context;
                                                cpu_execute_bli_continue:
                                                    cpu_execute_bli();
                                                    if (cpu.inBlock) {
                                                        goto cpu_execute_continue;
                                                    } else {
                                                        r->PC = cpu_address_mode(r->PC + 2 + cpu.SUFFIX, cpu.ADL);
                                                    }
                                                    break;
                                                case 3:  /* There are only a few of these, so a simple switch for these shouldn't matter too much */
                                                    switch(context.opcode) {
                                                        case 0xC2: /* INIRX */
                                                        case 0xC3: /* OTIRX */
                                                        case 0xCA: /* INDRX */
                                                        case 0xCB: /* OTDRX */
                                                            goto cpu_execute_bli_start;
                                                        case 0xC7: /* LD I, HL */
                                                            r->I = r->HL;
                                                            break;
                                                        case 0xD7: /* LD HL, I */
                                                            r->HL = cpu_mask_mode(r->I | (r->MBASE << 16), cpu.L);
                                                            r->F = cpuflag_undef(r->F);
                                                            break;
                                                        default:   /* OPCODETRAP */
                                                            cpu_trap();
                                                            break;
                                                    }
                                                    break;
                                                default: /* OPCODETRAP */
                                                    cpu_trap();
                                                    break;
                                            }
                                            break;
                                        case 3: /* 0xFD prefixed opcodes */
                                            if (cpu.PREFIX) {
                                                cpu_trap();
                                                break;
                                            }
                                            cpu.PREFIX = 3;
                                            continue;
                                    }
                                    break;
                            }
                            break;
                        case 6: /* alu[y] n */
                            cpu_execute_alu(context.y, cpu_fetch_byte());
                            break;
                        case 7: /* RST y*8 */
                            if (cpu.PREFIX) {
                                cpu_trap();
                                break;
                            }
                            cpu_rst(context.y << 3, cpu.L, cpu.L, cpu.SUFFIX);
                            break;
                    }
                    break;
            }
            cpu_inst_start();
        } while (cpu.PREFIX || cpu.SUFFIX || cpu.cycles < cpu.next);
    }
}

bool cpu_restore(FILE *image) {
    return fread(&cpu, sizeof(cpu), 1, image) == 1;
}

bool cpu_save(FILE *image) {
    return fwrite(&cpu, sizeof(cpu), 1, image) == 1;
}

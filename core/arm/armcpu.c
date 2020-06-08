#include "armcpu.h"

#include "armmem.h"
#include "armstate.h"
#include "../defines.h"

#include <stdlib.h>
#include <string.h>

#ifdef __GCC_ASM_FLAG_OUTPUTS__
# if defined(__i386) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_IX64)
#  define FLAGS_FROM_EXTENDED_X86_ASM 1
# else
#  define FLAGS_FROM_EXTENDED_X86_ASM 0
# endif
#else
# define FLAGS_FROM_EXTENDED_X86_ASM 0
#endif

#if (__has_builtin(__builtin_add_overflow) && __has_builtin(__builtin_sub_overflow)) || __GNUC__ >= 5
# define FLAGS_FROM_OVERFLOW_BUILTINS 1
#else
# define FLAGS_FROM_OVERFLOW_BUILTINS 0
#endif

#if __has_builtin(__builtin_constant_p) || __GNUC__ >= 3 // Not sure, so conservative
# define HAVE_BUILTIN_CONSTANT_P 1
#else
# define HAVE_BUILTIN_CONSTANT_P 0
#endif

void arm_cpu_reset(arm_state_t *state) {
    memset(&state->cpu, 0, sizeof(state->cpu));
    state->cpu.sp = arm_mem_load_word(state, 0x2000);
    state->cpu.pc = arm_mem_load_word(state, 0x2004) + 1;
}

static uint32_t arm_bitcount_9(uint32_t x) {
#if defined(__POPCNT__) && (__has_builtin(__builtin_popcount) || __GNUC__ >= 4)
    return __builtin_popcount(x & 0777);
#else
    uint64_t result = x & 0777, mask = UINT64_C(0x1111111111111111);
    result *= UINT64_C(0001001001001);
#if defined(__x86_64__) || defined(_M_IX64)
    __asm__("andq\t%1, %0\nimulq\t%1, %0" : "+r"(result) : "r"(mask) : "cc");
#else
    result &= mask;
    result *= mask;
#endif
    return result >> 60;
#endif
}

static uint32_t arm_movs(arm_cpu_t *cpu, uint32_t x) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    if (true
# if HAVE_BUILTIN_CONSTANT_P
        && !__builtin_constant_p(!x) && !__builtin_constant_p(x >> 31)
# endif
        ) {
        __asm__("testl\t%2, %2" : "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "r"(x) : "cc");
    } else
#endif
    {
        cpu->z = !x;
        cpu->n = x >> 31;
    }
    return x;
}

static uint32_t arm_lsls(arm_cpu_t *cpu, uint32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        cpu->c = (y == 32) & x;
        x = 0;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("shll\t%4, %0" : "+r"(x), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "Ic"(y) : "cc");
        return x;
#else
        cpu->c = x >> (32 - y) & 1;
        x <<= y;
#endif
    }
    return arm_movs(cpu, x);
}

static uint32_t arm_lsrs(arm_cpu_t *cpu, uint32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        cpu->c = (y == 32) & x >> 31;
        x = 0;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("shrl\t%4, %0" : "+r"(x), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "Ic"(y) : "cc");
        return x;
#else
        cpu->c = x >> (y - 1) & 1;
        x >>= y;
#endif
    }
    return arm_movs(cpu, x);
}

static int32_t arm_asrs(arm_cpu_t *cpu, int32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        cpu->c = x >>= 31;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("sarl\t%4, %0" : "+r"(x), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "Ic"(y) : "cc");
        return x;
#else
        cpu->c = x >> (y - 1) & 1;
        x >>= y;
#endif
    }
    return arm_movs(cpu, x);
}

static uint32_t arm_rors(arm_cpu_t *cpu, uint32_t x, uint8_t y) {
    if (likely(y)) {
        if (likely(y &= 31)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
            __asm__("rorl\t%2, %0" : "+r"(x), "=@ccc"(cpu->c) : "Ic"(y) : "cc");
#else
            x = x >> y | x << (32 - y);
            cpu->c = x >> 31;
#endif
        } else {
            cpu->c = x >> 31;
        }
    }
    return arm_movs(cpu, x);
}

static uint32_t arm_ands(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("andl\t%3, %0" : "+r"(x), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(cpu, x & y);
#endif
}

static uint32_t arm_eors(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("xorl\t%3, %0" : "+r"(x), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(cpu, x ^ y);
#endif
}

static uint32_t arm_orrs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("orl\t%3, %0" : "+r"(x), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(cpu, x | y);
#endif
}

static uint32_t arm_mvns(arm_cpu_t *cpu, uint32_t x) {
    return arm_eors(cpu, x, ~UINT32_C(0));
}

static uint32_t arm_negs(arm_cpu_t *cpu, uint32_t x) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("negl\t%0" : "+r"(x), "=@cco"(cpu->v), "=@ccnc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) :: "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    cpu->v = __builtin_sub_overflow(0, (int32_t)x, &result);
    cpu->c = x;
    return arm_movs(cpu, -x);
#else
    int64_t result = UINT64_C(0) - (int32_t)x;
    cpu->v = result != (int32_t)result;
    cpu->c = (uint32_t)result <= 0;
    //cpu->c = 0 >= x;
    return arm_movs(cpu, result);
#endif
}

static uint32_t arm_adds(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("addl\t%5, %0" : "+r"(x), "=@cco"(cpu->v), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    cpu->v = __builtin_add_overflow((int32_t)x, (int32_t)y, &result);
    cpu->c = __builtin_add_overflow(x, y, &x);
    return arm_movs(cpu, x);
#else
    int64_t result = (int64_t)(int32_t)x + (int32_t)y;
    cpu->v = result != (int32_t)result;
    cpu->c = (uint32_t)result < x;
    //cpu->c = x > ~y;
    return arm_movs(cpu, result);
#endif
}

static uint32_t arm_subs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("subl\t%5, %0" : "+r"(x), "=@cco"(cpu->v), "=@ccnc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    cpu->v = __builtin_sub_overflow((int32_t)x, (int32_t)y, &result);
    cpu->c = !__builtin_sub_overflow(x, y, &x);
    return arm_movs(cpu, x);
#else
    int64_t result = (int64_t)(int32_t)x - (int32_t)y;
    cpu->v = result != (int32_t)result;
    cpu->c = (uint32_t)result <= x;
    //cpu->c = x >= y;
    return arm_movs(cpu, result);
#endif
}

static uint32_t arm_adcs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tbtl\t$0, %k6\n\tadcl\t%5, %0\n" : "+r"(x), "=@cco"(cpu->v), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y), "m"(cpu->c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool carry = cpu->c;
    int32_t result;
    cpu->v = __builtin_add_overflow(x, y, &result);
    cpu->v |= __builtin_add_overflow(result, carry, &result);
    cpu->c = __builtin_add_overflow(x, y, &x);
    cpu->c |= __builtin_add_overflow(x, carry, &x);
    return arm_movs(cpu, x);
#else
    int64_t result = (uint64_t)(int32_t)x + (int32_t)y + cpu->c;
    cpu->v = result != (int32_t)result;
    cpu->c = ((uint64_t)x + y + cpu->c) >> 32;
    return arm_movs(cpu, result);
#endif
}

static uint32_t arm_sbcs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tcmpb\t$1, %6\n\tsbbl\t%5, %0\n" : "+r"(x), "=@cco"(cpu->v), "=@ccnc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y), "m"(cpu->c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool borrow = !cpu->c;
    int32_t result;
    cpu->v = __builtin_sub_overflow(x, y, &result);
    cpu->v |= __builtin_sub_overflow(result, borrow, &result);
    cpu->c = __builtin_sub_overflow(x, y, &x);
    cpu->c |= __builtin_sub_overflow(x, borrow, &x);
    return arm_movs(cpu, x);
#else
    int64_t result = (uint64_t)(int32_t)x - (int32_t)y - !cpu->c;
    cpu->v = result != (int32_t)result;
    cpu->c = ((uint64_t)x - y - !cpu->c) >> 32;
    return arm_movs(cpu, result);
#endif
}

static uint32_t arm_rev(uint32_t x) {
    return (x >> 24 & UINT32_C(0x000000FF)) |
           (x >>  8 & UINT32_C(0x0000FF00)) |
           (x <<  8 & UINT32_C(0x00FF0000)) |
           (x << 24 & UINT32_C(0xFF000000));
}

static uint32_t arm_rev16(uint32_t x) {
    return (x >> 8 & UINT32_C(0x000000FF)) |
           (x << 8 & UINT32_C(0x0000FF00)) |
           (x >> 8 & UINT32_C(0x00FF0000)) |
           (x << 8 & UINT32_C(0xFF000000));
}

static int16_t arm_revsh(uint32_t x) {
    return (x >> 8 & UINT32_C(0x000000FF)) |
           (x << 8 & UINT32_C(0x0000FF00));
}

static void arm_exception(arm_exception_number_t type) {
    (void)type;
    abort();
}

void arm_execute(arm_state_t *state) {
    arm_cpu_t *cpu = &state->cpu;
    uint32_t opcode;
    if (unlikely(cpu->pc & 1)) {
        arm_exception(ARM_Exception_HardFault);
    }
    opcode = arm_mem_load_half(state, cpu->pc - 2);
    cpu->pc += 2;
    switch (opcode >> 12 & 0xF) {
        case 0:
        case 1:
            switch (opcode >> 11 & 3) {
                case 0: // Logical Shift Left
                    cpu->r[opcode >> 0 & 7] = arm_lsls(cpu, cpu->r[opcode >> 3 & 7], opcode >> 5 & 0x1F);
                    break;
                case 1: // Logical Shift Right
                    cpu->r[opcode >> 0 & 7] = arm_lsrs(cpu, cpu->r[opcode >> 3 & 7], (((opcode >> 5) - 1) & 0x1F) + 1);
                    break;
                case 2: // Arithmetic Shift Right
                    cpu->r[opcode >> 0 & 7] = arm_asrs(cpu, cpu->r[opcode >> 3 & 7], (((opcode >> 5) - 1) & 0x1F) + 1);
                    break;
                case 3:
                    switch (opcode >> 8 & 3) {
                        case 0: // Add register
                            cpu->r[opcode >> 0 & 7] = arm_adds(cpu, cpu->r[opcode >> 3 & 7], cpu->r[opcode >> 6 & 7]);
                            break;
                        case 1: // Subtract register
                            cpu->r[opcode >> 0 & 7] = arm_subs(cpu, cpu->r[opcode >> 3 & 7], cpu->r[opcode >> 6 & 7]);
                            break;
                        case 2: // Add 3-bit immediate
                            cpu->r[opcode >> 0 & 7] = arm_adds(cpu, cpu->r[opcode >> 3 & 7], opcode >> 6 & 7);
                            break;
                        case 3: // Subtract 3-bit immediate
                            cpu->r[opcode >> 0 & 7] = arm_subs(cpu, cpu->r[opcode >> 3 & 7], opcode >> 6 & 7);
                            break;
                    }
                    break;
            }
            break;
        case 2:
        case 3:
            switch (opcode >> 11 & 3) {
                case 0: // Move
                    cpu->r[opcode >> 8 & 7] = arm_movs(cpu, opcode >> 0 & 0xFF);
                    break;
                case 1: // Compare
                    arm_subs(cpu, cpu->r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
                case 2: // Add 8-bit immediate
                    cpu->r[opcode >> 8 & 7] = arm_adds(cpu, cpu->r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
                case 3: // Subtract 8-bit immediate
                    cpu->r[opcode >> 8 & 7] = arm_subs(cpu, cpu->r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
            }
            break;
        case 4:
            switch (opcode >> 10 & 3) {
                case 0: // Data processing
                    switch (opcode >> 6 & 0xF) {
                        case 0: // Bitwise AND
                            cpu->r[opcode >> 0 & 7] = arm_ands(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 1: // Exclusive OR
                            cpu->r[opcode >> 0 & 7] = arm_eors(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 2: // Logical Shift Left
                            cpu->r[opcode >> 0 & 7] = arm_lsls(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 3: // Logical Shift Right
                            cpu->r[opcode >> 0 & 7] = arm_lsrs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 4: // Arithmetic Shift Right
                            cpu->r[opcode >> 0 & 7] = arm_asrs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 5: // Add with Carry
                            cpu->r[opcode >> 0 & 7] = arm_adcs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 6: // Subtract with Carry
                            cpu->r[opcode >> 0 & 7] = arm_sbcs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 7: // Rotate Right
                            cpu->r[opcode >> 0 & 7] = arm_rors(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 8: // Set flags on bitwise AND
                            arm_ands(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 9: // Reverse Subtract from 0
                            cpu->r[opcode >> 0 & 7] = arm_negs(cpu, cpu->r[opcode >> 3 & 7]);
                            break;
                        case 10: // Compare Registers
                            arm_subs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 11: // Compare Negative
                            arm_adds(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 12: // Logical OR
                            cpu->r[opcode >> 0 & 7] = arm_orrs(cpu, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7]);
                            break;
                        case 13: // Multiply Two Registers
                            cpu->r[opcode >> 0 & 7] = arm_movs(cpu, cpu->r[opcode >> 0 & 7] * cpu->r[opcode >> 3 & 7]);
                            break;
                        case 14: // Bit Clear
                            cpu->r[opcode >> 0 & 7] = arm_ands(cpu, cpu->r[opcode >> 0 & 7], ~cpu->r[opcode >> 3 & 7]);
                            break;
                        case 15: // Bitwise NOT
                            cpu->r[opcode >> 0 & 7] = arm_mvns(cpu, cpu->r[opcode >> 3 & 7]);
                            break;
                    }
                    break;
                case 1: // Special data instructions and branch and exchange
                    switch (opcode >> 8 & 3) {
                        case 0: // Add Registers
                            cpu->r[(opcode >> 4 & 8) | (opcode >> 0 & 7)] += cpu->r[opcode >> 1 & 7];
                            break;
                        case 1: // Compare Registers
                            arm_subs(cpu, cpu->r[(opcode >> 4 & 8) | (opcode >> 0 & 7)], cpu->r[opcode >> 1 & 7]);
                            break;
                        case 2: // Move Registers
                            cpu->r[(opcode >> 4 & 8) | (opcode >> 0 & 7)] = cpu->r[opcode >> 1 & 7];
                            break;
                        case 3: { // Branch (with Link) and Exchange
                            uint32_t address = cpu->r[opcode >> 3 & 0xF];
                            if (unlikely(opcode >> 7 & 1)) { // Branch with Link and Exchange
                                cpu->lr = cpu->pc - 1;
                            } else if (unlikely(cpu->mode && address >> 28 == 0xF)) {
                                // Exception Return
                                abort();
                            }
                            cpu->pc = address + 1;
                            break;
                        }
                    }
                    break;
                default: // Load from Literal Pool
                    cpu->r[opcode >> 8 & 7] = arm_mem_load_word(state, ((cpu->pc >> 2) + (opcode & 0xFF)) << 2);
                    break;
            }
            break;
        case 5:
            switch (opcode >> 9 & 7) {
                case 0: // Store Register
                    arm_mem_store_word(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 1: // Store Register Halfword
                    arm_mem_store_half(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 2: // Store Register Byte
                    arm_mem_store_byte(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 3: // Load Register Signed Byte
                    cpu->r[opcode >> 0 & 7] = (int8_t)arm_mem_load_byte(state, cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 4: // Load Register
                    cpu->r[opcode >> 0 & 7] = arm_mem_load_word(state, cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 5: // Load Register Halfword
                    cpu->r[opcode >> 0 & 7] = arm_mem_load_half(state, cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 6: // Load Register Byte
                    cpu->r[opcode >> 0 & 7] = arm_mem_load_byte(state, cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
                case 7: // Load Register Signed Halfword
                    cpu->r[opcode >> 0 & 7] = (int16_t)arm_mem_load_half(state, cpu->r[opcode >> 3 & 7] + cpu->r[opcode >> 6 & 7]);
                    break;
            }
            break;
        case 6:
            if (opcode >> 11 & 1) { // Load Register
                cpu->r[opcode >> 0 & 7] = arm_mem_load_word(state, cpu->r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 2));
            } else { // Store Register
                arm_mem_store_word(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 2));
            }
            break;
        case 7:
            if (opcode >> 11 & 1) { // Load Register Byte
                cpu->r[opcode >> 0 & 7] = arm_mem_load_byte(state, cpu->r[opcode >> 3 & 7] + (opcode >> 6 & 0x1F));
            } else { // Store Register Byte
                arm_mem_store_byte(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + (opcode >> 6 & 0x1F));
            }
            break;
        case 8:
            if (opcode >> 11 & 1) { // Load Register Halfword
                cpu->r[opcode >> 0 & 7] = arm_mem_load_half(state, cpu->r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 1));
            } else { // Store Register Halfword
                arm_mem_store_half(state, cpu->r[opcode >> 0 & 7], cpu->r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 1));
            }
            break;
        case 9:
            if (opcode >> 11 & 1) { // Load Register SP relative
                cpu->r[opcode >> 8 & 7] = arm_mem_load_word(state, cpu->sp + ((opcode >> 0 & 0xFF) << 2));
            } else { // Store Register SP relative
                arm_mem_store_word(state, cpu->r[opcode >> 8 & 7], cpu->sp + ((opcode >> 0 & 0xFF) << 2));
            }
            break;
        case 10: // Generate SP/PC
            cpu->r[opcode >> 8 & 7] = cpu->r[opcode >> 11 & 1 ? 13 : 15] + ((opcode >> 0 & 0xFF) << 2);
            break;
        case 11: // Miscellaneous 16-bit instructions
            switch (opcode >> 9 & 7) {
                case 0:
                    switch (opcode >> 7 & 3) {
                        case 0: // Add Immediate to SP
                            cpu->sp += (opcode & 0x7F) << 2;
                            break;
                        case 1: // Subtract Immediate from SP
                            cpu->sp -= (opcode & 0x7F) << 2;
                            break;
                    }
                    break;
                case 1:
                    switch (opcode >> 6 & 7) {
                        case 0: // Signed Extend Halfword
                            cpu->r[opcode >> 0 & 7] = (int16_t)cpu->r[opcode >> 3 & 7];
                            break;
                        case 1: // Signed Extend Byte
                            cpu->r[opcode >> 0 & 7] = (int8_t)cpu->r[opcode >> 3 & 7];
                            break;
                        case 2: // Unsigned Extend Halfword
                            cpu->r[opcode >> 0 & 7] = (uint16_t)cpu->r[opcode >> 3 & 7];
                            break;
                        case 3: // Unsigned Extend Byte
                            cpu->r[opcode >> 0 & 7] = (uint8_t)cpu->r[opcode >> 3 & 7];
                            break;
                    }
                    break;
                case 2: { // Push Multiple Registers
                    uint32_t address = cpu->sp -= (arm_bitcount_9(opcode) << 2);
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_mem_store_word(state, cpu->r[i], address);
                            address += 4;
                        }
                    }
                    if (opcode >> i & 1) {
                        arm_mem_store_word(state, cpu->lr, address);
                    }
                    break;
                }
                case 3:
                    switch (opcode >> 5 & 0xF) {
                        case 3: // Change Processor State
                            cpu->pm = opcode >> 4 & 1;
                            break;
                    }
                    break;
                case 5:
                    switch (opcode >> 6 & 7) {
                        case 0: // Byte-Reverse Word
                            cpu->r[opcode >> 0 & 7] = arm_rev(cpu->r[opcode >> 3 & 7]);
                            break;
                        case 1: // Byte-Reverse Packed Halfword
                            cpu->r[opcode >> 0 & 7] = arm_rev16(cpu->r[opcode >> 3 & 7]);
                            break;
                        case 3: // Byte-Reverse Signed Halfword
                            cpu->r[opcode >> 0 & 7] = arm_revsh(cpu->r[opcode >> 3 & 7]);
                            break;
                    }
                    break;
                case 6: { // Pop Multiple Registers
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            cpu->r[i] = arm_mem_load_word(state, cpu->sp);
                            cpu->sp += 4;
                        }
                    }
                    if (opcode >> i & 1) {
                        cpu->pc = arm_mem_load_word(state, cpu->sp) + 1;
                        cpu->sp += 4;
                    }
                    break;
                }
                case 7:
                    switch (opcode >> 8 & 1) {
                        case 0: // Breakpoint
                            break;
                        case 1: // Hints
                            switch (opcode >> 0 & 0xF) {
                                case 0:
                                    switch (opcode >> 4 & 0xF) {
                                        case 0: // No Operation hint
                                            break;
                                        case 1: // Yield hint
                                            break;
                                        case 2: // Wait for Event hint
                                            break;
                                        case 3: // Wait for Interrupt hint
                                            break;
                                        case 4: // Send Event hint
                                            break;
                                    }
                                    break;
                            }
                            break;
                    }
                    break;
            }
            break;
        case 12:
            switch (opcode >> 11 & 1) {
                case 0: { // Store multiple registers
                    uint32_t address = cpu->r[opcode >> 8 & 7];
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_mem_store_word(state, cpu->r[i], address);
                            address += 4;
                        }
                    }
                    cpu->r[opcode >> 8 & 7] = address;
                    break;
                }
                case 1: { // Load multiple registers
                    uint32_t address = cpu->r[opcode >> 8 & 7];
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            cpu->r[i] = arm_mem_load_word(state, address);
                            address += 4;
                        }
                    }
                    if (!(opcode >> (opcode >> 8 & 7) & 1)) {
                        cpu->r[opcode >> 8 & 7] = address;
                    }
                    break;
                }
            }
            break;
        case 13:
            switch (opcode >> 8 & 0xF) { // Conditional branch, and Supervisor Call
                case 0: // Branch Equal
                    if (cpu->z) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 1: // Branch Not equal
                    if (!cpu->z) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 2: // Branch Carry set
                    if (cpu->c) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 3: // Branch Carry clear
                    if (!cpu->c) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 4: // Branch Minus, negative
                    if (cpu->n) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 5: // Branch Plus, positive or zero
                    if (!cpu->n) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 6: // Branch Overflow
                    if (cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 7: // Branch No overflow
                    if (!cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 8: // Branch Unsigned higher
                    if (cpu->c && !cpu->z) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 9: // Branch Unsigned lower or same
                    if (!cpu->c || cpu->z) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 10: // Branch Signed greater than or equal
                    if (cpu->n == cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 11: // Branch Signed less than
                    if (cpu->n != cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 12: // Branch Signed greater than
                    if (!cpu->z && cpu->n == cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                case 13: // Branch Signed less than or equal
                    if (cpu->z || cpu->n != cpu->v) {
                        cpu->pc += ((int32_t)opcode << 24 >> 23) + 2;
                    }
                    break;
                default: // Permanently UNDEFINED
                    arm_exception(ARM_Exception_HardFault);
                    break;
                case 15: // Supervisor Call
                    arm_exception(ARM_Exception_SVCall);
                    break;
            }
            break;
        case 14:
            switch (opcode >> 11 & 1) {
                case 0: // Unconditional Branch
                    cpu->pc += ((int32_t)opcode << 21 >> 20) + 2;
                    break;
                default: // UNDEFINED 32-bit Thumb instruction
                    opcode = opcode << 16 | arm_mem_load_half(state, cpu->pc - 2);
                    cpu->pc += 2;
                    arm_exception(ARM_Exception_HardFault);
                    break;
            }
            break;
        case 15: // 32-bit Thumb instruction.
            opcode = opcode << 16 | arm_mem_load_half(state, cpu->pc - 2);
            cpu->pc += 2;
            if (!(opcode >> 27 & 1) && (opcode >> 15 & 1)) {
                switch (opcode >> 12 & 5) {
                    case 0:
                        switch (opcode >> 20 & 0x7F) {
                            case 0x38:
                            case 0x39: { // Move to Special Register
                                uint32_t value = cpu->r[opcode >> 16 & 0xF];
                                switch (opcode >> 0 & 0xFF) {
                                    case 0x00:
                                    case 0x01:
                                    case 0x02:
                                    case 0x03:
                                        cpu->v = value >> 28 & 1;
                                        cpu->c = value >> 29 & 1;
                                        cpu->z = value >> 30 & 1;
                                        cpu->n = value >> 31 & 1;
                                        break;
                                    case 0x80:
                                    case 0x81:
                                        *((opcode >> 0 & 1) == cpu->spsel ? &cpu->sp : &cpu->altsp) = value >> 0 & ~3;
                                        break;
                                    case 0x10:
                                        cpu->pm = value >> 0 & 1;
                                        break;
                                    case 0x14:
                                        if (cpu->mode && cpu->spsel != (value >> 1 & 1)) {
                                            uint32_t sp = cpu->sp;
                                            cpu->sp = cpu->altsp;
                                            cpu->altsp = sp;
                                            cpu->spsel = value >> 1 & 1;
                                        }
                                        break;
                                }
                                break;
                            }
                            case 0x3B: // Miscellaneous control instructions
                                switch (opcode >> 4 & 0xF) {
                                    case 4: // Data Synchronization Barrier
                                        break;
                                    case 5: // Data Memory Barrier
                                        break;
                                    case 6: // Instruction Synchronization Barrier
                                        break;
                                }
                                break;
                            case 0x3E:
                            case 0x3F: { // Move from Special Register
                                uint32_t value = 0;
                                switch (opcode >> 0 & 0xFF) {
                                    case 0x00:
                                    case 0x01:
                                    case 0x03:
                                    case 0x04:
                                    case 0x05:
                                    case 0x06:
                                    case 0x07:
                                        if (opcode >> 0 & 1) {
                                            value |= cpu->excNum << 0;
                                        }
                                        if (!(opcode >> 2 & 1)) {
                                            value |= cpu->v << 28;
                                            value |= cpu->c << 29;
                                            value |= cpu->z << 30;
                                            value |= cpu->n << 31;
                                        }
                                        break;
                                    case 0x08:
                                    case 0x09:
                                        value = (opcode >> 0 & 1) == cpu->spsel ? cpu->sp : cpu->altsp;
                                        break;
                                    case 0x10:
                                        value |= cpu->pm;
                                        break;
                                    case 0x14:
                                        value |= cpu->spsel << 1;
                                        break;
                                }
                                cpu->r[opcode >> 16 & 0xF] = value;
                                break;
                            }
                            default:
                                arm_exception(ARM_Exception_HardFault);
                                break;
                        }
                        break;
                    case 5: // Branch with Link
                        cpu->lr = cpu->pc - 1;
                        cpu->pc += ((int32_t)opcode << 5 >> 7 & UINT32_C(0xFF000000)) |
                            (~(opcode >> 3 ^ opcode << 10) & UINT32_C(0x00800000)) |
                            (~(opcode >> 4 ^ opcode << 11) & UINT32_C(0x00400000)) |
                            (opcode >> 4 & UINT32_C(0x003FF000)) |
                            (opcode << 1 & UINT32_C(0x00000FFE));
                        break;
                }
            } else { // UNDEFINED 32-bit Thumb instruction
                arm_exception(ARM_Exception_HardFault);
            }
            break;
    }
}

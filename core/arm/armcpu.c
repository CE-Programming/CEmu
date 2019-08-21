#include "armcpu.h"
#include "../defines.h"

#include "armmem.h"

#include <stdlib.h>

arm_cpu_state_t arm_cpu;

#ifdef __GCC_ASM_FLAG_OUTPUTS__
# if defined(__i386) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_IX64)
#  define FLAGS_FROM_EXTENDED_X86_ASM 1
# else
#  define FLAGS_FROM_EXTENDED_X86_ASM 0
# endif
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

static uint32_t arm_movs(uint32_t x) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    if (true
# if HAVE_BUILTIN_CONSTANT_P
        && !__builtin_constant_p(!x) && !__builtin_constant_p(x >> 31)
# endif
        ) {
        __asm__("testl\t%2, %2" : "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "r"(x) : "cc");
    } else
#endif
    {
        arm_cpu.z = !x;
        arm_cpu.n = x >> 31;
    }
    return x;
}

static uint32_t arm_lsls(uint32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        arm_cpu.c = (y == 32) & x;
        x = 0;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("shll\t%4, %0" : "+r"(x), "=@ccc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "Ic"(y) : "cc");
        return x;
#else
        arm_cpu.c = x >> (32 - y) & 1;
        x <<= y;
#endif
    }
    return arm_movs(x);
}

static uint32_t arm_lsrs(uint32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        arm_cpu.c = (y == 32) & x >> 31;
        x = 0;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("shrl\t%4, %0" : "+r"(x), "=@ccc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "Ic"(y) : "cc");
        return x;
#else
        arm_cpu.c = x >> (y - 1) & 1;
        x >>= y;
#endif
    }
    return arm_movs(x);
}

static int32_t arm_asrs(int32_t x, uint8_t y) {
    if (unlikely(y >= 32)) {
        arm_cpu.c = x >>= 31;
    } else if (likely(y)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
        __asm__("sarl\t%4, %0" : "+r"(x), "=@ccc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "Ic"(y) : "cc");
        return x;
#else
        arm_cpu.c = x >> (y - 1) & 1;
        x >>= y;
#endif
    }
    return arm_movs(x);
}

static uint32_t arm_rors(uint32_t x, uint8_t y) {
    if (likely(y)) {
        if (likely(y &= 31)) {
#if FLAGS_FROM_EXTENDED_X86_ASM
            __asm__("rorl\t%2, %0" : "+r"(x), "=@ccc"(arm_cpu.c) : "Ic"(y) : "cc");
#else
            x = x >> y | x << (32 - y);
            arm_cpu.c = x >> 31;
#endif
        } else {
            arm_cpu.c = x >> 31;
        }
    }
    return arm_movs(x);
}

static uint32_t arm_ands(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("andl\t%3, %0" : "+r"(x), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(x & y);
#endif
}

static uint32_t arm_eors(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("xorl\t%3, %0" : "+r"(x), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(x ^ y);
#endif
}

static uint32_t arm_orrs(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("orl\t%3, %0" : "+r"(x), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "r"(y) : "cc");
    return x;
#else
    return arm_movs(x | y);
#endif
}

static uint32_t arm_mvns(uint32_t x) {
    return arm_eors(x, ~UINT32_C(0));
}

static uint32_t arm_negs(uint32_t x) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("negl\t%0" : "+r"(x), "=@cco"(arm_cpu.v), "=@ccnc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) :: "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    arm_cpu.v = __builtin_sub_overflow(0, (int32_t)x, &result);
    arm_cpu.c = x;
    return arm_movs(-x);
#else
    int64_t result = UINT64_C(0) - (int32_t)x;
    arm_cpu.v = result != (int32_t)result;
    arm_cpu.c = (uint32_t)result <= 0;
    //arm_cpu.c = 0 >= x;
    return arm_movs(result);
#endif
}

static uint32_t arm_adds(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("addl\t%5, %0" : "+r"(x), "=@cco"(arm_cpu.v), "=@ccc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    arm_cpu.v = __builtin_add_overflow((int32_t)x, (int32_t)y, &result);
    arm_cpu.c = __builtin_add_overflow(x, y, &x);
    return arm_movs(x);
#else
    int64_t result = (int64_t)(int32_t)x + (int32_t)y;
    arm_cpu.v = result != (int32_t)result;
    arm_cpu.c = (uint32_t)result < x;
    //arm_cpu.c = x > ~y;
    return arm_movs(result);
#endif
}

static uint32_t arm_subs(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("subl\t%5, %0" : "+r"(x), "=@cco"(arm_cpu.v), "=@ccnc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t result;
    arm_cpu.v = __builtin_sub_overflow((int32_t)x, (int32_t)y, &result);
    arm_cpu.c = !__builtin_sub_overflow(x, y, &x);
    return arm_movs(x);
#else
    int64_t result = (int64_t)(int32_t)x - (int32_t)y;
    arm_cpu.v = result != (int32_t)result;
    arm_cpu.c = (uint32_t)result <= x;
    //arm_cpu.c = x >= y;
    return arm_movs(result);
#endif
}

static uint32_t arm_adcs(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tbtl\t$0, %k6\n\tadcl\t%5, %0\n" : "+r"(x), "=@cco"(arm_cpu.v), "=@ccc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "ir"(y), "m"(arm_cpu.c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool carry = arm_cpu.c;
    int32_t result;
    arm_cpu.v = __builtin_add_overflow(x, y, &result);
    arm_cpu.v |= __builtin_add_overflow(result, carry, &result);
    arm_cpu.c = __builtin_add_overflow(x, y, &x);
    arm_cpu.c |= __builtin_add_overflow(x, carry, &x);
    return arm_movs(x);
#else
    int64_t result = (uint64_t)(int32_t)x + (int32_t)y + arm_cpu.c;
    arm_cpu.v = result != (int32_t)result;
    arm_cpu.c = ((uint64_t)x + y + arm_cpu.c) >> 32;
    return arm_movs(result);
#endif
}

static uint32_t arm_sbcs(uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tcmpb\t$1, %6\n\tsbbl\t%5, %0\n" : "+r"(x), "=@cco"(arm_cpu.v), "=@ccnc"(arm_cpu.c), "=@ccz"(arm_cpu.z), "=@ccs"(arm_cpu.n) : "ir"(y), "m"(arm_cpu.c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool borrow = !arm_cpu.c;
    int32_t result;
    arm_cpu.v = __builtin_sub_overflow(x, y, &result);
    arm_cpu.v |= __builtin_sub_overflow(result, borrow, &result);
    arm_cpu.c = __builtin_sub_overflow(x, y, &x);
    arm_cpu.c |= __builtin_sub_overflow(x, borrow, &x);
    return arm_movs(x);
#else
    int64_t result = (uint64_t)(int32_t)x - (int32_t)y - !arm_cpu.c;
    arm_cpu.v = result != (int32_t)result;
    arm_cpu.c = ((uint64_t)x - y - !arm_cpu.c) >> 32;
    return arm_movs(result);
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
    abort();
}

void arm_execute(void) {
    uint32_t opcode;
    if (unlikely(arm_cpu.pc & 1)) {
        arm_exception(ARM_Exception_HardFault);
    }
    opcode = arm_mem_load_half(arm_cpu.pc - 4);
    arm_cpu.pc += 2;
    switch (opcode >> 12 & 0xF) {
        case 0:
        case 1:
            switch (opcode >> 11 & 3) {
                case 0: // Logical Shift Left
                    arm_cpu.r[opcode >> 0 & 7] = arm_lsls(arm_cpu.r[opcode >> 3 & 7], opcode >> 5 & 0x1F);
                    break;
                case 1: // Logical Shift Right
                    arm_cpu.r[opcode >> 0 & 7] = arm_lsrs(arm_cpu.r[opcode >> 3 & 7], (((opcode >> 5) - 1) & 0x1F) + 1);
                    break;
                case 2: // Arithmetic Shift Right
                    arm_cpu.r[opcode >> 0 & 7] = arm_asrs(arm_cpu.r[opcode >> 3 & 7], (((opcode >> 5) - 1) & 0x1F) + 1);
                    break;
                case 3:
                    switch (opcode >> 8 & 3) {
                        case 0: // Add register
                            arm_cpu.r[opcode >> 0 & 7] = arm_adds(arm_cpu.r[opcode >> 3 & 7], arm_cpu.r[opcode >> 6 & 7]);
                            break;
                        case 1: // Subtract register
                            arm_cpu.r[opcode >> 0 & 7] = arm_subs(arm_cpu.r[opcode >> 3 & 7], arm_cpu.r[opcode >> 6 & 7]);
                            break;
                        case 2: // Add 3-bit immediate
                            arm_cpu.r[opcode >> 0 & 7] = arm_adds(arm_cpu.r[opcode >> 3 & 7], opcode >> 6 & 7);
                            break;
                        case 3: // Subtract 3-bit immediate
                            arm_cpu.r[opcode >> 0 & 7] = arm_subs(arm_cpu.r[opcode >> 3 & 7], opcode >> 6 & 7);
                            break;
                    }
                    break;
            }
        case 2:
        case 3:
            switch (opcode >> 11 & 3) {
                case 0: // Move
                    arm_cpu.r[opcode >> 8 & 7] = arm_movs(opcode >> 0 & 0xFF);
                    break;
                case 1: // Compare
                    arm_subs(arm_cpu.r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
                case 2: // Add 8-bit immediate
                    arm_cpu.r[opcode >> 8 & 7] = arm_adds(arm_cpu.r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
                case 3: // Subtract 8-bit immediate
                    arm_cpu.r[opcode >> 8 & 7] = arm_subs(arm_cpu.r[opcode >> 8 & 7], opcode >> 0 & 0xFF);
                    break;
            }
            break;
        case 4:
            switch (opcode >> 10 & 3) {
                case 0: // Data processing
                    switch (opcode >> 6 & 0xF) {
                        case 0: // Bitwise AND
                            arm_cpu.r[opcode >> 0 & 7] = arm_ands(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 1: // Exclusive OR
                            arm_cpu.r[opcode >> 0 & 7] = arm_eors(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 2: // Logical Shift Left
                            arm_cpu.r[opcode >> 0 & 7] = arm_lsls(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 3: // Logical Shift Right
                            arm_cpu.r[opcode >> 0 & 7] = arm_lsrs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 4: // Arithmetic Shift Right
                            arm_cpu.r[opcode >> 0 & 7] = arm_asrs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 5: // Add with Carry
                            arm_cpu.r[opcode >> 0 & 7] = arm_adcs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 6: // Subtract with Carry
                            arm_cpu.r[opcode >> 0 & 7] = arm_sbcs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 7: // Rotate Right
                            arm_cpu.r[opcode >> 0 & 7] = arm_rors(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 8: // Set flags on bitwise AND
                            arm_ands(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 9: // Reverse Subtract from 0
                            arm_cpu.r[opcode >> 0 & 7] = arm_negs(arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 10: // Compare Registers
                            arm_subs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 11: // Compare Negative
                            arm_adds(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 12: // Logical OR
                            arm_cpu.r[opcode >> 0 & 7] = arm_orrs(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 13: // Multiply Two Registers
                            arm_cpu.r[opcode >> 0 & 7] = arm_movs(arm_cpu.r[opcode >> 0 & 7] * arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 14: // Bit Clear
                            arm_cpu.r[opcode >> 0 & 7] = arm_ands(arm_cpu.r[opcode >> 0 & 7], ~arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 15: // Bitwise NOT
                            arm_cpu.r[opcode >> 0 & 7] = arm_mvns(arm_cpu.r[opcode >> 3 & 7]);
                            break;
                    }
                    break;
                case 1: // Special data instructions and branch and exchange
                    switch (opcode >> 8 & 3) {
                        case 0: // Add Registers
                            arm_cpu.r[(opcode >> 4 & 8) | (opcode >> 0 & 7)] += arm_cpu.r[opcode >> 1 & 7];
                            break;
                        case 1: // Compare Registers
                            arm_subs(arm_cpu.r[(opcode >> 4 & 8) | (opcode >> 0 & 7)], arm_cpu.r[opcode >> 1 & 7]);
                            break;
                        case 2: // Move Registers
                            arm_cpu.r[(opcode >> 4 & 8) | (opcode >> 0 & 7)] = arm_cpu.r[opcode >> 1 & 7];
                            break;
                        case 3: { // Branch (with Link) and Exchange
                            uint32_t address = arm_cpu.r[opcode >> 3 & 0xF];
                            if (unlikely(opcode >> 7 & 1)) { // Branch with Link and Exchange
                                arm_cpu.lr = arm_cpu.pc - 1;
                            } else if (unlikely(arm_cpu.mode && address >> 28 == 0xF)) {
                                // Exception Return
                                abort();
                            }
                            arm_cpu.pc = address;
                            break;
                        }
                    }
                    break;
                default: // Load from Literal Pool
                    arm_cpu.r[opcode >> 8 & 7] = arm_mem_load_word(((arm_cpu.pc >> 2) + (opcode & 0xFF)) << 2);
                    break;
            }
            break;
        case 5:
            switch (opcode >> 9 & 7) {
                case 0: // Store Register
                    arm_mem_store_word(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 1: // Store Register Halfword
                    arm_mem_store_half(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 2: // Store Register Byte
                    arm_mem_store_byte(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 3: // Load Register Signed Byte
                    arm_cpu.r[opcode >> 0 & 7] = (int8_t)arm_mem_load_byte(arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 4: // Load Register
                    arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_word(arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 5: // Load Register Halfword
                    arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_half(arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 6: // Load Register Byte
                    arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_byte(arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
                case 7: // Load Register Signed Halfword
                    arm_cpu.r[opcode >> 0 & 7] = (int16_t)arm_mem_load_half(arm_cpu.r[opcode >> 3 & 7] + arm_cpu.r[opcode >> 6 & 7]);
                    break;
            }
            break;
        case 6:
            if (opcode >> 11 & 1) { // Load Register
                arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_word(arm_cpu.r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 2));
            } else { // Store Register
                arm_mem_store_word(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 2));
            }
            break;
        case 7:
            if (opcode >> 11 & 1) { // Load Register Byte
                arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_byte(arm_cpu.r[opcode >> 3 & 7] + (opcode >> 6 & 0x1F));
            } else { // Store Register Byte
                arm_mem_store_byte(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + (opcode >> 6 & 0x1F));
            }
            break;
        case 8:
            if (opcode >> 11 & 1) { // Load Register Halfword
                arm_cpu.r[opcode >> 0 & 7] = arm_mem_load_half(arm_cpu.r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 1));
            } else { // Store Register Halfword
                arm_mem_store_half(arm_cpu.r[opcode >> 0 & 7], arm_cpu.r[opcode >> 3 & 7] + ((opcode >> 6 & 0x1F) << 1));
            }
            break;
        case 9:
            if (opcode >> 11 & 1) { // Load Register SP relative
                arm_cpu.r[opcode >> 8 & 7] = arm_mem_load_word(arm_cpu.sp + ((opcode >> 0 & 0xFF) << 2));
            } else { // Store Register SP relative
                arm_mem_store_word(arm_cpu.r[opcode >> 8 & 7], arm_cpu.sp + ((opcode >> 0 & 0xFF) << 2));
            }
            break;
        case 10: // Generate SP/PC
            arm_cpu.r[opcode >> 8 & 7] = arm_cpu.r[opcode >> 11 & 1 ? 13 : 15] + ((opcode >> 0 & 0xFF) << 2);
            break;
        case 11: // Miscellaneous 16-bit instructions
            switch (opcode >> 9 & 7) {
                case 0:
                    switch (opcode >> 7 & 3) {
                        case 0: // Add Immediate to SP
                            arm_cpu.sp += (opcode & 0x7F) << 2;
                            break;
                        case 1: // Subtract Immediate from SP
                            arm_cpu.sp -= (opcode & 0x7F) << 2;
                            break;
                    }
                    break;
                case 1:
                    switch (opcode >> 6 & 7) {
                        case 0: // Signed Extend Halfword
                            arm_cpu.r[opcode >> 0 & 7] = (int16_t)arm_cpu.r[opcode >> 3 & 7];
                            break;
                        case 1: // Signed Extend Byte
                            arm_cpu.r[opcode >> 0 & 7] = (int8_t)arm_cpu.r[opcode >> 3 & 7];
                            break;
                        case 2: // Unsigned Extend Halfword
                            arm_cpu.r[opcode >> 0 & 7] = (uint16_t)arm_cpu.r[opcode >> 3 & 7];
                            break;
                        case 3: // Unsigned Extend Byte
                            arm_cpu.r[opcode >> 0 & 7] = (uint8_t)arm_cpu.r[opcode >> 3 & 7];
                            break;
                    }
                    break;
                case 2: { // Push Multiple Registers
                    uint32_t address = arm_cpu.sp -= (arm_bitcount_9(opcode) << 2);
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_mem_store_word(arm_cpu.r[i], address);
                            address += 4;
                        }
                    }
                    if (opcode >> i & 1) {
                        arm_mem_store_word(arm_cpu.lr, address);
                    }
                    break;
                }
                case 3:
                    switch (opcode >> 5 & 0xF) {
                        case 3: // Change Processor State
                            arm_cpu.pm = opcode >> 4 & 1;
                            break;
                    }
                    break;
                case 5:
                    switch (opcode >> 6 & 7) {
                        case 0: // Byte-Reverse Word
                            arm_cpu.r[opcode >> 0 & 7] = arm_rev(arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 1: // Byte-Reverse Packed Halfword
                            arm_cpu.r[opcode >> 0 & 7] = arm_rev16(arm_cpu.r[opcode >> 3 & 7]);
                            break;
                        case 3: // Byte-Reverse Signed Halfword
                            arm_cpu.r[opcode >> 0 & 7] = arm_revsh(arm_cpu.r[opcode >> 3 & 7]);
                            break;
                    }
                    break;
                case 6: { // Pop Multiple Registers
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_cpu.r[i] = arm_mem_load_word(arm_cpu.sp);
                            arm_cpu.sp += 4;
                        }
                    }
                    if (opcode >> i & 1) {
                        arm_cpu.pc = arm_mem_load_word(arm_cpu.sp) - 1;
                        arm_cpu.sp += 4;
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
                    uint32_t address = arm_cpu.r[opcode >> 8 & 7];
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_mem_store_word(arm_cpu.r[i], address);
                            address += 4;
                        }
                    }
                    arm_cpu.r[opcode >> 8 & 7] = address;
                    break;
                }
                case 1: { // Load multiple registers
                    uint32_t address = arm_cpu.r[opcode >> 8 & 7];
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opcode >> i & 1) {
                            arm_cpu.r[i] = arm_mem_load_word(address);
                            address += 4;
                        }
                    }
                    if (!(opcode >> (opcode >> 8 & 7) & 1)) {
                        arm_cpu.r[opcode >> 8 & 7] = address;
                    }
                    break;
                }
            }
            break;
        case 13:
            switch (opcode >> 8 & 0xF) { // Conditional branch, and Supervisor Call
                case 0: // Branch Equal
                    if (arm_cpu.z) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 1: // Branch Not equal
                    if (!arm_cpu.z) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 2: // Branch Carry set
                    if (arm_cpu.c) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 3: // Branch Carry clear
                    if (!arm_cpu.c) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 4: // Branch Minus, negative
                    if (arm_cpu.n) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 5: // Branch Plus, positive or zero
                    if (!arm_cpu.n) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 6: // Branch Overflow
                    if (arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 7: // Branch No overflow
                    if (!arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 8: // Branch Unsigned higher
                    if (arm_cpu.c && !arm_cpu.z) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 9: // Branch Unsigned lower or same
                    if (!arm_cpu.c || arm_cpu.z) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 10: // Branch Signed greater than or equal
                    if (arm_cpu.n == arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 11: // Branch Signed less than
                    if (arm_cpu.n != arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 12: // Branch Signed greater than
                    if (!arm_cpu.z && arm_cpu.n == arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
                    }
                    break;
                case 13: // Branch Signed less than or equal
                    if (arm_cpu.z || arm_cpu.n != arm_cpu.v) {
                        arm_cpu.pc += (int32_t)opcode << 24 >> 23;
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
                    arm_cpu.pc += (int32_t)opcode << 21 >> 20;
                    break;
                default: // UNDEFINED 32-bit Thumb instruction
                    opcode = opcode << 16 | arm_mem_load_half(arm_cpu.pc - 4);
                    arm_cpu.pc += 2;
                    arm_exception(ARM_Exception_HardFault);
                    break;
            }
            break;
        case 15: // 32-bit Thumb instruction.
            opcode = opcode << 16 | arm_mem_load_half(arm_cpu.pc - 4);
            arm_cpu.pc += 2;
            if (!(opcode >> 27 & 1) && (opcode >> 15 & 1)) {
                switch (opcode >> 20 & 0x7F) {
                    case 0x38:
                    case 0x39: { // Move to Special Register
                        uint32_t value = arm_cpu.r[opcode >> 16 & 0xF];
                        switch (opcode >> 0 & 0xFF) {
                            case 0x00:
                            case 0x01:
                            case 0x02:
                            case 0x03:
                                arm_cpu.v = value >> 28 & 1;
                                arm_cpu.c = value >> 29 & 1;
                                arm_cpu.z = value >> 30 & 1;
                                arm_cpu.n = value >> 31 & 1;
                                break;
                            case 0x80:
                            case 0x81:
                                *((opcode >> 0 & 1) == arm_cpu.spsel ? &arm_cpu.sp : &arm_cpu.altsp) = value >> 0 & ~3;
                                break;
                            case 0x10:
                                arm_cpu.pm = value >> 0 & 1;
                                break;
                            case 0x14:
                                if (arm_cpu.mode && arm_cpu.spsel != (value >> 1 & 1)) {
                                    uint32_t sp = arm_cpu.sp;
                                    arm_cpu.sp = arm_cpu.altsp;
                                    arm_cpu.altsp = sp;
                                    arm_cpu.spsel = value >> 1 & 1;
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
                                    value |= arm_cpu.excNum << 0;
                                }
                                if (!(opcode >> 2 & 1)) {
                                    value |= arm_cpu.v << 28;
                                    value |= arm_cpu.c << 29;
                                    value |= arm_cpu.z << 30;
                                    value |= arm_cpu.n << 31;
                                }
                                break;
                            case 0x08:
                            case 0x09:
                                value = (opcode >> 0 & 1) == arm_cpu.spsel ? arm_cpu.sp : arm_cpu.altsp;
                                break;
                            case 0x10:
                                value |= arm_cpu.pm;
                                break;
                            case 0x14:
                                value |= arm_cpu.spsel << 1;
                                break;
                        }
                        arm_cpu.r[opcode >> 16 & 0xF] = value;
                        break;
                    }
                    default:
                        arm_exception(ARM_Exception_HardFault);
                        break;
                }
            } else if ((opcode >> 1 & 5) == 5) { // Branch with Link
                arm_cpu.lr = arm_cpu.pc - 1;
                arm_cpu.pc += ((int32_t)opcode << 5 >> 7 & UINT32_C(0xFF000000)) |
                    (~(opcode >> 3 ^ opcode << 10) & UINT32_C(0x00800000)) |
                    (~(opcode >> 4 ^ opcode << 11) & UINT32_C(0x00400000)) |
                    (opcode >> 4 & UINT32_C(0x003FF000)) |
                    (opcode << 1 & UINT32_C(0x00000FFE));
            } else { // UNDEFINED 32-bit Thumb instruction
                arm_exception(ARM_Exception_HardFault);
            }
            break;
    }
}

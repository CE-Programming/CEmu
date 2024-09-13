#include "armcpu.h"

#include "armmem.h"
#include "armstate.h"
#include "../defines.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>
#if defined(_MSC_VER) && !defined(__clang__)
#include <intrin.h>
#endif

#ifndef __has_builtin
# define __has_builtin(builtin) 0
#endif

#if defined(__aarch64__) && defined(__APPLE__)
# define DEBUG_BREAK __builtin_trap()
#elif defined(_MSC_VER)
# define DEBUG_BREAK __debugbreak()
#else
# define DEBUG_BREAK asm("int3")
#endif

#if (defined(__GNUC__) || defined(__clang__))
# define EXTENDED_ASM 1
#else
# define EXTENDED_ASM 0
#endif

#if defined(__GCC_ASM_FLAG_OUTPUTS__) &&       \
    (defined(__i386) || defined(__x86_64__) || \
     defined(_M_IX86) || defined(_M_X64))
# define FLAGS_FROM_EXTENDED_X86_ASM 1
#else
# define FLAGS_FROM_EXTENDED_X86_ASM 0
#endif

#if (__has_builtin(__builtin_add_overflow) && __has_builtin(__builtin_sub_overflow)) || __GNUC__ >= 5
# define FLAGS_FROM_OVERFLOW_BUILTINS 1
#else
# define FLAGS_FROM_OVERFLOW_BUILTINS 0
#endif

#if _MSC_VER >= 1937 && !defined(__clang__) && (defined(_M_IX86) || defined(_M_X64))
# define FLAGS_FROM_MSVC_INTRINSICS 1
#else
# define FLAGS_FROM_MSVC_INTRINSICS 0
#endif

#if __has_builtin(__builtin_constant_p) || __GNUC__ >= 3 // Not sure, so conservative
# define HAVE_BUILTIN_CONSTANT_P 1
#else
# define HAVE_BUILTIN_CONSTANT_P 0
#endif

static uint8_t bitcount9(uint32_t x) {
#if __has_builtin(__builtin_popcount) || __GNUC__ >= 4
    return __builtin_popcount(x & 0777);
#elif UINT32_MAX >= UINTPTR_MAX
    uint32_t res = (x &= 0777), mask = UINT32_C(0x11111111);
    res *= UINT32_C(0001001001001);
    res >>= 3;
# if EXTENDED_ASM && (defined(__i386__) || defined(_M_IX86))
    __asm__("andl\t%1, %0\nimull\t%1, %0" : "+r"(res) : "r"(mask) : "cc");
# else
    res &= mask;
    res *= mask;
#endif
    return (res >> 28) + (x >> 8);
#else
    uint64_t res = x & 0777, mask = UINT64_C(0x1111111111111111);
    res *= UINT64_C(0001001001001);
# if EXTENDED_ASM && (defined(__x86_64__) || defined(_M_X64))
    __asm__("andq\t%1, %0\nimulq\t%1, %0" : "+r"(res) : "r"(mask) : "cc");
# else
    res &= mask;
    res *= mask;
# endif
    return res >> 60;
#endif
}

static uint8_t lowestsetbit32(uint32_t x) {
    assert(x && "invalid argument");
#if __has_builtin(__builtin_ctz) || __GNUC__ >= 4
    return __builtin_ctz(x);
#elif defined(_MSC_VER) && !defined(__clang__)
    unsigned long index;
    _BitScanForward(&index, x);
    return index;
#else
# if EXTENDED_ASM && \
    (defined(__i386) || defined(__x86_64__) || \
     defined(_M_IX86) || defined(_M_X64))
    uint32_t res;
    __asm__("bsfl\t%1, %0" : "+r"(res) : "r"(x) : "cc");
# else
    uint8_t res = 0;
    x &= -x;
    if (x & UINT32_C(0xAAAAAAAA)) res += 1;
    if (x & UINT32_C(0xCCCCCCCC)) res += 2;
    if (x & UINT32_C(0xF0F0F0F0)) res += 4;
    if (x & UINT32_C(0xFF00FF00)) res += 8;
    if (x & UINT32_C(0xFFFF0000)) res += 16;
# endif
    return res;
#endif
}

void arm_cpu_reset(arm_t *arm) {
    arm_cpu_t *cpu = &arm->cpu;
    memset(cpu, 0, sizeof(*cpu));
    cpu->sp = arm_mem_load_word(arm, cpu->scb.vtor + 0);
    if (cpu->exc) {
        cpu->exc = false;
        return;
    }
    cpu->pc = arm_mem_load_word(arm, cpu->scb.vtor + 4) + 1;
    if (cpu->exc) {
        cpu->exc = false;
        return;
    }
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
    int32_t res;
    cpu->v = __builtin_sub_overflow(0, (int32_t)x, &res);
    cpu->c = !res;
    return arm_movs(cpu, res);
#elif FLAGS_FROM_MSVC_INTRINSICS
    int32_t res;
    cpu->v = _sub_overflow_i32(0, 0, x, &res);
    cpu->c = !res;
    return arm_movs(cpu, res);
#else
    uint32_t res = -x;
    cpu->v = (x & res) >> 31;
    cpu->c = !res;
    return arm_movs(cpu, res);
#endif
}

static uint32_t arm_adds(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("addl\t%5, %0" : "+r"(x), "=@cco"(cpu->v), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t res;
    cpu->v = __builtin_add_overflow((int32_t)x, (int32_t)y, &res);
    cpu->c = __builtin_add_overflow(x, y, &x);
    return arm_movs(cpu, x);
#elif FLAGS_FROM_MSVC_INTRINSICS
    int32_t res;
    cpu->v = _add_overflow_i32(0, x, y, &res);
    cpu->c = _addcarry_u32(0, x, y, &x);
    return arm_movs(cpu, x);
#else
    uint32_t res = x + y;
    cpu->v = ((res ^ x) & (res ^ y)) >> 31;
    cpu->c = res < x;
    //int64_t res = (int64_t)(int32_t)x + (int32_t)y;
    //cpu->v = res != (int32_t)res;
    //cpu->c = (uint32_t)res < x;
    //cpu->c = x > ~y;
    return arm_movs(cpu, res);
#endif
}

static uint32_t arm_subs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("subl\t%5, %0" : "+r"(x), "=@cco"(cpu->v), "=@ccnc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    int32_t res;
    cpu->v = __builtin_sub_overflow((int32_t)x, (int32_t)y, &res);
    cpu->c = !__builtin_sub_overflow(x, y, &x);
    return arm_movs(cpu, x);
#elif FLAGS_FROM_MSVC_INTRINSICS
    int32_t res;
    cpu->v = _sub_overflow_i32(0, x, y, &res);
    cpu->c = !_subborrow_u32(0, x, y, &x);
    return arm_movs(cpu, x);
#else
    uint32_t res = x - y;
    cpu->v = ((x ^ y) & (res ^ x)) >> 31;
    cpu->c = res <= x;
    //int64_t res = (int64_t)(int32_t)x - (int32_t)y;
    //cpu->v = res != (int32_t)res;
    //cpu->c = (uint32_t)res <= x;
    //cpu->c = x >= y;
    return arm_movs(cpu, res);
#endif
}

static uint32_t arm_adcs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tbtl\t$0, %k6\n\tadcl\t%5, %0\n" : "+r"(x), "=@cco"(cpu->v), "=@ccc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y), "m"(cpu->c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool carry = cpu->c;
    int32_t res;
    cpu->v = __builtin_add_overflow(x, y, &res);
    cpu->v ^= __builtin_add_overflow(res, carry, &res);
    cpu->c = __builtin_add_overflow(x, y, &x);
    cpu->c |= __builtin_add_overflow(x, carry, &x);
    return arm_movs(cpu, x);
#elif FLAGS_FROM_MSVC_INTRINSICS
    bool carry = cpu->c;
    int32_t res;
    cpu->v = _add_overflow_i32(carry, x, y, &res);
    cpu->c = _addcarry_u32(carry, x, y, &x);
    return arm_movs(cpu, x);
#else
    uint32_t res = x + y + cpu->c;
    uint32_t carries = (x | y) ^ ((x ^ y) & res);
    cpu->c = carries >> 31;
    cpu->v = cpu->c ^ (carries >> 30 & 1);
    //int64_t res = (uint64_t)(int32_t)x + (int32_t)y + cpu->c;
    //cpu->v = res != (int32_t)res;
    //cpu->c = ((uint64_t)x + y + cpu->c) >> 32;
    return arm_movs(cpu, res);
#endif
}

static uint32_t arm_sbcs(arm_cpu_t *cpu, uint32_t x, uint32_t y) {
#if FLAGS_FROM_EXTENDED_X86_ASM
    __asm__("\tcmpb\t$1, %6\n\tsbbl\t%5, %0\n" : "+r"(x), "=@cco"(cpu->v), "=@ccnc"(cpu->c), "=@ccz"(cpu->z), "=@ccs"(cpu->n) : "ir"(y), "m"(cpu->c) : "cc");
    return x;
#elif FLAGS_FROM_OVERFLOW_BUILTINS
    bool borrow = !cpu->c;
    int32_t res;
    cpu->v = __builtin_sub_overflow(x, y, &res);
    cpu->v ^= __builtin_sub_overflow(res, borrow, &res);
    cpu->c = !__builtin_sub_overflow(x, y, &x);
    cpu->c &= !__builtin_sub_overflow(x, borrow, &x);
    return arm_movs(cpu, x);
#elif FLAGS_FROM_MSVC_INTRINSICS
    bool borrow = !cpu->c;
    int32_t res;
    cpu->v = _sub_overflow_i32(borrow, x, y, &res);
    cpu->c = !_subborrow_u32(borrow, x, y, &x);
    return arm_movs(cpu, x);
#else
    return arm_adcs(cpu, x, ~y);
    //int64_t res = (uint64_t)(int32_t)x - (int32_t)y - !cpu->c;
    //cpu->v = res != (int32_t)res;
    //cpu->c = !(((uint64_t)x - y - !cpu->c) >> 32);
    //return arm_movs(cpu, res);
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

static void arm_cpu_tick(arm_t *arm) {
    arm_systick_t *systick = &arm->cpu.systick;
    if (likely(systick->ctrl & SysTick_CTRL_ENABLE_Msk)) {
        if (unlikely(!systick->val)) {
            systick->val = systick->load;
        } else if (unlikely(!--systick->val)) {
            systick->ctrl |= SysTick_CTRL_COUNTFLAG_Msk;
            if (likely(systick->ctrl & SysTick_CTRL_TICKINT_Msk)) {
                arm->cpu.scb.icsr |= SCB_ICSR_PENDSTSET_Msk;
            }
        }
    }
}

bool arm_cpu_exception(arm_t *arm, arm_exception_number_t exc) {
    arm_cpu_t *cpu = &arm->cpu;
    arm_exception_number_t curexc =
        (cpu->scb.icsr & SCB_ICSR_VECTACTIVE_Msk) >>
        SCB_ICSR_VECTACTIVE_Pos;
    uint32_t sp = cpu->sp;
    bool align = sp >> 2 & 1;
    assert(exc > ARM_Thread_Mode &&
           exc < ARM_Invalid_Exception &&
           "Invalid exception");
    if (cpu->active & UINT64_C(1) << exc) {
        return false;
    }
    if (arm->debug && exc == ARM_Exception_HardFault) {
        DEBUG_BREAK;
    }
    if (cpu->exc) {
        cpu->exc = false;
        cpu->pc = 0;
        if (arm->debug) {
            DEBUG_BREAK;
        }
        return false;
    }
    arm_cpu_tick(arm);
    cpu->exc = true;
    sp -= 0x20;
    sp &= ~7;
    arm_mem_store_word(arm, cpu->r0, sp + 0x00);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->r1, sp + 0x04);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->r2, sp + 0x08);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->r3, sp + 0x0C);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->ip, sp + 0x10);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->lr, sp + 0x14);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->pc - 2, sp + 0x18);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    arm_mem_store_word(arm, cpu->n         << 31 |
                            cpu->z         << 30 |
                            cpu->c         << 29 |
                            cpu->v         << 28 |
                            (~cpu->pc & 1) << 24 |
                            align          <<  9 |
                            curexc         <<  0,
                       sp + 0x1C);
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    uint32_t pc = arm_mem_load_word(arm, cpu->scb.vtor + (exc << 2)) + 1;
    if (!pc || pc >= UINT32_C(0x80000000)) {
        DEBUG_BREAK;;
    }
    if (!cpu->exc) {
        cpu->exc = true;
        return false;
    }
    if (unlikely(curexc != ARM_Thread_Mode)) {
        cpu->lr = -15;
        cpu->sp = sp;
    } else if (unlikely(cpu->spsel)) {
        cpu->lr = -3;
        cpu->sp = cpu->altsp;
        cpu->altsp = sp;
        cpu->spsel = false;
    } else {
        cpu->lr = -7;
        cpu->sp = sp;
    }
    cpu->scb.icsr = (cpu->scb.icsr & ~SCB_ICSR_VECTACTIVE_Msk) |
        (exc & SCB_ICSR_VECTACTIVE_Msk) << SCB_ICSR_VECTACTIVE_Pos;
    cpu->active |= UINT64_C(1) << exc;
    cpu->pc = pc;
    return true;
}

static void arm_cpu_interwork(arm_t *arm, uint32_t addr) {
    arm_cpu_t *cpu = &arm->cpu;
    arm_exception_number_t curexc =
        (cpu->scb.icsr & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    if (likely(curexc == ARM_Thread_Mode || addr >> 28 != 0xF)) {
        cpu->pc = addr + 1;
    } else {
        // Exception Return
        uint32_t sp, val;
        assert(!~(addr | 0xF) && cpu->active & UINT64_C(1) << curexc &&
               "Unpredictable exception return");
        cpu->active &= ~(1 << curexc);
        switch (addr) {
            case -15:
                assert(cpu->active && "Unpredictable exception return");
                sp = cpu->sp;
                cpu->spsel = false;
                break;
            case -7:
                assert(!cpu->active && "Unpredictable exception return");
                sp = cpu->sp;
                cpu->spsel = false;
                break;
            case -3:
                assert(!cpu->active && "Unpredictable exception return");
                sp = cpu->altsp;
                cpu->altsp = cpu->sp;
                cpu->spsel = true;
                break;
            default:
                assert(false && "Unpredictable exception return");
        }
        cpu->r0 = arm_mem_load_word(arm, sp + 0x00);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->r1 = arm_mem_load_word(arm, sp + 0x04);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->r2 = arm_mem_load_word(arm, sp + 0x08);
        if (cpu->exc) {
            if (unlikely(cpu->pc & 1)) {
                arm_cpu_exception(arm, ARM_Exception_HardFault);
                cpu->exc = false;
                return;
            }
            cpu->exc = false;
            return;
        }
        cpu->r3 = arm_mem_load_word(arm, sp + 0x0C);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->ip = arm_mem_load_word(arm, sp + 0x10);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->lr = arm_mem_load_word(arm, sp + 0x14);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->pc = arm_mem_load_word(arm, sp + 0x18) + 1;
        if (!cpu->pc || cpu->pc >= UINT32_C(0x80000000)) {
            DEBUG_BREAK;
        }
        assert(cpu->pc & 1 && "Unpredictable exception return");
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        val = arm_mem_load_word(arm, sp + 0x1C);
        if (cpu->exc) {
            cpu->exc = false;
            return;
        }
        cpu->n = val >> 31 & 1;
        cpu->z = val >> 30 & 1;
        cpu->c = val >> 29 & 1;
        cpu->v = val >> 28 & 1;
        cpu->pc += val >> 24 & 1;
        cpu->sp = (sp + 0x20) | (val >> 7 & 4);
        curexc = (val & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
        switch (addr) {
            case -15:
                assert(curexc != ARM_Thread_Mode && "Unpredictable exception return");
                cpu->scb.icsr = (cpu->scb.icsr & ~SCB_ICSR_VECTACTIVE_Msk) |
                    curexc << SCB_ICSR_VECTACTIVE_Pos;
                break;
            case -7:
            case -3:
                assert(curexc == ARM_Thread_Mode && "Unpredictable exception return");
                cpu->scb.icsr &= ~SCB_ICSR_VECTACTIVE_Msk;
                break;
        }
    }
}

void arm_cpu_execute(arm_t *arm) {
    arm_cpu_t *cpu = &arm->cpu;
    uint32_t icsr = cpu->scb.icsr, pc = cpu->pc - 2, opc, val;
    if (arm->debug && pc >= UINT32_C(0x80000000)) {
        DEBUG_BREAK;
    }
    uint8_t i;
    arm_exception_number_t curexc =
        (icsr & SCB_ICSR_VECTACTIVE_Msk) >> SCB_ICSR_VECTACTIVE_Pos;
    if (unlikely(cpu->pc & 1)) {
        arm_cpu_exception(arm, ARM_Exception_HardFault);
        cpu->exc = false;
        return;
    }
    if (unlikely(!cpu->pm &&
                 (icsr & (SCB_ICSR_NMIPENDSET_Msk |
                          SCB_ICSR_PENDSVSET_Msk |
                          SCB_ICSR_PENDSTSET_Msk) ||
                  cpu->nvic.ipr & cpu->nvic.ier))) {
        if (icsr & SCB_ICSR_NMIPENDSET_Msk) {
            if (likely(arm_cpu_exception(arm, ARM_Exception_NMI))) {
                cpu->scb.icsr &= ~SCB_ICSR_NMIPENDSET_Msk;
                cpu->exc = false;
                return;
            }
        } else if (icsr & SCB_ICSR_PENDSVSET_Msk) {
            if (likely(arm_cpu_exception(arm, ARM_Exception_PendSV))) {
                cpu->scb.icsr &= ~SCB_ICSR_PENDSVSET_Msk;
                cpu->exc = false;
                return;
            }
        } else if (icsr & SCB_ICSR_PENDSTSET_Msk) {
            if (likely(arm_cpu_exception(arm, ARM_Exception_SysTick))) {
                cpu->scb.icsr &= ~SCB_ICSR_PENDSTSET_Msk;
                cpu->exc = false;
                return;
            }
        } else {
            uint8_t src = lowestsetbit32(cpu->nvic.ipr & cpu->nvic.ier);
            if (likely(arm_cpu_exception(arm, ARM_Exception_External + src))) {
                arm_mem_update_pending(arm);
                cpu->exc = false;
                return;
            }
        }
        if (unlikely(cpu->exc)) {
            cpu->exc = false;
            return;
        }
    }
    arm_cpu_tick(arm);
    opc = arm_mem_load_half(arm, pc);
    if (unlikely(cpu->exc)) {
        cpu->exc = false;
        return;
    }
    cpu->pc += 2;
    //arm->debug |= pc == UINT32_C(0x10E00);
    if (arm->debug) {
        fprintf(stderr, "PC %08X %04X\n", pc, opc);
        if (!arm->sync.slp) {
            DEBUG_BREAK;
        }
        //if (pc == UINT32_C(0x000010BC) ||
        //    pc == UINT32_C(0x000010CA) ||
        //    pc == UINT32_C(0x000010F6) ||
        //    pc == UINT32_C(0x00001104)) DEBUG_BREAK;
    }
    switch (opc >> 12 & 0xF) {
        case 0:
        case 1:
            switch (opc >> 11 & 3) {
                case 0: // Logical Shift Left
                    cpu->r[opc >> 0 & 7] = arm_lsls(cpu, cpu->r[opc >> 3 & 7], opc >> 6 & 0x1F);
                    break;
                case 1: // Logical Shift Right
                    cpu->r[opc >> 0 & 7] = arm_lsrs(cpu, cpu->r[opc >> 3 & 7], (((opc >> 6) - 1) & 0x1F) + 1);
                    break;
                case 2: // Arithmetic Shift Right
                    cpu->r[opc >> 0 & 7] = arm_asrs(cpu, cpu->r[opc >> 3 & 7], (((opc >> 6) - 1) & 0x1F) + 1);
                    break;
                case 3:
                    switch (opc >> 9 & 3) {
                        case 0: // Add register
                            cpu->r[opc >> 0 & 7] = arm_adds(cpu, cpu->r[opc >> 3 & 7], cpu->r[opc >> 6 & 7]);
                            break;
                        case 1: // Subtract register
                            cpu->r[opc >> 0 & 7] = arm_subs(cpu, cpu->r[opc >> 3 & 7], cpu->r[opc >> 6 & 7]);
                            break;
                        case 2: // Add 3-bit immediate
                            cpu->r[opc >> 0 & 7] = arm_adds(cpu, cpu->r[opc >> 3 & 7], opc >> 6 & 7);
                            break;
                        case 3: // Subtract 3-bit immediate
                            cpu->r[opc >> 0 & 7] = arm_subs(cpu, cpu->r[opc >> 3 & 7], opc >> 6 & 7);
                            break;
                    }
                    break;
            }
            break;
        case 2:
        case 3:
            switch (opc >> 11 & 3) {
                case 0: // Move
                    cpu->r[opc >> 8 & 7] = arm_movs(cpu, opc >> 0 & 0xFF);
                    break;
                case 1: // Compare
                    arm_subs(cpu, cpu->r[opc >> 8 & 7], opc >> 0 & 0xFF);
                    break;
                case 2: // Add 8-bit immediate
                    cpu->r[opc >> 8 & 7] = arm_adds(cpu, cpu->r[opc >> 8 & 7], opc >> 0 & 0xFF);
                    break;
                case 3: // Subtract 8-bit immediate
                    cpu->r[opc >> 8 & 7] = arm_subs(cpu, cpu->r[opc >> 8 & 7], opc >> 0 & 0xFF);
                    break;
            }
            break;
        case 4:
            switch (opc >> 10 & 3) {
                case 0: // Data processing
                    switch (opc >> 6 & 0xF) {
                        case 0: // Bitwise AND
                            cpu->r[opc >> 0 & 7] = arm_ands(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 1: // Exclusive OR
                            cpu->r[opc >> 0 & 7] = arm_eors(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 2: // Logical Shift Left
                            cpu->r[opc >> 0 & 7] = arm_lsls(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 3: // Logical Shift Right
                            cpu->r[opc >> 0 & 7] = arm_lsrs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 4: // Arithmetic Shift Right
                            cpu->r[opc >> 0 & 7] = arm_asrs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 5: // Add with Carry
                            cpu->r[opc >> 0 & 7] = arm_adcs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 6: // Subtract with Carry
                            cpu->r[opc >> 0 & 7] = arm_sbcs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 7: // Rotate Right
                            cpu->r[opc >> 0 & 7] = arm_rors(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 8: // Set flags on bitwise AND
                            arm_ands(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 9: // Reverse Subtract from 0
                            cpu->r[opc >> 0 & 7] = arm_negs(cpu, cpu->r[opc >> 3 & 7]);
                            break;
                        case 10: // Compare Registers
                            arm_subs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 11: // Compare Negative
                            arm_adds(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 12: // Logical OR
                            cpu->r[opc >> 0 & 7] = arm_orrs(cpu, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7]);
                            break;
                        case 13: // Multiply Two Registers
                            cpu->r[opc >> 0 & 7] = arm_movs(cpu, cpu->r[opc >> 0 & 7] * cpu->r[opc >> 3 & 7]);
                            break;
                        case 14: // Bit Clear
                            cpu->r[opc >> 0 & 7] = arm_ands(cpu, cpu->r[opc >> 0 & 7], ~cpu->r[opc >> 3 & 7]);
                            break;
                        case 15: // Bitwise NOT
                            cpu->r[opc >> 0 & 7] = arm_mvns(cpu, cpu->r[opc >> 3 & 7]);
                            break;
                    }
                    break;
                case 1: // Special data instructions and branch and exchange
                    switch (opc >> 8 & 3) {
                        case 0: // Add Registers
                            i = (opc >> 4 & 8) | (opc >> 0 & 7);
                            cpu->r[i] += cpu->r[opc >> 3 & 0xF];
                            if (unlikely(i == 15)) {
                                cpu->pc = (cpu->pc | 1) + 1;
                            }
                            break;
                        case 1: // Compare Registers
                            arm_subs(cpu, cpu->r[(opc >> 4 & 8) | (opc >> 0 & 7)], cpu->r[opc >> 3 & 0xF]);
                            break;
                        case 2: // Move Registers
                            i = (opc >> 4 & 8) | (opc >> 0 & 7);
                            cpu->r[i] = cpu->r[opc >> 3 & 0xF];
                            if (unlikely(i == 15)) {
                                cpu->pc = (cpu->pc | 1) + 1;
                            }
                            break;
                        case 3: // Branch (with Link) and Exchange
                            val = cpu->r[opc >> 3 & 0xF];
                            if (unlikely(opc >> 7 & 1)) { // Branch with Link and Exchange
                                cpu->lr = cpu->pc - 1;
                                cpu->pc = val + 1;
                            } else {
                                arm_cpu_interwork(arm, val);
                            }
                            break;
                    }
                    break;
                default: // Load from Literal Pool
                    cpu->r[opc >> 8 & 7] = arm_mem_load_word(arm, ((cpu->pc >> 2) + (opc & 0xFF)) << 2);
                    break;
            }
            break;
        case 5:
            switch (opc >> 9 & 7) {
                case 0: // Store Register
                    arm_mem_store_word(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 1: // Store Register Halfword
                    arm_mem_store_half(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 2: // Store Register Byte
                    arm_mem_store_byte(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 3: // Load Register Signed Byte
                    cpu->r[opc >> 0 & 7] = (int8_t)arm_mem_load_byte(arm, cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 4: // Load Register
                    cpu->r[opc >> 0 & 7] = arm_mem_load_word(arm, cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 5: // Load Register Halfword
                    cpu->r[opc >> 0 & 7] = arm_mem_load_half(arm, cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 6: // Load Register Byte
                    cpu->r[opc >> 0 & 7] = arm_mem_load_byte(arm, cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
                case 7: // Load Register Signed Halfword
                    cpu->r[opc >> 0 & 7] = (int16_t)arm_mem_load_half(arm, cpu->r[opc >> 3 & 7] + cpu->r[opc >> 6 & 7]);
                    break;
            }
            break;
        case 6:
            if (opc >> 11 & 1) { // Load Register
                cpu->r[opc >> 0 & 7] = arm_mem_load_word(arm, cpu->r[opc >> 3 & 7] + ((opc >> 6 & 0x1F) << 2));
            } else { // Store Register
                arm_mem_store_word(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + ((opc >> 6 & 0x1F) << 2));
            }
            break;
        case 7:
            if (opc >> 11 & 1) { // Load Register Byte
                cpu->r[opc >> 0 & 7] = arm_mem_load_byte(arm, cpu->r[opc >> 3 & 7] + (opc >> 6 & 0x1F));
            } else { // Store Register Byte
                arm_mem_store_byte(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + (opc >> 6 & 0x1F));
            }
            break;
        case 8:
            if (opc >> 11 & 1) { // Load Register Halfword
                cpu->r[opc >> 0 & 7] = arm_mem_load_half(arm, cpu->r[opc >> 3 & 7] + ((opc >> 6 & 0x1F) << 1));
            } else { // Store Register Halfword
                arm_mem_store_half(arm, cpu->r[opc >> 0 & 7], cpu->r[opc >> 3 & 7] + ((opc >> 6 & 0x1F) << 1));
            }
            break;
        case 9:
            if (opc >> 11 & 1) { // Load Register SP relative
                cpu->r[opc >> 8 & 7] = arm_mem_load_word(arm, cpu->sp + ((opc >> 0 & 0xFF) << 2));
            } else { // Store Register SP relative
                arm_mem_store_word(arm, cpu->r[opc >> 8 & 7], cpu->sp + ((opc >> 0 & 0xFF) << 2));
            }
            break;
        case 10: // Add SP/PC relative
            cpu->r[opc >> 8 & 7] = (opc >> 11 & 1 ? cpu->sp : cpu->pc & ~2) + ((opc >> 0 & 0xFF) << 2);
            break;
        case 11: // Miscellaneous 16-bit instructions
            switch (opc >> 9 & 7) {
                case 0:
                    switch (opc >> 7 & 3) {
                        case 0: // Add Immediate to SP
                            cpu->sp += (opc & 0x7F) << 2;
                            break;
                        case 1: // Subtract Immediate from SP
                            cpu->sp -= (opc & 0x7F) << 2;
                            break;
                    }
                    break;
                case 1:
                    switch (opc >> 6 & 7) {
                        case 0: // Signed Extend Halfword
                            cpu->r[opc >> 0 & 7] = (int16_t)cpu->r[opc >> 3 & 7];
                            break;
                        case 1: // Signed Extend Byte
                            cpu->r[opc >> 0 & 7] = (int8_t)cpu->r[opc >> 3 & 7];
                            break;
                        case 2: // Unsigned Extend Halfword
                            cpu->r[opc >> 0 & 7] = (uint16_t)cpu->r[opc >> 3 & 7];
                            break;
                        case 3: // Unsigned Extend Byte
                            cpu->r[opc >> 0 & 7] = (uint8_t)cpu->r[opc >> 3 & 7];
                            break;
                    }
                    break;
                case 2: // Push Multiple Registers
                    val = cpu->sp -= (bitcount9(opc) << 2);
                    int i;
                    for (i = 0; i < 8; i++) {
                        if (opc >> i & 1) {
                            arm_mem_store_word(arm, cpu->r[i], val);
                            if (unlikely(cpu->exc)) {
                                cpu->exc = false;
                                return;
                            }
                            val += 4;
                        }
                    }
                    if (opc >> i & 1) {
                        arm_mem_store_word(arm, cpu->lr, val);
                    }
                    break;
                case 3:
                    switch (opc >> 5 & 0xF) {
                        case 3: // Change Processor State
                            cpu->pm = opc >> 4 & 1;
                            break;
                    }
                    break;
                case 5:
                    switch (opc >> 6 & 7) {
                        case 0: // Byte-Reverse Word
                            cpu->r[opc >> 0 & 7] = arm_rev(cpu->r[opc >> 3 & 7]);
                            break;
                        case 1: // Byte-Reverse Packed Halfword
                            cpu->r[opc >> 0 & 7] = arm_rev16(cpu->r[opc >> 3 & 7]);
                            break;
                        case 3: // Byte-Reverse Signed Halfword
                            cpu->r[opc >> 0 & 7] = arm_revsh(cpu->r[opc >> 3 & 7]);
                            break;
                    }
                    break;
                case 6: // Pop Multiple Registers
                    for (i = 0; i < 8; i++) {
                        if (opc >> i & 1) {
                            cpu->r[i] = arm_mem_load_word(arm, cpu->sp);
                            if (unlikely(cpu->exc)) {
                                cpu->exc = false;
                                return;
                            }
                            cpu->sp += 4;
                        }
                    }
                    if (opc >> i & 1) {
                        val = arm_mem_load_word(arm, cpu->sp);
                        cpu->sp += 4;
                        arm_cpu_interwork(arm, val);
                    }
                    break;
                case 7:
                    switch (opc >> 8 & 1) {
                        case 0: // Breakpoint
                            break;
                        case 1: // Hints
                            switch (opc >> 0 & 0xF) {
                                case 0:
                                    switch (opc >> 4 & 0xF) {
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
            switch (opc >> 11 & 1) {
                case 0: // Store multiple registers
                    val = cpu->r[opc >> 8 & 7];
                    for (i = 0; i < 8; i++) {
                        if (opc >> i & 1) {
                            arm_mem_store_word(arm, cpu->r[i], val);
                            if (unlikely(cpu->exc)) {
                                cpu->exc = false;
                                return;
                            }
                            val += 4;
                        }
                    }
                    cpu->r[opc >> 8 & 7] = val;
                    break;
                case 1: // Load multiple registers
                    val = cpu->r[opc >> 8 & 7];
                    for (i = 0; i < 8; i++) {
                        if (opc >> i & 1) {
                            cpu->r[i] = arm_mem_load_word(arm, val);
                            if (unlikely(cpu->exc)) {
                                cpu->exc = false;
                                return;
                            }
                            val += 4;
                        }
                    }
                    if (!(opc >> (opc >> 8 & 7) & 1)) {
                        cpu->r[opc >> 8 & 7] = val;
                    }
                    break;
            }
            break;
        case 13:
            switch (opc >> 8 & 0xF) { // Conditional branch, and Supervisor Call
                case 0: // Branch Equal
                    if (cpu->z) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 1: // Branch Not equal
                    if (!cpu->z) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 2: // Branch Carry set
                    if (cpu->c) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 3: // Branch Carry clear
                    if (!cpu->c) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 4: // Branch Minus, negative
                    if (cpu->n) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 5: // Branch Plus, positive or zero
                    if (!cpu->n) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 6: // Branch Overflow
                    if (cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 7: // Branch No overflow
                    if (!cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 8: // Branch Unsigned higher
                    if (cpu->c && !cpu->z) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 9: // Branch Unsigned lower or same
                    if (!cpu->c || cpu->z) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 10: // Branch Signed greater than or equal
                    if (cpu->n == cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 11: // Branch Signed less than
                    if (cpu->n != cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 12: // Branch Signed greater than
                    if (!cpu->z && cpu->n == cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                case 13: // Branch Signed less than or equal
                    if (cpu->z || cpu->n != cpu->v) {
                        cpu->pc += ((int32_t)opc << 24 >> 23) + 2;
                    }
                    break;
                default: // Permanently UNDEFINED
                    arm_cpu_exception(arm, ARM_Exception_HardFault);
                    cpu->exc = false;
                    return;
                case 15: // Supervisor Call
                    arm_cpu_exception(arm, ARM_Exception_SVCall);
                    cpu->exc = false;
                    return;
            }
            break;
        case 14:
            switch (opc >> 11 & 1) {
                case 0: // Unconditional Branch
                    cpu->pc += ((int32_t)opc << 21 >> 20) + 2;
                    break;
                default: // UNDEFINED 32-bit Thumb instruction
                    arm_cpu_tick(arm);
                    opc = opc << 16 | arm_mem_load_half(arm, cpu->pc - 2);
                    if (unlikely(cpu->exc)) {
                        cpu->exc = false;
                        return;
                    }
                    cpu->pc += 2;
                    arm_cpu_exception(arm, ARM_Exception_HardFault);
                    cpu->exc = false;
                    return;
            }
            break;
        case 15: // 32-bit Thumb instruction.
            arm_cpu_tick(arm);
            opc = opc << 16 | arm_mem_load_half(arm, cpu->pc - 2);
            if (unlikely(cpu->exc)) {
                cpu->exc = false;
                return;
            }
            cpu->pc += 2;
            if (!(opc >> 27 & 1) && (opc >> 15 & 1)) {
                switch (opc >> 12 & 5) {
                    case 0:
                        switch (opc >> 20 & 0x7F) {
                            case 0x38:
                            case 0x39: // Move to Special Register
                                val = cpu->r[opc >> 16 & 0xF];
                                switch (opc >> 0 & 0xFF) {
                                    case 0x00:
                                    case 0x01:
                                    case 0x02:
                                    case 0x03:
                                        cpu->v = val >> 28 & 1;
                                        cpu->c = val >> 29 & 1;
                                        cpu->z = val >> 30 & 1;
                                        cpu->n = val >> 31 & 1;
                                        break;
                                    case 0x80:
                                    case 0x81:
                                        *((opc >> 0 & 1) == cpu->spsel ? &cpu->sp : &cpu->altsp) = val >> 0 & ~3;
                                        break;
                                    case 0x10:
                                        cpu->pm = val >> 0 & 1;
                                        break;
                                    case 0x14:
                                        if (likely(cpu->spsel != (val >> 1 & 1) &&
                                                   curexc == ARM_Thread_Mode)) {
                                            uint32_t sp = cpu->sp;
                                            cpu->sp = cpu->altsp;
                                            cpu->altsp = sp;
                                            cpu->spsel = val >> 1 & 1;
                                        }
                                        break;
                                }
                                break;
                            case 0x3B: // Miscellaneous control instructions
                                switch (opc >> 4 & 0xF) {
                                    case 4: // Data Synchronization Barrier
                                        break;
                                    case 5: // Data Memory Barrier
                                        break;
                                    case 6: // Instruction Synchronization Barrier
                                        break;
                                }
                                break;
                            case 0x3E:
                            case 0x3F: // Move from Special Register
                                val = 0;
                                switch (opc >> 0 & 0xFF) {
                                    case 0x00:
                                    case 0x01:
                                    case 0x03:
                                    case 0x04:
                                    case 0x05:
                                    case 0x06:
                                    case 0x07:
                                        if (opc >> 0 & 1) {
                                            val |= curexc << 0;
                                        }
                                        if (!(opc >> 2 & 1)) {
                                            val |= cpu->v << 28;
                                            val |= cpu->c << 29;
                                            val |= cpu->z << 30;
                                            val |= cpu->n << 31;
                                        }
                                        break;
                                    case 0x08:
                                    case 0x09:
                                        val = (opc >> 0 & 1) == cpu->spsel ? cpu->sp : cpu->altsp;
                                        break;
                                    case 0x10:
                                        val |= cpu->pm;
                                        break;
                                    case 0x14:
                                        val |= cpu->spsel << 1;
                                        break;
                                }
                                cpu->r[opc >> 8 & 0xF] = val;
                                break;
                            default:
                                arm_cpu_exception(arm, ARM_Exception_HardFault);
                                cpu->exc = false;
                                return;
                        }
                        break;
                    case 5: // Branch with Link
                        cpu->lr = cpu->pc - 1;
                        cpu->pc += ((int32_t)opc << 5 >> 7 & UINT32_C(0xFF000000)) |
                            (~(opc >> 3 ^ opc << 10) & UINT32_C(0x00800000)) |
                            (~(opc >> 4 ^ opc << 11) & UINT32_C(0x00400000)) |
                            (opc >> 4 & UINT32_C(0x003FF000)) |
                            (opc << 1 & UINT32_C(0x00000FFE));
                        break;
                }
            } else { // UNDEFINED 32-bit Thumb instruction
                arm_cpu_exception(arm, ARM_Exception_HardFault);
                cpu->exc = false;
                return;
            }
            break;
    }
    if (!cpu->pc || cpu->pc >= UINT32_C(0x80000000) ||
        cpu->pc - 2 == UINT32_C(0x00006A0C)) {
        DEBUG_BREAK;
    }
    if (unlikely(cpu->exc)) {
        cpu->exc = false;
        return;
    }
}

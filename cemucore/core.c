/*
 * Copyright (c) 2015-2021 CE Programming.
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "core.h"

#include "compiler.h"
#include "os.h"

#include <errno.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifndef CEMUCORE_NOTHREADS
static void *thread_start(void *data)
{
    cemucore_t *core = data;
#ifndef __APPLE__
    pthread_setname_np(core->thread, "cemucore");
#endif
    int debug_keypad_row = 0;
    for (int i = 1; i != 256; ++i)
    {
        core->memory.flash[0x200 + i] = i;
    }
    while (sync_loop(&core->sync))
    {
        {
            uint16_t events = atomic_exchange_explicit(&core->keypad.events[debug_keypad_row], 0,
                                                       memory_order_acquire);
            for (int press = 0; press <= 8; press += 8)
            {
                for (int col = 0; col != 8; ++col)
                {
                    if (events & 1 << (press + col))
                    {
                        fprintf(stderr, "Key (%d,%d) was %s\n", debug_keypad_row, col, press ? "pressed" : "released");
                    }
                }
            }
            core->keypad.keys[debug_keypad_row] |= events >> 8;
            core->keypad.keys[debug_keypad_row] &= ~events;
            debug_keypad_row = (debug_keypad_row + 1) & 7;
        }
        if (!(rand() & 7))
        {
            core_sig(core, CEMUCORE_SIG_SOFT_CMD, false);
            sync_sleep(&core->sync); // wait for response
        }
        core->cpu.regs.r += 2;
        {
            const struct timespec delay =
            {
                .tv_sec = 0,
                .tv_nsec = 100000000L,
            };
            sync_delay(&core->sync, &delay);
        }
    }
    return NULL;
}
#endif

cemucore_t *cemucore_create(cemucore_create_flags_t create_flags,
                            cemucore_sig_handler_t sig_handler,
                            void *sig_handler_data)
{
    cemucore_t *core;
    core_alloc(core, 1);
#ifndef CEMUCORE_NOTHREADS
    if (!sync_init(&core->sync))
    {
        core_free(core, 1);
        return NULL;
    }
#endif
    core->sig_handler = sig_handler;
    core->sig_handler_data = sig_handler_data;
    core->dev = CEMUCORE_DEV_UNKNOWN;
    scheduler_init(&core->scheduler);
    cpu_init(&core->cpu);
    memory_init(&core->memory);
#ifndef CEMUCORE_NOTHREADS
    if (!(create_flags & CEMUCORE_CREATE_FLAG_THREADED))
    {
        core->thread = pthread_self();
        return core;
    }
    else if (!pthread_create(&core->thread, NULL, &thread_start, core))
    {
        return core;
    }
#endif
    memory_destroy(&core->memory);
    cpu_destroy(&core->cpu);
    scheduler_destroy(&core->scheduler);
#ifndef CEMUCORE_NOTHREADS
    sync_destroy(&core->sync);
#endif
    core_free(core, 1);
    return NULL;
}

cemucore_t *cemucore_destroy(cemucore_t *core)
{
    if (!core)
    {
        return core;
    }

#ifndef CEMUCORE_NOTHREADS
    sync_stop(&core->sync);
    pthread_join(core->thread, NULL);
#endif

    memory_destroy(&core->memory);
    cpu_destroy(&core->cpu);
    scheduler_destroy(&core->scheduler);
#ifndef CEMUCORE_NOTHREADS
    sync_destroy(&core->sync);
#endif
    core_free(core, 1);
    return NULL;
}

static bool cemucore_prop_needs_sync(cemucore_prop_t prop)
{
    switch (prop)
    {
        case CEMUCORE_PROP_KEY:
        case CEMUCORE_PROP_GPIO_ENABLE:
            return false;
        default:
            return true;
    }
}

static int32_t do_cemucore_get(cemucore_t *core, cemucore_prop_t prop, int32_t addr)
{
    int32_t val = -1;
    switch (prop)
    {
        case CEMUCORE_PROP_KEY:
            (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.events[addr >> 3 & 7],
                                                          UINT16_C(1) << ((val ? 8 : 0) + (addr & 7)),
                                                          memory_order_release);
            // handle keypad_intrpt_check?
            break;
        case CEMUCORE_PROP_GPIO_ENABLE:
            (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.gpio_enable,
                                                          UINT32_C(1) << (addr & 15),
                                                          memory_order_release);
            // handle keypad_intrpt_check?
            break;
        case CEMUCORE_PROP_DEV:
            val = core->dev;
            break;
        case CEMUCORE_PROP_FLASH_SIZE:
            val = core->memory.flash_size;
            break;
        case CEMUCORE_PROP_TRANSFER:
            switch (addr)
            {
                case CEMUCORE_TRANSFER_TOTAL:     val = core->usb.total;     break;
                case CEMUCORE_TRANSFER_PROGRESS:  val = core->usb.progress;  break;
                case CEMUCORE_TRANSFER_REMAINING: val = core->usb.remaining; break;
                case CEMUCORE_TRANSFER_ERROR:     val = core->usb.error;     break;
            }
            break;
        case CEMUCORE_PROP_REG:
            switch (addr)
            {
                case CEMUCORE_FLAG_C:  val = core->cpu.regs.cf;   break;
                case CEMUCORE_FLAG_N:  val = core->cpu.regs.nf;   break;
                case CEMUCORE_FLAG_PV: val = core->cpu.regs.pv;   break;
                case CEMUCORE_FLAG_X:  val = core->cpu.regs.xf;   break;
                case CEMUCORE_FLAG_HC: val = core->cpu.regs.hc;   break;
                case CEMUCORE_FLAG_Y:  val = core->cpu.regs.yf;   break;
                case CEMUCORE_FLAG_Z:  val = core->cpu.regs.zf;   break;
                case CEMUCORE_FLAG_S:  val = core->cpu.regs.sf;   break;

                case CEMUCORE_REG_F:   val = core->cpu.regs.f;    break;
                case CEMUCORE_REG_A:   val = core->cpu.regs.a;    break;
                case CEMUCORE_REG_C:   val = core->cpu.regs.c;    break;
                case CEMUCORE_REG_B:   val = core->cpu.regs.b;    break;
                case CEMUCORE_REG_BCU: val = core->cpu.regs.bcu;  break;
                case CEMUCORE_REG_E:   val = core->cpu.regs.e;    break;
                case CEMUCORE_REG_D:   val = core->cpu.regs.d;    break;
                case CEMUCORE_REG_DEU: val = core->cpu.regs.deu;  break;
                case CEMUCORE_REG_L:   val = core->cpu.regs.l;    break;
                case CEMUCORE_REG_H:   val = core->cpu.regs.h;    break;
                case CEMUCORE_REG_HLU: val = core->cpu.regs.hlu;  break;
                case CEMUCORE_REG_IXL: val = core->cpu.regs.ixl;  break;
                case CEMUCORE_REG_IXH: val = core->cpu.regs.ixh;  break;
                case CEMUCORE_REG_IXU: val = core->cpu.regs.ixu;  break;
                case CEMUCORE_REG_IYL: val = core->cpu.regs.iyl;  break;
                case CEMUCORE_REG_IYH: val = core->cpu.regs.iyh;  break;
                case CEMUCORE_REG_IYU: val = core->cpu.regs.iyu;  break;
                case CEMUCORE_REG_R:   val = (core->cpu.regs.r & 1) << 7 |
                    core->cpu.regs.r >> 1; break;
                case CEMUCORE_REG_MB:  val = core->cpu.regs.mb;   break;

                case CEMUCORE_REG_AF:  val = core->cpu.regs.af;   break;
                case CEMUCORE_REG_BC:  val = core->cpu.regs.bc;   break;
                case CEMUCORE_REG_DE:  val = core->cpu.regs.de;   break;
                case CEMUCORE_REG_HL:  val = core->cpu.regs.hl;   break;
                case CEMUCORE_REG_IX:  val = core->cpu.regs.ix;   break;
                case CEMUCORE_REG_IY:  val = core->cpu.regs.iy;   break;
                case CEMUCORE_REG_SPS: val = core->cpu.regs.sps;  break;
                case CEMUCORE_REG_I:   val = core->cpu.regs.i;    break;

                case CEMUCORE_REG_UBC: val = core->cpu.regs.ubc;  break;
                case CEMUCORE_REG_UDE: val = core->cpu.regs.ude;  break;
                case CEMUCORE_REG_UHL: val = core->cpu.regs.uhl;  break;
                case CEMUCORE_REG_UIX: val = core->cpu.regs.uix;  break;
                case CEMUCORE_REG_UIY: val = core->cpu.regs.uiy;  break;
                case CEMUCORE_REG_SPL: val = core->cpu.regs.spl;  break;
                case CEMUCORE_REG_PC:  val = core->cpu.regs.pc;   break;
                case CEMUCORE_REG_RPC: val = core->cpu.regs.rpc;  break;
            }
            break;
        case CEMUCORE_PROP_REG_SHADOW:
            switch (addr)
            {
                case CEMUCORE_FLAG_C:  val = core->cpu.regs.shadow_cf;  break;
                case CEMUCORE_FLAG_N:  val = core->cpu.regs.shadow_nf;  break;
                case CEMUCORE_FLAG_PV: val = core->cpu.regs.shadow_pv;  break;
                case CEMUCORE_FLAG_X:  val = core->cpu.regs.shadow_xf;  break;
                case CEMUCORE_FLAG_HC: val = core->cpu.regs.shadow_hc;  break;
                case CEMUCORE_FLAG_Y:  val = core->cpu.regs.shadow_yf;  break;
                case CEMUCORE_FLAG_Z:  val = core->cpu.regs.shadow_zf;  break;
                case CEMUCORE_FLAG_S:  val = core->cpu.regs.shadow_sf;  break;

                case CEMUCORE_REG_F:   val = core->cpu.regs.shadow_f;   break;
                case CEMUCORE_REG_A:   val = core->cpu.regs.shadow_a;   break;
                case CEMUCORE_REG_C:   val = core->cpu.regs.shadow.c;   break;
                case CEMUCORE_REG_B:   val = core->cpu.regs.shadow.b;   break;
                case CEMUCORE_REG_BCU: val = core->cpu.regs.shadow.bcu; break;
                case CEMUCORE_REG_E:   val = core->cpu.regs.shadow.e;   break;
                case CEMUCORE_REG_D:   val = core->cpu.regs.shadow.d;   break;
                case CEMUCORE_REG_DEU: val = core->cpu.regs.shadow.deu; break;
                case CEMUCORE_REG_L:   val = core->cpu.regs.shadow.l;   break;
                case CEMUCORE_REG_H:   val = core->cpu.regs.shadow.h;   break;
                case CEMUCORE_REG_HLU: val = core->cpu.regs.shadow.hlu; break;

                case CEMUCORE_REG_AF:  val = core->cpu.regs.shadow_af;  break;
                case CEMUCORE_REG_BC:  val = core->cpu.regs.shadow.bc;  break;
                case CEMUCORE_REG_DE:  val = core->cpu.regs.shadow.de;  break;
                case CEMUCORE_REG_HL:  val = core->cpu.regs.shadow.hl;  break;

                case CEMUCORE_REG_UBC: val = core->cpu.regs.shadow.ubc; break;
                case CEMUCORE_REG_UDE: val = core->cpu.regs.shadow.ude; break;
                case CEMUCORE_REG_UHL: val = core->cpu.regs.shadow.uhl; break;
            }
            break;
        case CEMUCORE_PROP_MEMORY:
            if (addr >= 0 && addr < core->memory.flash_size)
            {
        case CEMUCORE_PROP_FLASH:
                val = core->memory.flash[addr & (core->memory.flash_size - 1)];
            }
            else if (addr >= 0xD00000 && addr < 0xE00000)
            {
        case CEMUCORE_PROP_RAM:
                if ((addr &= 0x7FFFF) < 0x65800)
                {
                    val = core->memory.ram[addr];
                }
                else
                {
                    val = rand();
                }
            }
            break;
        case CEMUCORE_PROP_PORT:
            break;
#ifndef CEMUCORE_NODEBUG
        case CEMUCORE_PROP_MEMORY_DEBUG_FLAGS:
            val = core->memory.debug[addr & 0xFFFFFF];
            break;
        case CEMUCORE_PROP_FLASH_DEBUG_FLAGS:
            val = core->memory.debug[addr & (core->memory.flash_size - 1)];
            break;
        case CEMUCORE_PROP_RAM_DEBUG_FLAGS:
            if ((addr &= 0x7FFFF) < 0x65800)
            {
                val = core->memory.debug[0xD00000 + addr];
            }
            break;
        case CEMUCORE_PROP_PORT_DEBUG_FLAGS:
            break;
#endif
        default:
            break;
    }
    return val;
}

int32_t cemucore_get(cemucore_t *core, cemucore_prop_t prop, int32_t addr)
{
    if (!core)
    {
        return -1;
    }
    if (cemucore_prop_needs_sync(prop))
    {
        sync_enter(&core->sync);
    }
    int32_t val = do_cemucore_get(core, prop, addr);
    if (cemucore_prop_needs_sync(prop))
    {
        sync_leave(&core->sync);
    }
    return val;
}

void cemucore_get_buffer(cemucore_t *core, cemucore_prop_t prop, int32_t addr, void *buf, uint32_t len)
{
    if (!core || len > 0x1000000)
    {
        return;
    }
    sync_enter(&core->sync);
    uint8_t *dst = buf;
    int32_t remaining = len;
    switch (prop)
    {
        case CEMUCORE_PROP_MEMORY:
            if (addr >= 0 && addr < core->memory.flash_size)
            {
        case CEMUCORE_PROP_FLASH:
                addr &= core->memory.flash_size - 1;
                if (remaining > core->memory.flash_size - addr)
                {
                    remaining = core->memory.flash_size - addr;
                }
                memcpy(dst, &core->memory.flash[addr], remaining);
                addr += remaining;
                dst += remaining;
                remaining = len -= remaining;
            }
            else if (addr >= 0xD00000 && addr < 0xE00000)
            {
        case CEMUCORE_PROP_RAM:
                if ((addr &= 0x7FFFF) < 0x65800)
                {
                    if (remaining > 0x65800 - addr)
                    {
                        remaining = 0x65800 - addr;
                    }
                    memcpy(dst, &core->memory.ram[addr], remaining);
                    addr += remaining;
                    dst += remaining;
                    remaining = len -= remaining;
                }
            }
            break;
        case CEMUCORE_PROP_PORT:
            break;
#ifndef CEMUCORE_NODEBUG
        case CEMUCORE_PROP_MEMORY_DEBUG_FLAGS:
            addr &= 0xFFFFFF;
            if (remaining > 0x1000000 - addr)
            {
                remaining = 0x1000000 - addr;
            }
            memcpy(dst, &core->memory.debug[addr], remaining);
            addr += remaining;
            dst += remaining;
            remaining = len -= remaining;
            break;
        case CEMUCORE_PROP_FLASH_DEBUG_FLAGS:
            addr &= core->memory.flash_size - 1;
            if (remaining > core->memory.flash_size - addr)
            {
                remaining = core->memory.flash_size - addr;
            }
            memcpy(dst, &core->memory.debug[addr], remaining);
            addr += remaining;
            dst += remaining;
            remaining = len -= remaining;
            break;
        case CEMUCORE_PROP_RAM_DEBUG_FLAGS:
            if ((addr &= 0x7FFFF) < 0x65800)
            {
                if (remaining > 0x65800 - addr)
                {
                    remaining = 0x65800 - addr;
                }
                memcpy(dst, &core->memory.debug[0xD00000 + addr], remaining);
                addr += remaining;
                dst += remaining;
                remaining = len -= remaining;
            }
            break;
        case CEMUCORE_PROP_PORT_DEBUG_FLAGS:
            break;
#endif
        default:
            break;
    }
    while (remaining--)
    {
        *dst++ = do_cemucore_get(core, prop, addr++);
    }
    sync_leave(&core->sync);
}

static void do_cemucore_set(cemucore_t *core, cemucore_prop_t prop, int32_t addr, int32_t val)
{
    switch (prop)
    {
        case CEMUCORE_PROP_KEY:
            (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.events[addr >> 3 & 7],
                                                          UINT16_C(1) << ((val ? 8 : 0) + (addr & 7)),
                                                          memory_order_release);
            // handle keypad_intrpt_check?
            break;
        case CEMUCORE_PROP_GPIO_ENABLE:
            (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.gpio_enable,
                                                          UINT32_C(1) << (addr & 15),
                                                          memory_order_release);
            // handle keypad_intrpt_check?
            break;
        case CEMUCORE_PROP_DEV:
            if (core->dev != (cemucore_dev_t)val)
            {
                core->dev = val;
                core_sig(core, CEMUCORE_SIG_DEV_CHANGED, true);
            }
            break;
        case CEMUCORE_PROP_REG:
            switch (addr)
            {
                case CEMUCORE_FLAG_C:  core->cpu.regs.cf  = val; break;
                case CEMUCORE_FLAG_N:  core->cpu.regs.nf  = val; break;
                case CEMUCORE_FLAG_PV: core->cpu.regs.pv  = val; break;
                case CEMUCORE_FLAG_X:  core->cpu.regs.xf  = val; break;
                case CEMUCORE_FLAG_HC: core->cpu.regs.hc  = val; break;
                case CEMUCORE_FLAG_Y:  core->cpu.regs.yf  = val; break;
                case CEMUCORE_FLAG_Z:  core->cpu.regs.zf  = val; break;
                case CEMUCORE_FLAG_S:  core->cpu.regs.sf  = val; break;

                case CEMUCORE_REG_F:   core->cpu.regs.f   = val; break;
                case CEMUCORE_REG_A:   core->cpu.regs.a   = val; break;
                case CEMUCORE_REG_C:   core->cpu.regs.c   = val; break;
                case CEMUCORE_REG_B:   core->cpu.regs.b   = val; break;
                case CEMUCORE_REG_BCU: core->cpu.regs.bcu = val; break;
                case CEMUCORE_REG_E:   core->cpu.regs.e   = val; break;
                case CEMUCORE_REG_D:   core->cpu.regs.d   = val; break;
                case CEMUCORE_REG_DEU: core->cpu.regs.deu = val; break;
                case CEMUCORE_REG_L:   core->cpu.regs.l   = val; break;
                case CEMUCORE_REG_H:   core->cpu.regs.h   = val; break;
                case CEMUCORE_REG_HLU: core->cpu.regs.hlu = val; break;
                case CEMUCORE_REG_IXL: core->cpu.regs.ixl = val; break;
                case CEMUCORE_REG_IXH: core->cpu.regs.ixh = val; break;
                case CEMUCORE_REG_IXU: core->cpu.regs.ixu = val; break;
                case CEMUCORE_REG_IYL: core->cpu.regs.iyl = val; break;
                case CEMUCORE_REG_IYH: core->cpu.regs.iyh = val; break;
                case CEMUCORE_REG_IYU: core->cpu.regs.iyu = val; break;
                case CEMUCORE_REG_R:   core->cpu.regs.r   =
                    val << 1 | val >> 7;  break;
                case CEMUCORE_REG_MB:  core->cpu.regs.mb  = val; break;

                case CEMUCORE_REG_AF:  core->cpu.regs.af  = val; break;
                case CEMUCORE_REG_BC:  core->cpu.regs.bc  = val; break;
                case CEMUCORE_REG_DE:  core->cpu.regs.de  = val; break;
                case CEMUCORE_REG_HL:  core->cpu.regs.hl  = val; break;
                case CEMUCORE_REG_IX:  core->cpu.regs.ix  = val; break;
                case CEMUCORE_REG_IY:  core->cpu.regs.iy  = val; break;
                case CEMUCORE_REG_SPS: core->cpu.regs.sps = val; break;
                case CEMUCORE_REG_I:   core->cpu.regs.i   = val; break;

                case CEMUCORE_REG_UBC: core->cpu.regs.ubc = val; break;
                case CEMUCORE_REG_UDE: core->cpu.regs.ude = val; break;
                case CEMUCORE_REG_UHL: core->cpu.regs.uhl = val; break;
                case CEMUCORE_REG_UIX: core->cpu.regs.uix = val; break;
                case CEMUCORE_REG_UIY: core->cpu.regs.uiy = val; break;
                case CEMUCORE_REG_SPL: core->cpu.regs.spl = val; break;
                case CEMUCORE_REG_PC:  core->cpu.regs.pc  = val; break;
                case CEMUCORE_REG_RPC: core->cpu.regs.rpc = val; break;
            }
            break;
        case CEMUCORE_PROP_REG_SHADOW:
            switch (addr)
            {
                case CEMUCORE_FLAG_C:  core->cpu.regs.shadow_cf  = val; break;
                case CEMUCORE_FLAG_N:  core->cpu.regs.shadow_nf  = val; break;
                case CEMUCORE_FLAG_PV: core->cpu.regs.shadow_pv  = val; break;
                case CEMUCORE_FLAG_X:  core->cpu.regs.shadow_xf  = val; break;
                case CEMUCORE_FLAG_HC: core->cpu.regs.shadow_hc  = val; break;
                case CEMUCORE_FLAG_Y:  core->cpu.regs.shadow_yf  = val; break;
                case CEMUCORE_FLAG_Z:  core->cpu.regs.shadow_zf  = val; break;
                case CEMUCORE_FLAG_S:  core->cpu.regs.shadow_sf  = val; break;

                case CEMUCORE_REG_F:   core->cpu.regs.shadow_f   = val; break;
                case CEMUCORE_REG_A:   core->cpu.regs.shadow_a   = val; break;
                case CEMUCORE_REG_C:   core->cpu.regs.shadow.c   = val; break;
                case CEMUCORE_REG_B:   core->cpu.regs.shadow.b   = val; break;
                case CEMUCORE_REG_BCU: core->cpu.regs.shadow.bcu = val; break;
                case CEMUCORE_REG_E:   core->cpu.regs.shadow.e   = val; break;
                case CEMUCORE_REG_D:   core->cpu.regs.shadow.d   = val; break;
                case CEMUCORE_REG_DEU: core->cpu.regs.shadow.deu = val; break;
                case CEMUCORE_REG_L:   core->cpu.regs.shadow.l   = val; break;
                case CEMUCORE_REG_H:   core->cpu.regs.shadow.h   = val; break;
                case CEMUCORE_REG_HLU: core->cpu.regs.shadow.hlu = val; break;

                case CEMUCORE_REG_AF:  core->cpu.regs.shadow_af  = val; break;
                case CEMUCORE_REG_BC:  core->cpu.regs.shadow.bc  = val; break;
                case CEMUCORE_REG_DE:  core->cpu.regs.shadow.de  = val; break;
                case CEMUCORE_REG_HL:  core->cpu.regs.shadow.hl  = val; break;

                case CEMUCORE_REG_UBC: core->cpu.regs.shadow.ubc = val; break;
                case CEMUCORE_REG_UDE: core->cpu.regs.shadow.ude = val; break;
                case CEMUCORE_REG_UHL: core->cpu.regs.shadow.uhl = val; break;
            }
            break;
        case CEMUCORE_PROP_MEMORY:
            if (addr >= INT32_C(0) && addr < INT32_C(0x400000))
            {
                core->memory.flash[addr] = val;
            }
            else if (addr >= INT32_C(0xD00000) && addr < INT32_C(0xE00000))
            {
                if ((addr &= INT32_C(0x7FFFF)) < INT32_C(0x65800))
                {
                    core->memory.ram[addr] = val;
                }
            }
            break;
        case CEMUCORE_PROP_FLASH:
            core->memory.flash[addr & INT32_C(0x3FFFFF)] = val;
            break;
        case CEMUCORE_PROP_RAM:
            if ((addr &= INT32_C(0x7FFFF)) < INT32_C(0x65800))
            {
                core->memory.ram[addr] = val;
            }
            break;
        case CEMUCORE_PROP_PORT:
            break;
#ifndef CEMUCORE_NODEBUG
        case CEMUCORE_PROP_MEMORY_DEBUG_FLAGS:
            core->memory.debug[addr & INT32_C(0xFFFFFF)] = val;
            break;
        case CEMUCORE_PROP_FLASH_DEBUG_FLAGS:
            core->memory.debug[addr & INT32_C(0x3FFFFF)] = val;
            break;
        case CEMUCORE_PROP_RAM_DEBUG_FLAGS:
            if ((addr &= INT32_C(0x7FFFF)) < INT32_C(0x65800))
            {
                core->memory.debug[INT32_C(0xD00000) + addr] = val;
            }
            break;
        case CEMUCORE_PROP_PORT_DEBUG_FLAGS:
            break;
#endif
        default:
            break;
    }
}

void cemucore_set(cemucore_t *core, cemucore_prop_t prop, int32_t addr, int32_t val)
{
    if (!core)
    {
        return;
    }
    if (cemucore_prop_needs_sync(prop))
    {
        sync_enter(&core->sync);
    }
    do_cemucore_set(core, prop, addr, val);
    if (cemucore_prop_needs_sync(prop))
    {
        sync_leave(&core->sync);
    }
}

void cemucore_set_buffer(cemucore_t *core, cemucore_prop_t prop, int32_t addr, const void *buf, uint32_t len)
{
    if (!core || len > 0x1000000)
    {
        return;
    }
    sync_enter(&core->sync);
    const uint8_t *src = buf;
    int32_t remaining = len;
    switch (prop)
    {
        case CEMUCORE_PROP_MEMORY:
            if (addr >= 0 && addr < core->memory.flash_size)
            {
        case CEMUCORE_PROP_FLASH:
                addr &= core->memory.flash_size - 1;
                if (remaining > core->memory.flash_size - addr)
                {
                    remaining = core->memory.flash_size - addr;
                }
                memcpy(&core->memory.flash[addr], src, remaining);
                addr += remaining;
                src += remaining;
                remaining = len -= remaining;
            }
            else if (addr >= 0xD00000 && addr < 0xE00000)
            {
        case CEMUCORE_PROP_RAM:
                if ((addr &= 0x7FFFF) < 0x65800)
                {
                    if (remaining > 0x65800 - addr)
                    {
                        remaining = 0x65800 - addr;
                    }
                    memcpy(&core->memory.ram[addr], src, remaining);
                    addr += remaining;
                    src += remaining;
                    remaining = len -= remaining;
                }
            }
            break;
        case CEMUCORE_PROP_PORT:
            break;
#ifndef CEMUCORE_NODEBUG
        case CEMUCORE_PROP_MEMORY_DEBUG_FLAGS:
            addr &= 0xFFFFFF;
            if (remaining > 0x1000000 - addr)
            {
                remaining = 0x1000000 - addr;
            }
            memcpy(&core->memory.debug[addr], src, remaining);
            addr += remaining;
            src += remaining;
            remaining = len -= remaining;
            break;
        case CEMUCORE_PROP_FLASH_DEBUG_FLAGS:
            addr &= core->memory.flash_size - 1;
            if (remaining > core->memory.flash_size - addr)
            {
                remaining = core->memory.flash_size - addr;
            }
            memcpy(&core->memory.debug[addr], src, remaining);
            addr += remaining;
            src += remaining;
            remaining = len -= remaining;
            break;
        case CEMUCORE_PROP_RAM_DEBUG_FLAGS:
            if ((addr &= 0x7FFFF) < 0x65800)
            {
                if (remaining > 0x65800 - addr)
                {
                    remaining = 0x65800 - addr;
                }
                memcpy(&core->memory.debug[0xD00000 + addr], src, remaining);
                addr += remaining;
                src += remaining;
                remaining = len -= remaining;
            }
            break;
        case CEMUCORE_PROP_PORT_DEBUG_FLAGS:
            break;
#endif
        default:
            break;
    }
    while (remaining--)
    {
        do_cemucore_set(core, prop, addr++, *src++);
    }
    sync_leave(&core->sync);
}

static int do_command(cemucore_t *core, const char *const *args)
{
    if (!args[0])
    {
        return EINVAL;
    }
    if (!strcmp(args[0], "load"))
    {
        if (!args[1] || !args[2])
        {
            return EINVAL;
        }
        if (!strcmp(args[1], "rom"))
        {
            FILE *file = fopen_utf8(args[2], "rb");
            if (!file)
            {
                goto load_rom_return;
            }
            if (fseek(file, 0, SEEK_END))
            {
                goto load_rom_close_file;
            }
            long size = ftell(file);
            if (size < 0)
            {
                goto load_rom_close_file;
            }
            if (!size || (size & (size - 1)) || size > MEMORY_MAX_FLASH_SIZE)
            {
                errno = EINVAL;
                goto load_rom_close_file;
            }
            if (size != core->memory.flash_size)
            {
                core->memory.flash_size = size;
                void *flash = realloc(core->memory.flash, size);
                if (!flash)
                {
                    goto load_rom_close_file;
                }
                core->memory.flash_size = size;
                core->memory.flash = flash;
                core_sig(core, CEMUCORE_SIG_FLASH_SIZE_CHANGED, true);
            }
            memset(core->memory.flash, 0xFF, core->memory.flash_size);
            if (fseek(file, 0, SEEK_SET) ||
                fread(core->memory.flash, core->memory.flash_size, 1, file) != 1)
            {
                goto load_rom_close_file;
            }
            // TODO: reset emu
            errno = 0;
        load_rom_close_file:
            fclose(file);
            file = NULL;
        load_rom_return:
            return errno;
        }
        if (!strcmp(args[1], "ram"))
        {
            FILE *file = fopen_utf8(args[2], "rb");
            if (!file)
            {
                goto load_ram_return;
            }
            if (fseek(file, 0, SEEK_END))
            {
                goto load_ram_close_file;
            }
            long size = ftell(file);
            if (size < 0)
            {
                goto load_ram_close_file;
            }
            if (size != MEMORY_RAM_SIZE)
            {
                errno = EINVAL;
                goto load_ram_close_file;
            }
            memset(core->memory.ram, 0x00, MEMORY_RAM_SIZE);
            if (fseek(file, 0, SEEK_SET) ||
                fread(core->memory.ram, MEMORY_RAM_SIZE, 1, file) != 1)
            {
                goto load_ram_close_file;
            }
            // TODO: reset emu
            errno = 0;
        load_ram_close_file:
            fclose(file);
            file = NULL;
        load_ram_return:
            return errno;
        }
        return EINVAL;
    }
    if (!strcmp(args[0], "store"))
    {
        if (!args[1] || !args[2])
        {
            return EINVAL;
        }
        if (!strcmp(args[1], "rom"))
        {
            FILE *file = fopen_utf8(args[2], "wb");
            if (!file)
            {
                goto store_rom_return;
            }
            if (fseek(file, 0, SEEK_END))
            {
                goto store_rom_close_file;
            }
            if (fwrite(core->memory.flash, core->memory.flash_size, 1, file) != 1)
            {
                goto store_rom_close_file;
            }
            errno = 0;
        store_rom_close_file:
            fclose(file);
            file = NULL;
        store_rom_return:
            return errno;
        }
        if (!strcmp(args[1], "ram"))
        {
            FILE *file = fopen_utf8(args[2], "wb");
            if (!file)
            {
                goto store_ram_return;
            }
            if (fwrite(core->memory.ram, MEMORY_RAM_SIZE, 1, file) != 1)
            {
                goto store_ram_close_file;
            }
            errno = 0;
        store_ram_close_file:
            fclose(file);
            file = NULL;
        store_ram_return:
            return errno;
        }
        return EINVAL;
    }
    return EINVAL;
}

int cemucore_command(cemucore_t *core, const char *const *args)
{
    if (!core)
    {
        return EINVAL;
    }
    sync_enter(&core->sync);
    int error = do_command(core, args);
    sync_leave(&core->sync);
    return error;
}

bool cemucore_sleep(cemucore_t *core)
{
    return core && sync_sleep(&core->sync);
}

bool cemucore_wake(cemucore_t *core)
{
    return core && sync_wake(&core->sync);
}

void core_sig(cemucore_t *core, cemucore_sig_t sig, bool leave)
{
    if (leave)
    {
        sync_leave(&core->sync);
    }
    if (core->sig_handler)
    {
        core->sig_handler(sig, core->sig_handler_data);
    }
    if (leave)
    {
        sync_enter(&core->sync);
    }
}

struct core_safe_alloc_node;
#ifndef CEMUCORE_NOSAFEALLOC
static struct core_safe_alloc_node {
    struct core_safe_alloc_node *next;
    size_t type_size;
    void *memory;
    size_t count;
} *core_safe_alloc_list;
#endif

static struct core_safe_alloc_node **core_record_safe_alloc(size_t type_size, void *new_memory, size_t new_count, const char *file, int line, const char *func)
{
#ifdef CEMUCORE_NOSAFEALLOC
    (void)type_size;
    (void)new_memory;
    (void)new_count;
    (void)func;
    (void)file;
    (void)line;
    return NULL;
#else
    struct core_safe_alloc_node *node = malloc(sizeof(struct core_safe_alloc_node));
    if (cemucore_unlikely(!node))
    {
        core_fatal_location(file, line, "%s: out of safe memory", func);
    }
    node->next = core_safe_alloc_list;
    node->type_size = type_size;
    node->memory = new_memory;
    node->count = new_count;
    core_safe_alloc_list = node;
    return &core_safe_alloc_list;
#endif
}

static struct core_safe_alloc_node **core_verify_safe_alloc(size_t type_size, void *old_memory, size_t old_count, const char *file, int line, const char *func)
{
#ifdef CEMUCORE_NOSAFEALLOC
    (void)type_size;
    (void)old_memory;
    (void)old_count;
    (void)func;
    (void)file;
    (void)line;
    return NULL;
#else
    struct core_safe_alloc_node *node;
    if (cemucore_unlikely(!old_memory))
    {
        node = *core_record_safe_alloc(type_size, old_memory, 0, file, line, func);
    }
    else
    {
        for (node = core_safe_alloc_list; node && node->memory != old_memory; node = node->next)
        {
        }
    }
    if (cemucore_unlikely(!node))
    {
        core_fatal_location(file, line, "%s: invalid old memory", func);
    }
    if (cemucore_unlikely(node->type_size != type_size))
    {
        core_fatal_location(file, line, "%s: invalid type size", func);
    }
    if (cemucore_unlikely(node->count != old_count))
    {
        core_fatal_location(file, line, "%s: invalid old count", func);
    }
    return &core_safe_alloc_list;
#endif
}

static void core_update_safe_alloc(struct core_safe_alloc_node **node_update, void *new_memory, size_t new_count)
{
#ifdef CEMUCORE_NOSAFEALLOC
    (void)node_update;
    (void)new_memory;
    (void)new_count;
#else
    struct core_safe_alloc_node *node = *node_update;
    node->memory = new_memory;
    node->count = new_count;
#endif
}

static void core_remove_safe_alloc(struct core_safe_alloc_node **node_update)
{
#ifdef CEMUCORE_NOSAFEALLOC
    (void)node_update;
#else
    struct core_safe_alloc_node *node = *node_update;
    *node_update = node->next;
    free(node);
#endif
}

#undef core_alloc_location
void *core_alloc_location(size_t type_size, size_t new_count, const char *file, int line)
{
    void *new_memory = calloc(new_count ? new_count : 1, type_size ? type_size : 1);
    if (cemucore_unlikely(!new_memory))
    {
        core_fatal_location(file, line, "core_alloc: out of memory");
    }
    core_record_safe_alloc(type_size, new_memory, new_count, file, line, "core_alloc");
    return new_memory;
}

#undef core_realloc_location
void *core_realloc_location(size_t type_size, void *old_memory, size_t old_count, size_t new_count, const char *file, int line)
{
    size_t old_size, new_size;
    if (cemucore_unlikely(cemucore_mul_overflow(type_size, old_count, &old_size) ||
                          cemucore_mul_overflow(type_size, new_count, &new_size)))
    {
        core_fatal_location(file, line, "core_realloc: size overflow");
    }
    char *new_memory = realloc(old_memory, new_size);
    if (cemucore_unlikely(!new_memory))
    {
        core_fatal_location(file, line, "core_realloc: out of memory");
    }
    if (new_size > old_size)
    {
        memset((char *)new_memory + old_size, 0, new_size - old_size);
    }
    core_update_safe_alloc(core_verify_safe_alloc(type_size, old_memory, old_count, file, line, "core_realloc"), new_memory, new_count);
    return new_memory;
}

#undef core_free_location
void *core_free_location(size_t type_size, void *old_memory, size_t old_count, const char *file, int line)
{
    core_remove_safe_alloc(core_verify_safe_alloc(type_size, old_memory, old_count, file, line, "core_free"));
    free(old_memory);
    return NULL;
}

#undef core_fatal_location
void core_fatal_location(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    abort();
}

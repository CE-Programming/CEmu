/*
 * Copyright (c) 2015-2020 CE Programming.
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

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#ifndef CEMUCORE_NOTHREADS
static void *thread_start(void *data)
{
    cemucore_t *core = data;
    int debug_keypad_row = 0;
    while (sync_loop(&core->sync))
    {
        const struct timespec sleep = {
            .tv_sec = 0,
            .tv_nsec = 100000000L,
        };
        nanosleep(&sleep, NULL);
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
    }
    return NULL;
}
#endif

cemucore_t *cemucore_create(cemucore_create_flags_t create_flags,
                            cemucore_signal_t signal, void *signal_data)
{
    cemucore_t *core = calloc(1, sizeof(cemucore_t));
    if (!core)
    {
        return NULL;
    }

    core->signal = signal;
    core->signal_data = signal_data;

#ifndef CEMUCORE_NOTHREADS
    if (!sync_init(&core->sync))
    {
        free(core);
        return NULL;
    }

    if (create_flags & CEMUCORE_CREATE_FLAG_THREADED)
    {
        if (pthread_create(&core->thread, NULL, &thread_start, core))
        {
            free(core);
            return NULL;
        }
    }
    else
    {
        core->thread = pthread_self();
    }
#endif

    return core;
}

void cemucore_destroy(cemucore_t *core)
{
    if (!core)
    {
        return;
    }

#ifndef CEMUCORE_NOTHREADS
    sync_stop(&core->sync);
    pthread_join(core->thread, NULL);
#endif

    free(core);
}

int32_t cemucore_get(cemucore_t *core, cemucore_prop_t prop, int32_t addr)
{
    int32_t val = -1;
    if (!core)
    {
        return val;
    }
    switch (prop)
    {
    default:
        break;
    case CEMUCORE_PROP_REG:
        sync_enter(&core->sync);
        switch (addr)
        {
        case CEMUCORE_REG_F:   val = core->cpu.regs.f;   break;
        case CEMUCORE_REG_A:   val = core->cpu.regs.a;   break;
        case CEMUCORE_REG_C:   val = core->cpu.regs.c;   break;
        case CEMUCORE_REG_B:   val = core->cpu.regs.b;   break;
        case CEMUCORE_REG_BCU: val = core->cpu.regs.bcu; break;
        case CEMUCORE_REG_E:   val = core->cpu.regs.e;   break;
        case CEMUCORE_REG_D:   val = core->cpu.regs.d;   break;
        case CEMUCORE_REG_DEU: val = core->cpu.regs.deu; break;
        case CEMUCORE_REG_L:   val = core->cpu.regs.l;   break;
        case CEMUCORE_REG_H:   val = core->cpu.regs.h;   break;
        case CEMUCORE_REG_HLU: val = core->cpu.regs.hlu; break;
        case CEMUCORE_REG_IXL: val = core->cpu.regs.ixl; break;
        case CEMUCORE_REG_IXH: val = core->cpu.regs.ixh; break;
        case CEMUCORE_REG_IXU: val = core->cpu.regs.ixu; break;
        case CEMUCORE_REG_IYL: val = core->cpu.regs.iyl; break;
        case CEMUCORE_REG_IYH: val = core->cpu.regs.iyh; break;
        case CEMUCORE_REG_IYU: val = core->cpu.regs.iyu; break;
        case CEMUCORE_REG_R:   val = core->cpu.regs.r;   break;

        case CEMUCORE_REG_AF:  val = core->cpu.regs.af;  break;
        case CEMUCORE_REG_BC:  val = core->cpu.regs.bc;  break;
        case CEMUCORE_REG_DE:  val = core->cpu.regs.de;  break;
        case CEMUCORE_REG_HL:  val = core->cpu.regs.hl;  break;
        case CEMUCORE_REG_IX:  val = core->cpu.regs.ix;  break;
        case CEMUCORE_REG_IY:  val = core->cpu.regs.iy;  break;
        case CEMUCORE_REG_SPS: val = core->cpu.regs.sps; break;
        case CEMUCORE_REG_I:   val = core->cpu.regs.i;   break;

        case CEMUCORE_REG_UBC: val = core->cpu.regs.ubc; break;
        case CEMUCORE_REG_UDE: val = core->cpu.regs.ude; break;
        case CEMUCORE_REG_UHL: val = core->cpu.regs.uhl; break;
        case CEMUCORE_REG_UIX: val = core->cpu.regs.uix; break;
        case CEMUCORE_REG_UIY: val = core->cpu.regs.uiy; break;
        case CEMUCORE_REG_SPL: val = core->cpu.regs.spl; break;
        case CEMUCORE_REG_PC:  val = core->cpu.regs.pc;  break;
        case CEMUCORE_REG_RPC: val = core->cpu.regs.rpc; break;
        }
        sync_leave(&core->sync);
        break;
    case CEMUCORE_PROP_REG_SHADOW:
        sync_enter(&core->sync);
        switch (addr)
        {
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
        sync_leave(&core->sync);
        break;
    case CEMUCORE_PROP_KEY:
        sync_enter(&core->sync);
        val = core->keypad.keys[addr >> 3 & 7] >> (addr & 7) & 1;
        sync_leave(&core->sync);
        break;
    case CEMUCORE_PROP_GPIO_ENABLE:
        sync_enter(&core->sync);
        val = cemucore_maybe_atomic_load_explicit(&core->keypad.gpio_enable,
                                                  memory_order_relaxed) >> (addr & 15) & 1;
        sync_leave(&core->sync);
        break;
    }
    return val;
}

void cemucore_set(cemucore_t *core, cemucore_prop_t prop, int32_t addr, int32_t val)
{
    if (!core)
    {
        return;
    }
    switch (prop)
    {
    default:
        break;
    case CEMUCORE_PROP_REG:
        sync_enter(&core->sync);
        switch (addr)
        {
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
          case CEMUCORE_REG_R:   core->cpu.regs.r   = val; break;
 
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
        sync_leave(&core->sync);
        break;
    case CEMUCORE_PROP_REG_SHADOW:
        sync_enter(&core->sync);
        switch (addr)
        {
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
        sync_leave(&core->sync);
        break;
    case CEMUCORE_PROP_KEY:
        (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.events[addr >> 3 & 7],
                                                      UINT16_C(1) << ((val ? 8 : 0) + (addr & 7)),
                                                      memory_order_release);
        break;
    case CEMUCORE_PROP_GPIO_ENABLE:
        (void)cemucore_maybe_atomic_fetch_or_explicit(&core->keypad.gpio_enable,
                                                      UINT32_C(1) << (addr & 15),
                                                      memory_order_release);
        break;
    }
}

keypad_t keypad;
debug_t debug;
lcd_t lcd;

ti_device_t get_device_type(void) {
    return TI84PCE;
}
uint8_t mem_peek_byte(uint32_t addr) {
    (void)addr;
    return 0;
}
void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t control, bool mask) {
    (void)dat;
    (void)dat_end;
    (void)width;
    (void)height;
    (void)addr;
    (void)control;
    (void)mask;
}
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t control, int size, int spi) {
    (void)output;
    (void)data;
    (void)data_end;
    (void)control;
    (void)size;
    (void)spi;
}
void emu_keypad_event(unsigned int row, unsigned int col, bool press) {
    (void)row;
    (void)col;
    (void)press;
}
void keypad_intrpt_check(void) {}
FILE *fopen_utf8(const char *filename, const char *mode) {
    return fopen(filename, mode);
}

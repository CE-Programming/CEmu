/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "emu.h"
#include "schedule.h"
#include "asic.h"
#include "cert.h"
#include "os/os.h"

const char *rom_image = NULL;

uint32_t cpu_events;
volatile bool exiting;

const char log_type_tbl[] = LOG_TYPE_TBL;
int log_enabled[MAX_LOG];
FILE *log_file[MAX_LOG];
void logprintf(int type, const char *str, ...) {
    if (log_enabled[type]) {
        va_list va;
        va_start(va, str);
        vfprintf(log_file[type], str, va);
        va_end(va);
    }
}

#include <time.h>
void throttle_interval_event(int index) {
    event_repeat(index, 27000000 / 60);

    gui_do_stuff(true);

    throttle_timer_wait();
}

bool emu_start(void) {
    bool ret = false;
    long lSize;

    asic_init();

    if (rom_image == NULL) {
        gui_console_printf("No ROM image specified.");
    }
    else {
        FILE *rom = fopen_utf8(rom_image, "rb");
        do {
            if (rom) {
                uint16_t field_type;
                const uint8_t *outer;
                const uint8_t *current;
                const uint8_t *data;
                uint32_t outer_field_size;
                uint32_t data_field_size;
                ti_device_type device_type;
                uint32_t offset;

                /* Get ROM file size */
                if (fseek(rom, 0L, SEEK_END) < 0) {
                    break;
                }

                lSize = ftell(rom);
                if (lSize < 0) {
                    break;
                }

                if (fseek(rom, 0L, SEEK_SET) < 0) {
                    break;
                }

                /* Read whole ROM. */
                if (fread(asic.mem->flash.block, 1, lSize, rom) < (size_t)lSize) {
                    break;
                }

                /* Parse certificate fields to determine model.                            */
                /* device_type = (ti_device_type)(asic.mem->flash.block[0x20017]);         */
                /* We've heard of the OS base being at 0x30000 on at least one calculator. */
                for (offset = 0x20000U; offset < 0x40000U; offset += 0x10000U) {
                    outer = asic.mem->flash.block;
                    /* Outer 0x800(0) field. */
                    if (cert_field_get(outer + offset, asic.mem->flash.size - offset, &field_type, &outer, &outer_field_size)) {
                        break;
                    }
                    if (field_type != 0x800F /*|| field_type == 0x800D || field_type == 0x800E*/) {
                        continue;
                    }
                    /*fprintf(stderr, "outer: %p\t%04X\t%p\t%u\n", asic.mem->flash.block, field_type, outer, outer_field_size);*/

                    /* Inner 0x801(0) field: calculator model */
                    if (cert_field_get(outer, outer_field_size, &field_type, &data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "inner 1: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (field_type != 0x8012 || data[0] != 0x13) {
                        break;
                    }

                    /* Inner 0x802(0) field: skip. */
                    data_field_size = outer_field_size - (data + data_field_size - outer);
                    data = outer;
                    if (cert_field_next(&data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "data: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    current = data;
                    if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "inner 2: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (field_type != 0x8021) {
                        break;
                    }

                    /* Inner 0x803(0) field: skip. */
                    data_field_size = outer_field_size - (data + data_field_size - outer);
                    data = current;
                    if (cert_field_next(&data, &data_field_size)) {
                        break;
                    }
                    current = data;
                    /*fprintf(stderr, "data: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "inner 3: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (field_type != 0x8032) {
                        break;
                    }

                    /* Inner 0x80A(0) field: skip. */
                    data_field_size = outer_field_size - (data + data_field_size - outer);
                    data = current;
                    if (cert_field_next(&data, &data_field_size)) {
                        break;
                    }
                    current = data;
                    /*fprintf(stderr, "data: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "inner 4: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (field_type != 0x80A1) {
                        break;
                    }

                    /* Inner 0x80C(0) field: keep. */
                    data_field_size = outer_field_size - (data + data_field_size - outer);
                    data = current;
                    if (cert_field_next(&data, &data_field_size)) {
                        break;
                    }
                    current = data;
                    /*fprintf(stderr, "data: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) {
                        break;
                    }
                    /*fprintf(stderr, "inner 5: %p\t%04X\t%p\t%u\n", outer, field_type, data, data_field_size);*/
                    if (field_type != 0x80C2) {
                        break;
                    }

                    /*fprintf(stderr, "Found calculator type %02X\n", data[1]);*/
                    if (data[1] != 0 && data[1] != 1) {
                        break;
                    }
                    device_type = (ti_device_type)(data[1]);

                    /* If we come here, we've found something. */
                    ret = true;
                    break;
                }


                if (ret) {
                    control.device_type = device_type;
                    asic.device_type = device_type;
                }

            }
        } while(0);

        if (!ret) {
            gui_console_printf("Error opening ROM image.\n", rom_image);
            emu_cleanup();
        }

        if (rom) {
            fclose(rom);
        }
    }

    return ret;
}


void emu_cleanup(void) {
    asic_free();
}

static void emu_reset(void) {
    sched_reset();

    sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

    asic_reset();

    /* Drain everything */
    cpu_reset();
    cpu_events &= EVENT_DEBUG_STEP;

    sched_update_next_event();
}

static void emu_main_loop(void) {
    while (!exiting) {
        if (cpu_events & EVENT_RESET) {
            cpu_events &= EVENT_DEBUG_STEP;
            gui_console_printf("CPU Reset triggered...");
            emu_reset();
        }
        if (!cpu.halted && cpu_events & EVENT_DEBUG_STEP) {
            cpu_events &= ~EVENT_DEBUG_STEP;
            debugger(DBG_STEP, 0);
        }
        sched_process_pending_events();
        cpu_execute();  // execute instructions with available clock cycles
    }
}

void emu_loop(bool reset) {
    if (reset) {
        emu_reset();
    }

    exiting = false;

    emu_main_loop();
    emu_cleanup();
}

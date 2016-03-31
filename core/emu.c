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

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "emu.h"
#include "schedule.h"
#include "asic.h"
#include "cert.h"
#include "os/os.h"

#define imageVersion 0xCECE0003

uint32_t cpuEvents;
volatile bool exiting;

void throttle_interval_event(int index) {
    event_repeat(index, 27000000 / 60);

    gui_do_stuff();

    throttle_timer_wait();
}

bool emu_save_rom(const char *file) {
    FILE *savedRom = fopen(file, "wb");

    gui_set_busy(true);

    if (!savedRom) {
        return false;
    }

    bool success = (fwrite(mem.flash.block, 1, flash_size, savedRom) == flash_size);

    fclose(savedRom);

    gui_set_busy(false);

    return success;
}

bool emu_save(const char *file) {
    FILE *savedImage = fopen_utf8(file, "wb");
    size_t size = sizeof(emu_image_t);
    emu_image_t* image = (emu_image_t*)malloc(size);
    bool success = false;

    gui_set_busy(true);

    do {
        if (!image) {
            break;
        }

        if (!asic_save(image)) {
            break;
        }

        image->version = imageVersion;

        success = (fwrite(image, 1, size, savedImage) == size);
    } while(0);

    free(image);
    fclose(savedImage);
    gui_set_busy(false);

    return success;
}

bool emu_start(const char *romImage, const char *savedImage) {
    bool ret = false;
    long lSize;
    FILE *imageFile;

    gui_set_busy(true);

    do {
        if(savedImage != NULL) {
            emu_image_t *image;
            imageFile = fopen_utf8(savedImage, "rb");

            if (!imageFile) {
                break;
            }
            if (fseek(imageFile, 0L, SEEK_END) < 0) {
                break;
            }
            lSize = ftell(imageFile);
            if (lSize < 0) {
                break;
            }
            if (fseek(imageFile, 0L, SEEK_SET) < 0) {
                break;
            }
            if((size_t)lSize < sizeof(emu_image_t)) {
                break;
            }

            image = (emu_image_t*)malloc(lSize);
            if(!image) {
                break;
            }
            if(fread(image, lSize, 1, imageFile) != 1) {
                free(image);
                break;
            }

            sched_reset();
            sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
            sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

            asic_init();
            asic_reset();

            if(image->version != imageVersion || !asic_restore(image)) {
                emu_cleanup();
                free(image);
                break;
            }
            free(image);
            ret = true;
        } else {
            asic_init();
            if (romImage == NULL) {
                gui_console_printf("[CEmu] No ROM image specified.\n");
                break;
            } else {
                FILE *romFile = fopen_utf8(romImage, "rb");
                do {
                    if (romFile) {
                        uint16_t field_type;
                        const uint8_t *outer;
                        const uint8_t *current;
                        const uint8_t *data;
                        uint32_t outer_field_size;
                        uint32_t data_field_size;
                        ti_device_t device_type;
                        uint32_t offset;

                        /* Get ROM file size */
                        if (fseek(romFile, 0L, SEEK_END) < 0) {
                            break;
                        }
                        lSize = ftell(romFile);
                        if (lSize < 0) {
                            break;
                        }
                        if (fseek(romFile, 0L, SEEK_SET) < 0) {
                            break;
                        }

                        /* Read whole ROM. */
                        if (fread(asic.mem->flash.block, 1, lSize, romFile) < (size_t)lSize) {
                            break;
                        }

                        if (asic.mem->flash.block[0x7E] == 0xFE) {
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
                            device_type = (ti_device_t)(data[1]);

                            /* If we come here, we've found something. */
                            ret = true;
                            break;
                        }

                        if (ret) {
                            set_device_type(device_type);
                        }
                    }
                } while(0);

                if (romFile) {
                    fclose(romFile);
                }
            }
        }
    } while(0);

    if(imageFile) {
        fclose(imageFile);
    }

    if (!ret) {
        gui_console_printf("[CEmu] Error opening image (Corrupted certificate?)\n");
        emu_cleanup();
    }

    gui_set_busy(false);

    return ret;
}


void emu_cleanup(void) {
    asic_free();
}

static void emu_reset(void) {
    /* Reset the scheduler */
    sched_reset();

    sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

    /* Reset the ASIC */
    asic_reset();

    /* Drain everything */
    cpuEvents = EVENT_NONE;

    sched_update_next_event();
}

static void emu_main_loop_inner(void) {
        if (cpuEvents & EVENT_RESET) {
            gui_console_printf("[CEmu] Calculator reset triggered...\n");
            cpu_reset();
            cpuEvents &= ~EVENT_RESET;
        }
#ifdef DEBUG_SUPPORT
        if (!cpu.halted && (cpuEvents & EVENT_DEBUG_STEP)) {
            cpuEvents &= ~EVENT_DEBUG_STEP;
            open_debugger(DBG_STEP, 0);
        }
#endif
        if (!asic.ship_mode_enabled) {
            sched_process_pending_events();
            cpu_execute();
        } else {
            gui_emu_sleep();
        }
}

void emu_loop(bool reset) {
    if (reset) {
        emu_reset();
    }

    exiting = false;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emu_main_loop_inner, 0, 1);
#else
    while (!exiting) {
        emu_main_loop_inner();
    }
#endif
    emu_cleanup();
}

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include "emu.h"
#include "asic.h"
#include "cert.h"
#include "os/os.h"
#include "schedule.h"
#include "debug/debug.h"

#define IMAGE_VERSION 0xCECE000F

uint32_t cpuEvents;
volatile bool exiting;
volatile bool emu_allow_instruction_commands = false; /* fixes alignment issues, apparently... */

void throttle_interval_event(int index) {
    event_repeat(index, 27000000 / 60);

    gui_do_stuff();

    throttle_timer_wait();
}

bool emu_save_rom(const char *name) {
    bool success = false;
    FILE *file = fopen(name, "wb");
    if (!file) {
        return false;
    }

    success = (fwrite(mem.flash.block, 1, SIZE_FLASH, file) == SIZE_FLASH);

    fclose(file);

    return success;
}

bool emu_save(const char *name) {
    FILE *file = NULL;
    emu_image_t *image = NULL;
    size_t size = sizeof(emu_image_t);
    bool success = false;

    file = fopen_utf8(name, "wb");
    if (!file) {
        return false;
    }

    image = (emu_image_t*)malloc(size);

    do {
        if (!image) {
            break;
        }

        if (!asic_save(image)) {
            break;
        }

        image->version = IMAGE_VERSION;

        success = (fwrite(image, 1, size, file) == size);
    } while (0);

    free(image);
    fclose(file);

    return success;
}

bool emu_load(const char *romName, const char *imageName) {
    bool ret = false;
    long lSize;

    if (imageName) {
        FILE *imageFile = NULL;
        emu_image_t *image = NULL;
        imageFile = fopen_utf8(imageName, "rb");

        if (!imageFile)                                goto ierr;
        if (fseek(imageFile, 0L, SEEK_END) < 0)        goto ierr;
        lSize = ftell(imageFile);
        if (lSize < 0)                                 goto ierr;
        if (fseek(imageFile, 0L, SEEK_SET) < 0)        goto ierr;
        if ((size_t)lSize < sizeof(emu_image_t))       goto ierr;

        if (!(image = (emu_image_t*)malloc(lSize)))    goto ierr;

        if (fread(image, lSize, 1, imageFile) != 1)    goto ierr;

        asic_init();
        sched_reset();
        asic_reset();

        if (image->version != IMAGE_VERSION)           goto ierr;
        if (!asic_restore(image))                      goto ierr;

        ret = true;
ierr:
        free(image);
    } else if (romName) {
        bool gotType = false;
        uint16_t field_type;
        const uint8_t *outer;
        const uint8_t *current;
        const uint8_t *data;
        uint32_t outer_field_size;
        uint32_t data_field_size;
        ti_device_t type;
        uint32_t offset;
        FILE *romFile = fopen_utf8(romName, "rb");

        if (!romFile)                                  goto rerr;

        if (fseek(romFile, 0L, SEEK_END) < 0)          goto rerr;
        lSize = ftell(romFile);
        if (lSize < 0)                                 goto rerr;
        if (fseek(romFile, 0L, SEEK_SET) < 0)          goto rerr;

        asic_init();

        if (fread(mem.flash.block, 1, lSize, romFile) < (size_t)lSize) goto rerr;

        /* Parse certificate fields to determine model.                            */
        /* We've heard of the OS base being at 0x30000 on at least one calculator. */
        for (offset = 0x20000U; offset < 0x40000U; offset += 0x10000U) {
            outer = mem.flash.block;
            /* Outer 0x800(0) field. */
            if (cert_field_get(outer + offset, SIZE_FLASH - offset, &field_type, &outer, &outer_field_size)) goto rerr;
            if (field_type != 0x800F) continue;

            /* Inner 0x801(0) field: calculator model */
            if (cert_field_get(outer, outer_field_size, &field_type, &data, &data_field_size)) break;
            if (field_type != 0x8012 || data[0] != 0x13) break;

            /* Inner 0x802(0) field: skip. */
            data_field_size = outer_field_size - (data + data_field_size - outer);
            data = outer;
            if (cert_field_next(&data, &data_field_size)) break;
            current = data;
            if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) break;
            if (field_type != 0x8021) break;

            /* Inner 0x803(0) field: skip. */
            data_field_size = outer_field_size - (data + data_field_size - outer);
            data = current;
            if (cert_field_next(&data, &data_field_size)) break;
            current = data;
            if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) break;
            if (field_type != 0x8032) break;

            /* Inner 0x80A(0) field: skip. */
            data_field_size = outer_field_size - (data + data_field_size - outer);
            data = current;
            if (cert_field_next(&data, &data_field_size)) break;
            current = data;
            if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) break;
            if (field_type != 0x80A1) break;

            /* Inner 0x80C(0) field: keep. */
            data_field_size = outer_field_size - (data + data_field_size - outer);
            data = current;
            if (cert_field_next(&data, &data_field_size)) break;
            current = data;
            if (cert_field_get(current, data_field_size, &field_type, &data, &data_field_size)) break;
            if (field_type != 0x80C2) break;

            if (data[1] != 0 && data[1] != 1) break;
            type = (ti_device_t)(data[1]);

            /* If we come here, we've found something. */
            gotType = true;
            break;
        }

        if (gotType) {
            set_device_type(type);
        } else {
            set_device_type(TI84PCE);
            gui_console_printf("[CEmu] Could not determine device type.\n");
        }

        ret = true;
    }
rerr:

    if (!ret) {
        emu_cleanup();
    }

    return ret;
}

void emu_cleanup(void) {
    asic_free();
}

static void EMSCRIPTEN_KEEPALIVE emu_reset(void) {
    sched_reset();
    asic_reset();

    cpuEvents = EVENT_NONE;

    sched_update_next_event();
}

static void emu_main_loop_inner(void) {
#ifdef DEBUG_SUPPORT
    if (cpuEvents & (EVENT_RESET | EVENT_DEBUG_STEP)) {
        if (!cpu.halted && cpuEvents & EVENT_DEBUG_STEP) {
            cpuEvents &= ~EVENT_DEBUG_STEP;
            open_debugger(DBG_STEP, 0);
        }
#endif
        if (cpuEvents & EVENT_RESET) {
            gui_console_printf("[CEmu] Reset triggered.\n");
            asic_reset();
            cpuEvents &= ~EVENT_RESET;
        }
#ifdef DEBUG_SUPPORT
    }
#endif

    sched_process_pending_events();
    cpu_execute();
}

void emu_loop(bool reset) {
    if (reset) {
        emu_reset();
    }

    exiting = false;
    while (!exiting) {
        emu_main_loop_inner();
    }
    emu_cleanup();
}

#include "emu.h"
#include "mem.h"
#include "asic.h"
#include "cpu.h"
#include "cert.h"
#include "os/os.h"
#include "schedule.h"
#include "debug/debug.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define IMAGE_VERSION 0xCECE0012

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
    bool success = false;
    uint32_t version = IMAGE_VERSION;

    file = fopen_utf8(name, "wb");
    if (file) {
        if (fwrite(&version, sizeof(version), 1, file) == 1 && asic_save(file)) {
            success = true;
        }
    }

    fclose(file);

    return success;
}

bool emu_load(const char *romName, const char *imageName) {
    uint32_t version;
    bool ret = false;
    FILE *file = NULL;

    if (imageName) {
        file = fopen_utf8(imageName, "rb");

        gui_console_printf("[CEmu] Loading Emulator Image...\n");

        if (!file) goto rerr;
        if (fread(&version, sizeof(version), 1, file) != 1) goto rerr;
        if (version != IMAGE_VERSION) goto rerr;

        asic_init();
        sched_reset();
        asic_reset();

        if (!asic_restore(file)) goto rerr;

        ret = true;
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
        size_t size;

        gui_console_printf("[CEmu] Loading ROM Image...\n");

        file = fopen_utf8(romName, "rb");

        if (!file) {
            gui_console_printf("[CEmu] ROM Error: File nonexistent\n");
            goto rerr;
        }

        if (fseek(file, 0L, SEEK_END) < 0) goto rerr;
        size = ftell(file);
        if (size > SIZE_FLASH) {
            gui_console_printf("[CEmu] ROM Error: Invalid size (%u bytes | max %u bytes)\n", (unsigned int)size, SIZE_FLASH);
            goto rerr;
        }
        rewind(file);

        asic_init();

        if (fread(mem.flash.block, size, 1, file) != 1) {
            gui_console_printf("[CEmu] ROM Error: Reading ROM image\n");
            goto rerr;
        }

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

        ret = true;

        if (gotType) {
            set_device_type(type);
        } else {
            set_device_type(TI84PCE);
            gui_console_printf("[CEmu] Could not determine device type.\n");
        }
    }
rerr:

    if (file) {
        fclose(file);
    }

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

#include "emu.h"
#include "mem.h"
#include "asic.h"
#include "cpu.h"
#include "cert.h"
#include "os/os.h"
#include "defines.h"
#include "schedule.h"
#include "debug/debug.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#define IMAGE_VERSION 0xCECE0013

void EMSCRIPTEN_KEEPALIVE emu_exit(void) {
    cpu.abort = CPU_ABORT_EXIT;
}

void EMSCRIPTEN_KEEPALIVE emu_reset(void) {
    asic_reset();
}

bool emu_save(bool image, const char *path) {
    FILE *file = NULL;
    bool success = false;
    uint32_t version = IMAGE_VERSION;

    file = fopen_utf8(path, "wb");

    if (file) {
        if (image) {
            if (fwrite(&version, sizeof(version), 1, file) == 1 && asic_save(file)) {
                success = true;
            }
        } else {
            success = (fwrite(mem.flash.block, 1, SIZE_FLASH, file) == SIZE_FLASH);
        }
        fclose(file);
    }

    return success;
}

int emu_load(bool image, const char *path) {
    uint32_t version;
    int ret = EMU_LOAD_FAIL;
    FILE *file = NULL;

    if (!path) {
        return ret;
    }

    if (image) {
        file = fopen_utf8(path, "rb");

        gui_console_printf("[CEmu] Loading Emulator Image...\n");

        if (!file) {
            gui_console_printf("[CEmu] Image file nonexistent.\n");
            goto rerr;
        }

        if (fread(&version, sizeof(version), 1, file) != 1) goto rerr;

        if (version != IMAGE_VERSION) {
            gui_console_printf("[CEmu] Error in versioning.\n");
            goto rerr;
        }

        asic_init();
        asic_reset();

        if (!asic_restore(file)) {
            gui_console_printf("[CEmu] Error reading image.\n");
            goto rerr;
        }

        gui_console_printf("[CEmu] Loaded Emulator Image.\n");

        ret = EMU_LOAD_OKAY;
    } else {
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

        file = fopen_utf8(path, "rb");

        if (!file) {
            gui_console_printf("[CEmu] ROM file nonexistent.\n");
            goto rerr;
        }

        if (fseek(file, 0L, SEEK_END) < 0) goto rerr;
        size = ftell(file);
        if (size > SIZE_FLASH) {
            gui_console_printf("[CEmu] Invalid ROM size (%u bytes | max %u bytes)\n", (unsigned int)size, SIZE_FLASH);
            goto rerr;
        }
        rewind(file);

        asic_init();

        if (fread(mem.flash.block, size, 1, file) != 1) {
            gui_console_printf("[CEmu] Error reading ROM image\n");
            goto rerr;
        }

        /* Parse certificate fields to determine model. */
        for (offset = 0x20000U; offset < 0x40000U; offset += 0x10000U) {
            outer = mem.flash.block;

            /* Outer 0x800(0) field. */
            if (cert_field_get(outer + offset, SIZE_FLASH - offset, &field_type, &outer, &outer_field_size)) break;
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

        ret = EMU_LOAD_OKAY;

        if (gotType) {
            set_device_type(type);
        } else {
            set_device_type(TI84PCE);
            gui_console_printf("[CEmu] Could not determine device type.\n");
            ret = EMU_LOAD_NOT_A_CE;
        }

        asic_reset();
    }
rerr:

    if (file) {
        fclose(file);
    }

    if (ret == EMU_LOAD_FAIL) {
        asic_free();
    }

    return ret;
}

#ifdef __EMSCRIPTEN__

extern bool throttle_triggered;

static void emu_main_loop_inner(void) {
    throttle_triggered = false;

    while (true) {
        if (cpu.abort == CPU_ABORT_EXIT) {
            emscripten_cancel_main_loop();
            return;
        }
        sched_process_pending_events();
        if (cpu.abort == CPU_ABORT_RESET) {
            cpu_transition_abort(CPU_ABORT_RESET, CPU_ABORT_NONE);
            gui_console_printf("[CEmu] Reset triggered.\n");
            asic_reset();
        }
        if (throttle_triggered) break;
        cpu_execute();
    }
}

void emu_loop(void) {
    cpu.abort = CPU_ABORT_NONE;
    emscripten_set_main_loop(emu_main_loop_inner, 60, 1);
    asic_free();
}

#else

void emu_loop(void) {
    cpu.abort = CPU_ABORT_NONE;
    while (cpu.abort != CPU_ABORT_EXIT) {
        sched_process_pending_events();
        if (cpu.abort == CPU_ABORT_RESET) {
            cpu_transition_abort(CPU_ABORT_RESET, CPU_ABORT_NONE);
            gui_console_printf("[CEmu] Reset triggered.\n");
            asic_reset();
#ifdef DEBUG_SUPPORT
            gui_debug_open(DBG_READY, 0);
#endif
        }
        cpu_execute();
    }
    asic_free();
}

#endif

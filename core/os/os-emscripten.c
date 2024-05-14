#ifdef __EMSCRIPTEN__

#include "os.h"

#include "../../core/asic.h"
#include "../../core/emu.h"
#include "../../core/lcd.h"
#include "../../core/link.h"
#include "../../core/panel.h"
#include "../../core/debug/debug.h"

#include <emscripten.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <stdarg.h>

#include <time.h>

static char file_buf[500] = {0};

static bool transfer_progress_cb(void *context, int value, int total) {
    (void)context;
    EM_ASM({
        if (window.transferProgressCallback) { transferProgressCallback($0, $1); }
    }, value, total);
    return false;
}

static int emu_send_variable_with_cb(const char *file, int location, usb_progress_handler_t *progress_handler) {
    return emu_send_variables(&file, 1, location, progress_handler, NULL);
}

static void gui_do_stuff(void) {
    if (unlikely(file_buf[0] != '\0')) {
        if (emu_send_variable_with_cb(file_buf, LINK_FILE, transfer_progress_cb) != LINK_GOOD) {
            fprintf(stderr, "[CEmu] Error sending file to emu: %s\n", file_buf);
            fflush(stderr);
            EM_ASM(
                if (window.transferErrorCallback) { transferErrorCallback() };
            );
        }
        file_buf[0] = 0;
    }
}

static void emu_loop(void) {
    emu_run(1u);
    gui_do_stuff();
}

void gui_console_clear(void) {}
void gui_debug_close(void) {}

void gui_debug_open(int reason, uint32_t data) {
    printf("[CEmu debug] (reason=%d, data=0x%X)\n", reason, data);
    fflush(stderr);
}

void gui_console_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
    va_end(ap);
}

void gui_console_err_printf(const char *fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
    va_end(ap);
}

FILE *fopen_utf8(const char *filename, const char *mode) {
    return fopen(filename, mode);
}

void EMSCRIPTEN_KEEPALIVE set_file_to_send(const char* path) {
    strcpy(file_buf, path);
}

uint8_t* EMSCRIPTEN_KEEPALIVE lcd_get_frame() {
    return &(panel.display[0][0][0]);
}

int EMSCRIPTEN_KEEPALIVE emsc_set_main_loop_timing(int mode, int value) {
    return emscripten_set_main_loop_timing(mode, value);
}

void EMSCRIPTEN_KEEPALIVE emsc_pause_main_loop() {
    emscripten_pause_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_resume_main_loop() {
    emscripten_resume_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_cancel_main_loop() {
    emu_reset();
    emscripten_cancel_main_loop();
}

int main(int argc, char* argv[]) {
    int success;
    (void)argc;
    (void)argv;

    success = emu_load(EMU_DATA_ROM, "CE.rom");

    if (success == EMU_STATE_VALID) {
#ifdef DEBUG_SUPPORT
        debug_init();
        debug_flag(DBG_SOFT_COMMANDS, true);
#endif
        emu_set_lcd_spi(1);
        emu_set_lcd_gamma(1);
        EM_ASM(
            emul_is_inited = true;
            emul_is_paused = false;
            initFuncs();
            initLCD();
            enableGUI();
        );
        emscripten_set_main_loop(emu_loop, 60, 1);
    } else {
        EM_ASM(
            emul_is_inited = false;
            disableGUI();
            alert("Error: Couldn't start emulation ; bad ROM?");
        );
        return 1;
    }

    puts("Finished");

    EM_ASM(
        emul_is_inited = false;
        disableGUI();
    );

    return 0;
}

#endif

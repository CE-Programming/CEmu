#ifdef __EMSCRIPTEN__

#include "os.h"

#include "../../core/asic.h"
#include "../../core/emu.h"
#include "../../core/lcd.h"
#include "../../core/link.h"
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

bool throttle_triggered;
char file_buf[500];

void gui_throttle() {
    throttle_triggered = true;
}

void gui_do_stuff() {
    if (file_buf[0] != '\0') {
        if (!sendVariableLink(file_buf, LINK_FILE)) {
            fprintf(stderr, "Error sending file to emu: %s\n", file_buf);
            fflush(stderr);
        }
        file_buf[0] = 0;
    }
}

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
    return &(spi.display[0][0][0]);
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

    success = emu_load(false, "CE.rom");

    if (success == EMU_LOAD_OKAY) {
#ifdef DEBUG_SUPPORT
        debug_init();
        debug.commands = true;
#endif
        lcd.spi = true;
        EM_ASM(
            emul_is_inited = true;
            emul_is_paused = false;
            initFuncs();
            initLCD();
            enableGUI();
        );
        emu_loop();
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

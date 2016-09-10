#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#define _BSD_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include "os.h"

#include "../../core/emu.h"
#include "../../core/lcd.h"

extern lcd_state_t lcd;

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait() {}

void gui_emu_sleep() { usleep(500); }
void gui_do_stuff() {}
void gui_set_busy(bool busy) {}

void gui_console_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
}

void gui_console_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gui_console_vprintf(fmt, ap);
    va_end(ap);
}

void gui_perror(const char *msg)
{
    gui_console_printf("[Error] %s: %s\n", msg, strerror(errno));
}

void EMSCRIPTEN_KEEPALIVE paintLCD(uint32_t *dest)
{
    if (lcd.control & 0x800) { // LCD on
        lcd_drawframe(dest, &lcd);
    } else { // LCD off
        EM_ASM(drawLCDOff());
    }
}

void EMSCRIPTEN_KEEPALIVE emsc_pause_main_loop() {
    emscripten_pause_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_resume_main_loop() {
    emscripten_resume_main_loop();
}

void EMSCRIPTEN_KEEPALIVE emsc_cancel_main_loop() {
    emu_cleanup();
    emscripten_cancel_main_loop();
}

int main(int argc, char* argv[])
{
    bool success;
    emulationPaused = false;

    success = emu_start("CE.rom", NULL);

    if (success) {
        EM_ASM(
            emul_is_inited = true;
            emul_is_paused = false;
            initFuncs();
            initLCD();
            enableGUI();
        );
        emu_loop(true);
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
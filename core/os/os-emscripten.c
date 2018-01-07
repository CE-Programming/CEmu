#ifdef __EMSCRIPTEN__

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

#include "os.h"

#include "../../core/emu.h"
#include "../../core/lcd.h"
#include "../../core/link.h"
#include "../../core/debug/debug.h"

extern lcd_state_t lcd;

char file_buf[500];

FILE *fopen_utf8(const char *filename, const char *mode)
{
    return fopen(filename, mode);
}

unsigned int sleep_amount_us = 10000;
void EMSCRIPTEN_KEEPALIVE set_sleep_amount_us(unsigned int amount)
{
    sleep_amount_us = amount;
}

bool throttle_triggered = false;

void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait() {
    //EM_ASM( Module.print('hello throttle_timer_wait') );
    //usleep(sleep_amount_us);
    throttle_triggered = true;
}

void gui_emu_sleep(unsigned long microseconds) {
    //usleep(microseconds);
}

void EMSCRIPTEN_KEEPALIVE set_file_to_send(const char* path)
{
    strcpy(file_buf, path);
}

void gui_do_stuff()
{
#ifdef DEBUG_SUPPORT
    if (debugger.bufferPos) {
        debugger.buffer[debugger.bufferPos] = '\0';
        fprintf(stderr, "[CEmu DbgOutPrint] %s\n", debugger.buffer);
        fflush(stderr);
        debugger.bufferPos = 0;
    }

    if (debugger.bufferErrPos) {
        debugger.bufferErr[debugger.bufferErrPos] = '\0';
        fprintf(stderr, "[CEmu DbgErrPrint] %s\n", debugger.bufferErr);
        fflush(stderr);
        debugger.bufferErrPos = 0;
    }
#endif
    if (file_buf[0] != '\0') {
        if (!sendVariableLink(file_buf, LINK_FILE))
        {
            fprintf(stderr, "Error sending file to emu: %s\n", file_buf);
            fflush(stderr);
        }
        file_buf[0] = 0;
    }
}

void gui_set_busy(bool busy) { (void)busy; }

void gui_debugger_raise_or_disable(bool entered)
{
    (void)entered;
#ifdef DEBUG_SUPPORT
    inDebugger = false;
#endif
}

void gui_debugger_send_command(int reason, uint32_t addr)
{
    printf("[CEmu Debugger] Got software command (r=%d, addr=0x%X)\n", reason, addr);
    fflush(stderr);
#ifdef DEBUG_SUPPORT
    inDebugger = false;
#endif
}

void gui_console_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
}

void gui_console_err_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stderr, fmt, ap);
    fflush(stderr);
}

void gui_console_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    gui_console_vprintf(fmt, ap);
    va_end(ap);
}

void gui_console_err_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_console_err_vprintf(fmt, ap);

    va_end(ap);
}

void gui_perror(const char *msg)
{
    fprintf(stderr, "[gui_perror] %s: %s\n", msg, strerror(errno));
}

uint32_t* EMSCRIPTEN_KEEPALIVE lcd_get_frame() {
    return spi.display;
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
    (void)argc;
    (void)argv;

    success = emu_load("CE.rom", NULL);

    if (success) {
#ifdef DEBUG_SUPPORT
        debugger_init();
#endif
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

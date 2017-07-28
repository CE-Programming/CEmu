#ifdef __EMSCRIPTEN__

#include <emscripten.h>

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <pthread.h>
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

void throttle_timer_off() {}
void throttle_timer_on() {}
void throttle_timer_wait() {
    //EM_ASM( Module.print('hello throttle_timer_wait') );
    usleep(10000);
}

void gui_emu_sleep(unsigned long microseconds) {
    usleep(microseconds);
}

void EMSCRIPTEN_KEEPALIVE set_file_to_send(const char* path)
{
    strcpy(file_buf, path);
}

void gui_do_stuff()
{
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
    inDebugger = false;
}

void gui_debugger_send_command(int reason, uint32_t addr)
{
    printf("[CEmu Debugger] Got software command (r=%d, addr=0x%X)\n", reason, addr);
    fflush(stderr);
    inDebugger = false;
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

uint32_t * buf_lcd_js = NULL;

void EMSCRIPTEN_KEEPALIVE set_lcd_js_ptr(uint32_t * ptr) {
    buf_lcd_js = ptr;
}

void EMSCRIPTEN_KEEPALIVE paint_LCD_to_JS()
{
    if (buf_lcd_js && lcd.control & 0x800) { // LCD on
        lcd_drawframe(buf_lcd_js, &lcd);
    } else { // LCD off
        //EM_ASM( drawLCDOff() );
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

static void* emu_loop_wrapper(void *arg)
{
    (void)arg;
    emu_loop(true);
    return NULL;
}

static void* lcd_paint_wrapper(void *arg)
{
    (void)arg;
    while (1)
    {
        paint_LCD_to_JS();
        usleep(20000);
    }
    return NULL;
}

void nothing()
{
    usleep(50);
}

int main(int argc, char* argv[])
{
    bool success;
    (void)argc;
    (void)argv;

    success = emu_load("CE.rom", NULL);

    if (success) {
        pthread_t lcd_thread;
        pthread_t lcd_thread2;
        debugger_init();
        EM_ASM(
            emul_is_inited = true;
            emul_is_paused = false;
            initFuncs();
            initLCD();
            enableGUI();
        );
        if(pthread_create(&lcd_thread,  NULL, emu_loop_wrapper, NULL))
        {
            fprintf(stderr, "Error creating emu loop thread\n");
            return 1;
        } else {
            fprintf(stderr, "Emu loop thread created OK\n");
            if(pthread_create(&lcd_thread2, NULL, lcd_paint_wrapper, NULL))
            {
                fprintf(stderr, "Error creating lcd paint thread\n");
                return 1;
            } else {
                fprintf(stderr, "lcd paint thread created OK\n");
            }
        }
    } else {
        EM_ASM(
            emul_is_inited = false;
            disableGUI();
            alert("Error: Couldn't start emulation ; bad ROM?");
        );
        return 1;
    }

    emscripten_set_main_loop(nothing, 0, 1);

    puts("Finished");

    EM_ASM(
        emul_is_inited = false;
        disableGUI();
    );

    return 0;
}

#endif

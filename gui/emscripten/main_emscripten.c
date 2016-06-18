#include <emscripten.h>

#define _BSD_SOURCE
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>

#include "../../core/emu.h"
#include "../../core/lcd.h"
//#include "../../core/debug/debug.h"

extern lcd_state_t lcd;

void gui_emu_sleep(void) {
    //puts("gui_emu_sleep");
}

void gui_do_stuff(void) {
    //puts("gui_do_stuff");
}

void gui_set_busy(bool busy) {
    puts("gui_set_busy (todo)");
}

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

void throttle_timer_off() {
    puts("throttle_timer_off");
}
void throttle_timer_on() {
    puts("throttle_timer_on");
}
void throttle_timer_wait()
{
    //puts("throttle_timer_wait");
    //usleep(10000);
}

/*
void gui_console_debug_char(const char c) {
    puts("gui_console_debug_char (todo)");
}
void gui_debugger_entered_or_left(bool entered)
{
    puts("gui_debugger_entered_or_left");
}
void gui_debugger_raise_or_disable(bool entered)
{
    puts("gui_debugger_raise_or_disable");
}
void gui_debugger_send_command(int reason, uint32_t addr)
{
    printf("gui_debugger_send_command: reason: %d ; addr = %u \n", reason, addr);
    //inDebugger = false;
}
*/

void test_gui_callback(void)
{
    puts("lcd_event_gui_callback");
}

void EMSCRIPTEN_KEEPALIVE paintLCD(uint32_t *dest)
{
    if (lcd.control & 0x800) {
        lcd_drawframe(dest, &lcd);
    } else {
        //puts("paintLCD (off)");
    }
}

void EMSCRIPTEN_KEEPALIVE press_key(int row, int col, int state)
{
    gui_console_printf("press_key: %d %d %d\n", row, col, state);
    keypad_key_event(row, col, state);
}

int main(int argc, char* argv[])
{
    //lcd_event_gui_callback = test_gui_callback;

    bool success = emu_start(/* todo : the rom file here, when loaded */, NULL);

    if (success) {
        EM_ASM(initLCD());
        emu_loop(true);
    } else {
        puts("Error: Couldn't start emulation");
    }

    puts("finished");

    return 0;
}

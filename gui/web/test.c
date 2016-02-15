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


bool throttleOn = true;
int speed, actualSpeed;

void gui_emu_sleep(void) {
//    usleep(50);
}

void gui_do_stuff(void)
{
    // std::cerr << "gui_do_stuff" << std::endl;
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

void gui_console_debug_char(const char c) {
    // std::cerr << "gui_console_debug_char" << std::endl;
}

void gui_perror(const char *msg)
{
    gui_console_printf("[Error] %s: %s\n", msg, strerror(errno));
}

void throttle_timer_off()
{
    // std::cerr << "throttle_timer_off" << std::endl;
}

void throttle_timer_on()
{
    // std::cerr << "throttle_timer_on" << std::endl;
}

void throttle_timer_wait()
{
    // std::cerr << "throttle_timer_wait" << std::endl;
    return;
}

void gui_debugger_entered_or_left(bool entered)
{
    // std::cerr << "gui_debugger_entered_or_left" << std::endl;
}

void EMSCRIPTEN_KEEPALIVE paintLCD(uint32_t *dest)
{
    lcd_drawframe(dest);
}

int main(int argc, char* argv[])
{
    puts("Hello");

    speed = actualSpeed = 100;

    rom_image = strdup("84pce_51.rom");

    bool reset_true = true;
    bool success = emu_start();

    puts("started");

    if (success) {
        emu_loop(reset_true);
    }

    puts("finished");

    return 0;
}

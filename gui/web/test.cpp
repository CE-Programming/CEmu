#include <emscripten.h>

#include <cstdarg>
#include <cstdio>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <iostream>
#include <thread>

#include "../../core/emu.h"
#include "../../core/lcd.h"

extern "C" {

bool throttleOn = true;
int speed, actualSpeed;
std::chrono::steady_clock::time_point lastTime;

uint16_t tmp_buff_lcd[240*320];
uint32_t bitfields[] = { 0x01F, 0x000, 0x000 };

void gui_emu_sleep(void) {
    usleep(50);
}

void gui_do_stuff(void)
{
    // std::cerr << "gui_do_stuff" << std::endl;
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now();
    lastTime += std::chrono::steady_clock::now() - cur_time;
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

    std::chrono::duration<int, std::ratio<100, 60>> unit(1);
    std::chrono::steady_clock::duration interval(std::chrono::duration_cast<std::chrono::steady_clock::duration>
                                                 (std::chrono::duration<int, std::ratio<1, 60 * 1000000>>(1000000 * 100 / speed)));
    std::chrono::steady_clock::time_point cur_time = std::chrono::steady_clock::now(), next_time = lastTime + interval;
    if (throttleOn && cur_time < next_time) {
        lastTime = next_time;
        std::this_thread::sleep_until(next_time);
    } else {
        lastTime = cur_time;
        std::this_thread::yield();
    }
}

void gui_debugger_entered_or_left(bool entered)
{
    // std::cerr << "gui_debugger_entered_or_left" << std::endl;
}

void EMSCRIPTEN_KEEPALIVE paintLCD(uint32_t *dest)
{
    lcd_drawframe((uint16_t*)tmp_buff_lcd, bitfields);

    int x, y;
    uint32_t tmp, R, G, B;

    for (y=0; y < 240; y++)
    {
        for (x=0; x < 320; x++)
        {
            tmp = *(tmp_buff_lcd+y*320+x);
            R = tmp & 0x1f;
            G = (tmp >> 5) & 0x3f;
            B = (tmp >> 11) & 0x1f;

            *(dest+y*320+x) = (R << 8) | (G << 16) | (B << 24);
        }
    }

}

int main(int argc, char* argv[])
{
    std::cout << "Hello" << std::endl;

    speed = actualSpeed = 100;
    lastTime = std::chrono::steady_clock::now();

    rom_image = strdup("84pce_51.rom");

    bool reset_true = true;
    bool success = emu_start();

    std::cout << "started" << std::endl;

    if (success) {
        emu_loop(reset_true);
    }

    std::cout << "finished" << std::endl;

    return 0;
}

} // extern C

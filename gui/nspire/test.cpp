#include <cstdarg>
#include <cstdio>
#include <cstring>
#include <cassert>
#include <chrono>
#include <unistd.h>
#include <iostream>
#include "asic.h"
#include "emu.h"
#include "debug/debug.h"

extern "C" {


void gui_do_stuff(bool wait)
{
    // std::cerr << "gui_do_stuff, wait = " << wait << std::endl;
}

void gui_console_printf(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);

    gui_console_vprintf(fmt, ap);

    va_end(ap);
}

void gui_console_vprintf(const char *fmt, va_list ap)
{
    vfprintf(stdout, fmt, ap);
    fflush(stdout);
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
    unsigned int now = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
    unsigned int throttle = throttle_delay * 1000;
    unsigned int left = throttle - (now % throttle);
    if (left > 0) {
        std::cerr << " usleep " << std::endl;
        //usleep(left);
    }
}

void gui_debugger_entered_or_left(bool entered)
{
    if(entered != 0) {
        std::cerr << "emu_thread->debuggerEntered(entered); entered = " << entered << std::endl;
    }
}

void getScreenshot(void)
{
    uint16_t *framebuffer = reinterpret_cast<uint16_t*>(malloc(320 * 240 * 2));
    uint32_t bitfields[] = { 0x01F, 0x000, 0x000 };

    lcd_drawframe(framebuffer, bitfields);

    char buf[5] = {0};
    for (uint32_t i = 0; i < 320 * 240 * 2; i++)
    {
        sprintf(buf, "%04X", framebuffer[i]);
        std::cerr << buf << "\t";
    }
    std::cerr << std::endl;
    free(framebuffer);
}

int main(int argc, char* argv[])
{
    srand(time(NULL));
    
    std::cout << "Hello" << std::endl;

    rom_image = strdup("84pce_51.rom.tns");

    bool reset_true = true;
    bool success = emu_start();

    std::cout << "started" << std::endl;

    if(success) { emu_loop(reset_true); }

    std::cout << "finished" << std::endl;

    return 0;
}

} // extern C

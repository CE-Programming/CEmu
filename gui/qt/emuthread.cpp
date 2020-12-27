#include "emuthread.h"

#include "../../core/emu.h"

void gui_console_clear(void) {
}

void gui_console_printf(const char *format, ...) {
    (void)format;
}

void gui_console_err_printf(const char *format, ...) {
    (void)format;
}

void gui_debug_open(int reason, uint32_t data) {
    (void)reason;
    (void)data;
}

void gui_debug_close(void) {
}

#ifndef _H_EMU
#define _H_EMU

#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Settings
extern volatile bool exiting;
extern const char *rom_image;

// GUI callbacks
void gui_do_stuff();
void gui_debug_printf(const char *fmt, ...);
void gui_debug_vprintf(const char *fmt, va_list ap);
void gui_perror(const char *msg);

int emulate(void);
void emu_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif

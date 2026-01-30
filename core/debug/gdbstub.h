#ifndef GDBSTUB_H
#define GDBSTUB_H

#include <stdbool.h>
#include <stdint.h>

bool gdbstub_init(unsigned int port);
bool gdbstub_init_from_env(void);
void gdbstub_shutdown(void);
void gdbstub_poll(void);
bool gdbstub_on_debug(int reason, uint32_t addr);
bool gdbstub_is_connected(void);

#endif

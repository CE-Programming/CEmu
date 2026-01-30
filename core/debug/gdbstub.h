#ifndef GDBSTUB_H
#define GDBSTUB_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool gdbstub_init(unsigned int port);
bool gdbstub_init_from_env(void);
void gdbstub_shutdown(void);
void gdbstub_poll(void);
bool gdbstub_on_debug(int reason, uint32_t addr);
bool gdbstub_is_connected(void);
int gdbstub_watch_mask(uint32_t addr);
void gdbstub_watch_access(uint32_t addr, bool write);
void gdbstub_handle_watch(void);

#ifdef __cplusplus
}
#endif

#endif

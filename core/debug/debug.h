#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"

extern volatile bool in_debugger;

enum DBG_REASON {
    DBG_USER,
    DBG_EXCEPTION,
    DBG_EXEC_BREAKPOINT,
    DBG_READ_BREAKPOINT,
    DBG_WRITE_BREAKPOINT,
    DBG_PORT_WRITE_BREAKPOINT,
    DBG_PORT_READ_BREAKPOINT,
};

#define DBG_NO_HANDLE   0
#define DBG_PORT_READ   1
#define DBG_PORT_WRITE  2
#define DBG_PORT_FREEZE 4

uint8_t debug_port_read_byte(const uint32_t addr);
void debugger(enum DBG_REASON reason, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif

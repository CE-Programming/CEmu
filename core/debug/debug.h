#ifndef DEBUG_H
#define DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif

#include "../defines.h"

extern volatile bool in_debugger;

#define DBG_USER                  0
#define DBG_EXCEPTION             1
#define DBG_EXEC_BREAKPOINT       2
#define DBG_READ_BREAKPOINT       3
#define DBG_WRITE_BREAKPOINT      4
#define DBG_PORT_WRITE_BREAKPOINT 5
#define DBG_PORT_READ_BREAKPOINT  6

#define DBG_NO_HANDLE   0
#define DBG_PORT_READ   1
#define DBG_PORT_WRITE  2
#define DBG_PORT_FREEZE 4

uint8_t debug_port_read_byte(const uint32_t addr);
void debugger(int reason, uint32_t addr);

#ifdef __cplusplus
}
#endif

#endif

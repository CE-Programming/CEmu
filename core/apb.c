#include "apb.h"
#include "debug/debug.h"

/* Global APB state */
eZ80portrange_t apb_map[0x10];

uint8_t port_read_byte(uint16_t port) {
#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[port] & DBG_PORT_READ) {
        open_debugger(HIT_PORT_READ_BREAKPOINT, port);
    }
#endif
    return apb_map[port_range(port)].read_in(addr_range(port));
}

void port_write_byte(uint16_t port, uint8_t value) {
#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[port] & DBG_PORT_FREEZE) {
        return;
    }
#endif

    apb_map[port_range(port)].write_out(addr_range(port), value);

#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[port] & DBG_PORT_WRITE) {
        open_debugger(HIT_PORT_WRITE_BREAKPOINT, port);
    }
#endif
}

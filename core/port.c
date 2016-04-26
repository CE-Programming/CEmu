#include "port.h"
#include "debug/debug.h"

/* Global APB state */
eZ80portrange_t port_map[0x10];

uint8_t port_peek_byte(uint16_t address) {
    return port_map[port_range(address)].read_in(addr_range(address));
}
uint8_t port_read_byte(uint16_t address) {
#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[address] & DBG_PORT_READ) {
        open_debugger(HIT_PORT_READ_BREAKPOINT, address);
    }
#endif
    return port_peek_byte(address);
}

void port_poke_byte(uint16_t address, uint8_t value) {
    port_map[port_range(address)].write_out(addr_range(address), value);
}
void port_write_byte(uint16_t address, uint8_t value) {
#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[address] & DBG_PORT_FREEZE) {
        return;
    }
#endif
    port_poke_byte(address, value);
#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[address] & DBG_PORT_WRITE) {
        open_debugger(HIT_PORT_WRITE_BREAKPOINT, address);
    }
#endif
}

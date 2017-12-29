#include "port.h"
#include "cpu.h"
#include "debug/debug.h"

/* Global APB state */
eZ80portrange_t port_map[0x10];

#define port_range(a) (((a)>>12)&0xF) /* converts an address to a port range 0x0-0xF */

static const uint32_t port_mirrors[0x10] = {0xFF,0xFF,0xFF,0x1FF,0xFFF,0xFF,0x1F,0xFF,0x7F,0xFFF,0x7F,0xFFF,0xFF,0x7F,0x7F,0xFFF};

static uint8_t port_read(uint16_t address, uint8_t loc, bool peek) {
    return port_map[loc].read(address & port_mirrors[loc], peek);
}
uint8_t port_peek_byte(uint16_t address) {
    return port_read(address, port_range(address), true);
}
uint8_t port_read_byte(uint16_t address) {
    uint8_t port_loc = port_range(address);
    static const uint8_t port_read_cycles[0x10] = {2,2,2,4,3,3,3,3,3,3,3,3,3,3,3,3};

    cpu.cycles += port_read_cycles[port_loc];

#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[address] & DBG_PORT_READ) {
        open_debugger(HIT_PORT_READ_WATCHPOINT, address);
    }
#endif
    return port_read(address, port_loc, false);
}

static void port_write(uint16_t address, uint8_t loc, uint8_t value, bool peek) {
    port_map[loc].write(address & port_mirrors[loc], value, peek);
}
void port_poke_byte(uint16_t address, uint8_t value) {
    port_write(address, port_range(address), value, true);
}
void port_write_byte(uint16_t address, uint8_t value) {
    uint8_t port_loc = port_range(address);
    static const uint8_t port_write_cycles[0x10] = {2,2,2,4,2,3,3,3,3,3,3,3,3,3,3,3};

    cpu.cycles += port_write_cycles[port_loc];

#ifdef DEBUG_SUPPORT
    if (debugger.data.ports[address] & (DBG_PORT_FREEZE | DBG_PORT_WRITE)) {
        if (debugger.data.ports[address] & DBG_PORT_WRITE) {
            open_debugger(HIT_PORT_WRITE_WATCHPOINT, address);
        }
        if (debugger.data.ports[address] & DBG_PORT_FREEZE) {
            return;
        }
    }
#endif
    port_write(address, port_loc, value, false);
}

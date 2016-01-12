#include <stdio.h>

#include "apb.h"
#include "mem.h"
#include "emu.h"
#include "debug/debug.h"

/* Global APB state */
apb_map_entry_t apb_map[0x10];

void apb_set_map(int entry, eZ80portrange_t *range){
    apb_map[entry].range = range;
}

uint8_t port_read_byte(const uint16_t addr) {
    uint16_t port = (port_range(addr) << 12) | addr_range(addr);
    uint8_t value = apb_map[port_range(addr)].range->read_in(addr_range(addr));

    if (mem.debug.ports[port] & DBG_PORT_READ) {
        debugger(HIT_PORT_READ_BREAKPOINT, port);
    }
    return value;
}

void port_write_byte(const uint16_t addr, const uint8_t value) {
    uint16_t port = (port_range(addr) << 12) | addr_range(addr);

    if (mem.debug.ports[port] & DBG_PORT_FREEZE) {
        return;
    }

    apb_map[port_range(addr)].range->write_out(addr_range(addr), value);

    if (mem.debug.ports[port] & DBG_PORT_WRITE) {
        debugger(HIT_PORT_WRITE_BREAKPOINT, port);
    }
}

void port_force_write_byte(const uint16_t addr, const uint8_t value) {
    apb_map[port_range(addr)].range->write_out(addr_range(addr), value);
}

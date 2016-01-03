#include "apb.h"
#include "mem.h"
#include "emu.h"
#include "debug/debug.h"
#include <stdio.h>
// Global APB state
apb_map_entry_t apb_map[0x10];

/* The APB (Advanced Peripheral Bus) hosts peripherals that do not require
 * high bandwidth. The bridge to the APB is accessed via addresses 0xE00000-0xFB0000.
 * There is an unmapped APB range from 0xE40000-0xEFFFFF.
 * Each range is 0x1000 bytes long.
 * Reads/Writes can be 8 bits wide. */

void apb_set_map(int entry, eZ80portrange_t *range){
    apb_map[entry].range = range;
}

uint8_t port_read_byte(const uint16_t addr) {
    uint16_t port = (port_range(addr) << 12) | addr_range(addr);
    uint8_t value = apb_map[port_range(addr)].range->read_in(addr_range(addr));

    if (mem.debug.ports[port] & DBG_PORT_READ) {
        debugger(DBG_PORT_READ_BREAKPOINT, port);
    }
    return value;
}

void port_write_byte(const uint16_t addr, const uint8_t value) {
    uint16_t port = (port_range(addr) << 12) | addr_range(addr);

    if (mem.debug.ports[port] & DBG_PORT_FREEZE) {
        printf("%04X -> %02X\n",port,mem.debug.ports[port]);
        return;
    }

    apb_map[port_range(addr)].range->write_out(addr_range(addr), value);

    if (mem.debug.ports[port] & DBG_PORT_WRITE) {
        debugger(DBG_PORT_WRITE_BREAKPOINT, port);
    }
}

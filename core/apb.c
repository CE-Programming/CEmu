#include "apb.h"

#define map_range(addr) (((addr<0xF00000) ? ((addr-0xDF0000)>>16) : ((addr-0xEB0000)>>16))&0xF)
#define map_port(addr) ((addr>>12)&0xF)

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

inline uint8_t mmio_read_byte(const uint32_t addr) {
  return apb_map[map_range(addr)].range->read_in( addr&0xFFF );
}
inline void mmio_write_byte(const uint32_t addr, const uint8_t value) {
  apb_map[map_range(addr)].range->write_out( addr&0xFFF, value);
}

inline uint8_t port_read_byte(const uint16_t addr) {
  return apb_map[map_port(addr)].range->read_in( addr&0xFFF );
}
inline void port_write_byte(const uint16_t addr, const uint8_t value) {
  apb_map[map_port(addr)].range->write_out( addr&0xFFF, value);
}

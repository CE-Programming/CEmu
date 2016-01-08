#ifndef APB_H
#define APB_H

#ifdef __cplusplus
extern "C" {
#endif

#include "defines.h"

typedef struct eZ80portrange {
    uint8_t (*read_in)(const uint16_t);
    void (*write_out)(const uint16_t, const uint8_t);
} eZ80portrange_t;

/* Standard APB entry */
typedef struct apb_map_entry {
    eZ80portrange_t *range;
} apb_map_entry_t;

/* External APB entries */
extern apb_map_entry_t apb_map[0x10];

void apb_set_map(int entry, eZ80portrange_t* range);

uint8_t port_read_byte(const uint16_t addr);
void port_write_byte(const uint16_t addr, const uint8_t value);
void port_force_write_byte(const uint16_t addr, const uint8_t value);

#ifdef __cplusplus
}
#endif

#endif

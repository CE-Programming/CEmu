#ifndef PORT_H
#define PORT_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>

typedef struct eZ80portrange {
    uint8_t (*read)(uint16_t, bool);
    void (*write)(uint16_t, uint8_t, bool);
} eZ80portrange_t;

extern eZ80portrange_t port_map[0x10];

uint8_t port_peek_byte(uint16_t addr);
uint8_t port_read_byte(uint16_t addr);
void port_poke_byte(uint16_t addr, uint8_t value);
void port_write_byte(uint16_t addr, uint8_t value);

#ifdef __cplusplus
}
#endif

#endif

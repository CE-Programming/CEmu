#ifndef BUS_H
#define BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>

uint8_t bus_rand(void);
void bus_init_rand(uint8_t s1, uint8_t s2, uint8_t s3);

#ifdef __cplusplus
}
#endif

#endif

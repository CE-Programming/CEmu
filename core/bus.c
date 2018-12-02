#include "bus.h"
#include <stdint.h>

/* derivation source:
 * https://www.electro-tech-online.com/threads/ultra-fast-pseudorandom-number-generator-for-8-bit.124249/
 */

static uint8_t a, b, c, x = 0;

void bus_init_rand(uint8_t s1, uint8_t s2, uint8_t s3) {
    a = s1;
    b = s2;
    c = s3;
    bus_rand();
}

uint8_t bus_rand(void) {
#ifndef FASTEST_RAND
    x++;
    a ^= c ^ x;
    b += a;
    c = ((c + (b >> 1)) ^ a);
    return c;
#else
    return 5;
#endif
}


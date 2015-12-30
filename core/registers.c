#include "registers.h"

void exx(eZ80registers_t *r) {
    uint32_t temp;
    temp = r->_HL; r->_HL = r->HL; r->HL = temp;
    temp = r->_DE; r->_DE = r->DE; r->DE = temp;
    temp = r->_BC; r->_BC = r->BC; r->BC = temp;
}

int parity(uint8_t x) {
    x ^= x >> 4;
    x ^= x >> 2;
    x ^= x >> 1;
    return x & 1;
}

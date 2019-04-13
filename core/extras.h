#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

// Those aren't related to physical keys - they're keycodes for the OS.
#define CE_KEY_ENTER    0x05
#define CE_KEY_CLEAR    0x09
#define CE_KEY_PRGM     0xDA
#define CE_KEY_ASM      0xFC9C
#define CE_KEY_CLASSIC  0xFBD3

bool sendCSC(uint8_t csc);
bool sendKey(uint16_t key);
bool sendLetterKeyPress(char letter);

#ifdef __cplusplus
}
#endif

#endif

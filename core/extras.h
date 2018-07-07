#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool sendCSC(uint8_t csc);
bool sendKey(uint16_t key);
bool sendLetterKeyPress(char letter);

#ifdef __cplusplus
}
#endif

#endif

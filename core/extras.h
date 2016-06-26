#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdint.h>
#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

void sendKey(uint16_t key);
void sendLetterKeyPress(char letter);

#ifdef __cplusplus
}
#endif

#endif

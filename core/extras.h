#ifndef EXTRAS_H
#define EXTRAS_H

#include <stdint.h>

#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#else
#define EMSCRIPTEN_KEEPALIVE
#endif

#ifdef __cplusplus
extern "C" {
#endif

void EMSCRIPTEN_KEEPALIVE sendKey(uint16_t key);
void EMSCRIPTEN_KEEPALIVE sendLetterKeyPress(char letter);

#ifdef __cplusplus
}
#endif

#endif

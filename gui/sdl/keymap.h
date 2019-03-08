#ifndef KEYMAP_H
#define KEYMAP_H

#include <SDL2/SDL.h>

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    SDL_Keysym keysym;
    int8_t row, col;
} cemu_sdl_key_t;

extern const cemu_sdl_key_t cemu_keymap[];
extern const cemu_sdl_key_t smartpad_keymap[];
extern const int numkeys;

bool keysyms_match(const SDL_Keysym *pattern, const SDL_Keysym *state, bool ignore_mod);

#endif

#include "keymap.h"

const cemu_sdl_key_t cemu_keymap[] = {
    { { SDL_SCANCODE_UNKNOWN, SDLK_F5,        KMOD_NONE,  0 }, 1, 0 }, /* graph */
    { { SDL_SCANCODE_UNKNOWN, SDLK_F4,        KMOD_NONE,  0 }, 1, 1 }, /* trace */
    { { SDL_SCANCODE_UNKNOWN, SDLK_F3,        KMOD_NONE,  0 }, 1, 2 }, /* zoom */
    { { SDL_SCANCODE_UNKNOWN, SDLK_F2,        KMOD_NONE,  0 }, 1, 3 }, /* wind */
    { { SDL_SCANCODE_UNKNOWN, SDLK_F1,        KMOD_NONE,  0 }, 1, 4 }, /* yequ */
    { { SDL_SCANCODE_UNKNOWN, SDLK_SEMICOLON, KMOD_NONE,  0 }, 1, 5 }, /* 2nd */
    { { SDL_SCANCODE_UNKNOWN, SDLK_BACKSPACE, KMOD_NONE,  0 }, 1, 6 }, /* mode */
    { { SDL_SCANCODE_UNKNOWN, SDLK_DELETE,    KMOD_NONE,  0 }, 1, 7 }, /* del */

    { { SDL_SCANCODE_UNKNOWN, SDLK_F12,       KMOD_NONE,  0 }, 2, 0 }, /* on */
    { { SDL_SCANCODE_UNKNOWN, SDLK_PERIOD,    KMOD_SHIFT, 0 }, 2, 1 }, /* sto */
    { { SDL_SCANCODE_UNKNOWN, SDLK_BACKQUOTE, KMOD_NONE,  0 }, 2, 2 }, /* ln */
    { { SDL_SCANCODE_UNKNOWN, SDLK_1,         KMOD_SHIFT, 0 }, 2, 3 }, /* log */
    { { SDL_SCANCODE_UNKNOWN, SDLK_2,         KMOD_SHIFT, 0 }, 2, 4 }, /* sq */
    { { SDL_SCANCODE_UNKNOWN, SDLK_BACKSLASH, KMOD_NONE,  0 }, 2, 5 }, /* inv */
    { { SDL_SCANCODE_UNKNOWN, SDLK_EQUALS,    KMOD_NONE,  0 }, 2, 6 }, /* math */
    { { SDL_SCANCODE_UNKNOWN, SDLK_QUOTE,     KMOD_NONE,  0 }, 2, 7 }, /* alpha */

    { { SDL_SCANCODE_UNKNOWN, SDLK_0,         KMOD_NONE,  0 }, 3, 0 }, /* 0 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_1,         KMOD_NONE,  0 }, 3, 1 }, /* 1 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_4,         KMOD_NONE,  0 }, 3, 2 }, /* 4 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_7,         KMOD_NONE,  0 }, 3, 3 }, /* 7 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_COMMA,     KMOD_NONE,  0 }, 3, 4 }, /* comma */
    { { SDL_SCANCODE_UNKNOWN, SDLK_4,         KMOD_SHIFT, 0 }, 3, 5 }, /* sin */
    { { SDL_SCANCODE_UNKNOWN, SDLK_PAGEUP,    KMOD_NONE,  0 }, 3, 6 }, /* apps */
    { { SDL_SCANCODE_UNKNOWN, SDLK_MINUS,     KMOD_SHIFT, 0 }, 3, 7 }, /* xton */

    { { SDL_SCANCODE_UNKNOWN, SDLK_PERIOD,    KMOD_NONE,  0 }, 4, 0 }, /* dot */
    { { SDL_SCANCODE_UNKNOWN, SDLK_2,         KMOD_NONE,  0 }, 4, 1 }, /* 2 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_5,         KMOD_NONE,  0 }, 4, 2 }, /* 5 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_8,         KMOD_NONE,  0 }, 4, 3 }, /* 8 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_9,         KMOD_SHIFT, 0 }, 4, 4 }, /* lpar */
    { { SDL_SCANCODE_UNKNOWN, SDLK_5,         KMOD_SHIFT, 0 }, 4, 5 }, /* cos */
    { { SDL_SCANCODE_UNKNOWN, SDLK_PAGEDOWN,  KMOD_NONE,  0 }, 4, 6 }, /* prgm */
    { { SDL_SCANCODE_UNKNOWN, SDLK_END,       KMOD_NONE,  0 }, 4, 7 }, /* stat */

    { { SDL_SCANCODE_UNKNOWN, SDLK_BACKQUOTE, KMOD_SHIFT, 0 }, 5, 0 }, /* neg */
    { { SDL_SCANCODE_UNKNOWN, SDLK_3,         KMOD_NONE,  0 }, 5, 1 }, /* 3 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_6,         KMOD_NONE,  0 }, 5, 2 }, /* 6 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_9,         KMOD_NONE,  0 }, 5, 3 }, /* 9 */
    { { SDL_SCANCODE_UNKNOWN, SDLK_0,         KMOD_SHIFT, 0 }, 5, 4 }, /* rpar */
    { { SDL_SCANCODE_UNKNOWN, SDLK_7,         KMOD_SHIFT, 0 }, 5, 5 }, /* tan */
    { { SDL_SCANCODE_UNKNOWN, SDLK_COMMA,     KMOD_SHIFT, 0 }, 5, 6 }, /* vars */

    { { SDL_SCANCODE_UNKNOWN, SDLK_RETURN,    KMOD_NONE,  0 }, 6, 0 }, /* enter */
    { { SDL_SCANCODE_UNKNOWN, SDLK_EQUALS,    KMOD_SHIFT, 0 }, 6, 1 }, /* add */
    { { SDL_SCANCODE_UNKNOWN, SDLK_MINUS,     KMOD_NONE,  0 }, 6, 2 }, /* sub */
    { { SDL_SCANCODE_UNKNOWN, SDLK_8,         KMOD_SHIFT, 0 }, 6, 3 }, /* mul */
    { { SDL_SCANCODE_UNKNOWN, SDLK_SLASH,     KMOD_NONE,  0 }, 6, 4 }, /* div */
    { { SDL_SCANCODE_UNKNOWN, SDLK_6,         KMOD_SHIFT, 0 }, 6, 5 }, /* pow */
    { { SDL_SCANCODE_UNKNOWN, SDLK_ESCAPE,    KMOD_NONE,  0 }, 6, 6 }, /* clr */

    { { SDL_SCANCODE_UNKNOWN, SDLK_DOWN,      KMOD_NONE,  0 }, 7, 0 }, /* down */
    { { SDL_SCANCODE_UNKNOWN, SDLK_LEFT,      KMOD_NONE,  0 }, 7, 1 }, /* left */
    { { SDL_SCANCODE_UNKNOWN, SDLK_RIGHT,     KMOD_NONE,  0 }, 7, 2 }, /* right */
    { { SDL_SCANCODE_UNKNOWN, SDLK_UP,        KMOD_NONE,  0 }, 7, 3 }, /* up */

    { { SDL_SCANCODE_UNKNOWN, SDLK_UNKNOWN,   KMOD_NONE,  0 },-1,-1 }
};

const cemu_sdl_key_t smartpad_keymap[] = {
    { { SDL_SCANCODE_F5,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 0 }, /* graph */
    { { SDL_SCANCODE_T,           SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 1 }, /* trace */
    { { SDL_SCANCODE_F3,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 2 }, /* zoom */
    { { SDL_SCANCODE_F2,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 3 }, /* wind */
    { { SDL_SCANCODE_F1,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 4 }, /* yequ */
    { { SDL_SCANCODE_F6,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 1, 5 }, /* 2nd */
    { { SDL_SCANCODE_F6,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 1, 6 }, /* mode */
    { { SDL_SCANCODE_F4,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 1, 7 }, /* del */

    { { SDL_SCANCODE_F5,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 2, 0 }, /* on */
    { { SDL_SCANCODE_F4,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 2, 1 }, /* sto */
    { { SDL_SCANCODE_F3,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 2, 2 }, /* ln */
    { { SDL_SCANCODE_F2,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 2, 3 }, /* log */
    { { SDL_SCANCODE_F1,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 2, 4 }, /* sq */
    { { SDL_SCANCODE_F9,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 2, 5 }, /* inv */
    { { SDL_SCANCODE_F8,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 2, 6 }, /* math */
    { { SDL_SCANCODE_F7,          SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 2, 7 }, /* alpha */

    { { SDL_SCANCODE_F3,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 3, 0 }, /* 0 */
    { { SDL_SCANCODE_F2,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 3, 1 }, /* 1 */
    { { SDL_SCANCODE_F1,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 3, 2 }, /* 4 */
    { { SDL_SCANCODE_F11,         SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 3, 3 }, /* 7 */
    { { SDL_SCANCODE_F10,         SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 3, 4 }, /* comma */
    { { SDL_SCANCODE_F9,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 3, 5 }, /* sin */
    { { SDL_SCANCODE_F8,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 3, 6 }, /* apps */
    { { SDL_SCANCODE_F7,          SDLK_UNKNOWN, KMOD_LCTRL |               KMOD_LALT | KMOD_LGUI, 0 }, 3, 7 }, /* xton */

    { { SDL_SCANCODE_F1,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 4, 0 }, /* dot */
    { { SDL_SCANCODE_F11,         SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 1 }, /* 2 */
    { { SDL_SCANCODE_F10,         SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 2 }, /* 5 */
    { { SDL_SCANCODE_F9,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 3 }, /* 8 */
    { { SDL_SCANCODE_F8,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 4 }, /* lpar */
    { { SDL_SCANCODE_F7,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 5 }, /* cos */
    { { SDL_SCANCODE_F6,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 6 }, /* prgm */
    { { SDL_SCANCODE_F5,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 4, 7 }, /* stat */

    { { SDL_SCANCODE_F8,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 0 }, /* neg */
    { { SDL_SCANCODE_F7,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 1 }, /* 3 */
    { { SDL_SCANCODE_F6,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 2 }, /* 6 */
    { { SDL_SCANCODE_F5,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 3 }, /* 9 */
    { { SDL_SCANCODE_F4,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 4 }, /* rpar */
    { { SDL_SCANCODE_F3,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 5 }, /* tan */
    { { SDL_SCANCODE_F2,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 5, 6 }, /* vars */

    { { SDL_SCANCODE_KP_ENTER,    SDLK_UNKNOWN, KMOD_NONE,                                        0 }, 6, 0 }, /* enter */
    { { SDL_SCANCODE_KP_PLUS,     SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 6, 1 }, /* add */
    { { SDL_SCANCODE_KP_MINUS,    SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 6, 2 }, /* sub */
    { { SDL_SCANCODE_KP_MULTIPLY, SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 6, 3 }, /* mul */
    { { SDL_SCANCODE_KP_DIVIDE,   SDLK_UNKNOWN, KMOD_LCTRL |                           KMOD_LGUI, 0 }, 6, 4 }, /* div */
    { { SDL_SCANCODE_F11,         SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 6, 5 }, /* pow */
    { { SDL_SCANCODE_F9,          SDLK_UNKNOWN, KMOD_LCTRL | KMOD_LSHIFT |             KMOD_LGUI, 0 }, 6, 6 }, /* clr */

    { { SDL_SCANCODE_DOWN,        SDLK_UNKNOWN,              KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 7, 0 }, /* down */
    { { SDL_SCANCODE_LEFT,        SDLK_UNKNOWN,              KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 7, 1 }, /* left */
    { { SDL_SCANCODE_RIGHT,       SDLK_UNKNOWN,              KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 7, 2 }, /* right */
    { { SDL_SCANCODE_UP,          SDLK_UNKNOWN,              KMOD_LSHIFT | KMOD_LALT | KMOD_LGUI, 0 }, 7, 3 }, /* up */

    { { SDL_SCANCODE_UNKNOWN, SDLK_UNKNOWN,    KMOD_NONE,  0 },-1,-1 }
};

static bool keymods_match(SDL_Keymod pattern, SDL_Keymod state, SDL_Keymod mask) {
    pattern &= mask;
    state &= mask;
    return pattern != mask ? state == pattern : (state & pattern) != KMOD_NONE;
}

bool keysyms_match(const SDL_Keysym *pattern, const SDL_Keysym *state, bool ignore_mod) {
    static const SDL_Keymod masks[] = { KMOD_CTRL, KMOD_SHIFT, KMOD_ALT, KMOD_GUI, KMOD_NONE };
    if (pattern->scancode != SDL_SCANCODE_UNKNOWN && pattern->scancode != state->scancode) {
        return false;
    }
    if (pattern->sym != SDLK_UNKNOWN && pattern->sym != state->sym) {
        return false;
    }
    if (ignore_mod) {
        return true;
    }
    for (const SDL_Keymod *mask = masks; *mask != KMOD_NONE; mask++) {
        if (!keymods_match(pattern->mod, state->mod, *mask)) {
            return false;
        }
    }
    return true;
}

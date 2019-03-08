#include <SDL2/SDL.h>

typedef struct {
    int sdl, row, col;
} cemu_sdl_key_t;

const cemu_sdl_key_t cemu_keymap[] = {
    { SDLK_F5, 1, 0 }, /* graph */
    { SDLK_F4, 1, 1 }, /* trace */
    { SDLK_F3, 1, 2 }, /* zoom */
    { SDLK_F2, 1, 3 }, /* wind */
    { SDLK_F1, 1, 4 }, /* yequ */
    { SDLK_SEMICOLON, 1, 5 }, /* 2nd */
    { SDLK_BACKSPACE, 1, 6 }, /* mode */
    { SDLK_DELETE, 1, 7 }, /* del */

    { SDLK_F12, 2, 0 }, /* on */
    { SDLK_GREATER, 2, 1 }, /* sto */
    { SDLK_BACKQUOTE, 2, 2 }, /* ln */
    { SDLK_EXCLAIM, 2, 3 }, /* log */
    { SDLK_AT, 2, 4 }, /* sq */
    { SDLK_BACKSLASH, 2, 5 }, /* inv */
    { SDLK_EQUALS, 2, 6 }, /* math */
    { SDLK_QUOTE, 2, 7 }, /* alpha */

    { SDLK_0, 3, 0 }, /* 0 */
    { SDLK_1, 3, 1 }, /* 1 */
    { SDLK_4, 3, 2 }, /* 4 */
    { SDLK_7, 3, 3 }, /* 7 */
    { SDLK_COMMA, 3, 4 }, /* comma */
    { SDLK_DOLLAR, 3, 5 }, /* sin */
    { SDLK_PAGEUP, 3, 6 }, /* apps */
    { SDLK_UNDERSCORE, 3, 7 }, /* xton */

    { SDLK_PERIOD, 4, 0 }, /* dot */
    { SDLK_2, 4, 1 }, /* 2 */
    { SDLK_5, 4, 2 }, /* 5 */
    { SDLK_8, 4, 3 }, /* 8 */
    { SDLK_LEFTPAREN, 4, 4 }, /* lpar */
    { SDLK_PERCENT, 4, 5 }, /* cos */
    { SDLK_PAGEDOWN, 4, 6 }, /* prgm */
    { SDLK_END, 4, 7 }, /* stat */

    { SDLK_DOLLAR, 5, 0 }, /* neg */
    { SDLK_3, 5, 1 }, /* 3 */
    { SDLK_6, 5, 2 }, /* 6 */
    { SDLK_9, 5, 3 }, /* 9 */
    { SDLK_RIGHTPAREN, 5, 4 }, /* rpar */
    { SDLK_AMPERSAND, 5, 5 }, /* tan */
    { SDLK_LESS, 5, 6 }, /* vars */

    { SDLK_RETURN, 6, 0 }, /* enter */
    { SDLK_PLUS, 6, 1 }, /* add */
    { SDLK_MINUS, 6, 2 }, /* sub */
    { SDLK_ASTERISK, 6, 3 }, /* mul */
    { SDLK_SLASH, 6, 4 }, /* div */
    { SDLK_CARET, 6, 5 }, /* pow */
    { SDLK_ESCAPE, 6, 6 }, /* clr */

    { SDLK_DOWN, 7, 0 }, /* down */
    { SDLK_LEFT, 7, 1 }, /* left */
    { SDLK_RIGHT, 7, 2 }, /* right */
    { SDLK_UP, 7, 3 }, /* up */
};

const int numkeys = sizeof(cemu_keymap)/sizeof(cemu_sdl_key_t);

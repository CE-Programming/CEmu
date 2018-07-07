#include "extras.h"
#include "emu.h"
#include "mem.h"
#include "defines.h"

/* A few needed locations */
#define CE_kbdScanCode  0xD00587
#define CE_kbdFlags     0xD00080
#define CE_kbdSCR       (1 << 3)
#define CE_kbdKey       0xD0058C
#define CE_keyExtend    0xD0058E
#define CE_graphFlags2  0xD0009F
#define CE_keyReady     (1 << 5)

bool EMSCRIPTEN_KEEPALIVE sendCSC(uint8_t csc) {
    uint8_t flags = mem_peek_byte(CE_kbdFlags);
    if (flags & CE_kbdSCR) {
        return false;
    }
    mem_poke_byte(CE_kbdScanCode, csc);
    mem_poke_byte(CE_kbdFlags, flags | CE_kbdSCR);
    return true;
}

bool EMSCRIPTEN_KEEPALIVE sendKey(uint16_t key) {
    uint8_t flags = mem_peek_byte(CE_graphFlags2);
    if (flags & CE_keyReady) {
        return false;
    }
    if (key < 0x100) {
        key <<= 8;
    }
    mem_poke_byte(CE_kbdKey, (uint8_t)(key >> 8));
    mem_poke_byte(CE_keyExtend, (uint8_t)(key & 0xFF));
    mem_poke_byte(CE_graphFlags2, flags | CE_keyReady);
    return true;
}

bool EMSCRIPTEN_KEEPALIVE sendLetterKeyPress(char letter) {
    uint16_t key;
    if (letter >= '0' && letter <= '9') {
        key = 0x8E + letter - '0';
    } else if (letter >= 'A' && letter <= 'Z') {
        key = 0x9A + letter - 'A';
    } else if (letter == 'Z' + 1 || letter == '@') { /* [ or @ for theta (caller should replace it) */
        key = 0xCC;
    } else {
        return true;
    }
    return sendKey(key);
}

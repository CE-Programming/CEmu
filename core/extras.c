#include "extras.h"
#include "emu.h"

// A few needed locations
#define CE_kbdKey       0xD0058C
#define CE_keyExtend    0xD0058E
void EMSCRIPTEN_KEEPALIVE sendKey(uint16_t key)
{
    mem_poke_byte(CE_kbdKey, (uint8_t)(key & 0xFF));
    mem_poke_byte(CE_keyExtend, (uint8_t)(key >> 8 | (key < 0x100)));
    mem_poke_byte(0xD0009F, (uint8_t)(mem_peek_byte(0xD0009F) | 0x20)); // TODO: name 0xD0009F (= flags+graphFlags2 = 0xD00080+0x1F)
}

void EMSCRIPTEN_KEEPALIVE sendLetterKeyPress(char letter)
{
    uint16_t val;
    if (letter != '@') { // @ for theta (caller should replace it)
        // Handles A-Z and 0-9
        val = (uint16_t)((letter >= 'A' && letter <= 'Z') ? (0x9A + letter - 'A') : (0x8E + letter - '0'));
    } else {
        val = 0xCC; // theta
    }
    sendKey(val);
}

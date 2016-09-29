/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include <string.h>
#include <stdio.h>

#include "sha256.h"
#include "emu.h"

static sha256_state_t sha256;

#define ROR(x, y) ((x) >> (y) | (x) << (32 - (y)))

static void initialize() {
    static const uint32_t initial_state[8] = {
        0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
        0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
    };
    memcpy(sha256.hash_state, initial_state, 32);
}

static void process_block() {
    static const uint32_t k[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    uint32_t a, b, c, d, e, f, g, h;
    uint32_t w[64];
    int i;

    memcpy(w, sha256.hash_block, 64);
    for (i = 16; i < 64; i++) {
        uint32_t s0 = ROR(w[i-15], 7) ^ ROR(w[i-15], 18) ^ (w[i-15] >> 3);
        uint32_t s1 = ROR(w[i-2], 17) ^ ROR(w[i-2], 19) ^ (w[i-2] >> 10);
        w[i] = w[i-16] + s0 + w[i-7] + s1;
    }

    a = sha256.hash_state[0];
    b = sha256.hash_state[1];
    c = sha256.hash_state[2];
    d = sha256.hash_state[3];
    e = sha256.hash_state[4];
    f = sha256.hash_state[5];
    g = sha256.hash_state[6];
    h = sha256.hash_state[7];

    for (i = 0; i < 64; i++) {
        uint32_t s0 = ROR(a, 2) ^ ROR(a, 13) ^ ROR(a, 22);
        uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
        uint32_t t2 = s0 + maj;
        uint32_t s1 = ROR(e, 6) ^ ROR(e, 11) ^ ROR(e, 25);
        uint32_t ch = (e & f) ^ (~e & g);
        uint32_t t1 = h + s1 + ch + k[i] + w[i];

        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    sha256.hash_state[0] += a;
    sha256.hash_state[1] += b;
    sha256.hash_state[2] += c;
    sha256.hash_state[3] += d;
    sha256.hash_state[4] += e;
    sha256.hash_state[5] += f;
    sha256.hash_state[6] += g;
    sha256.hash_state[7] += h;
}

void sha256_reset(void) {
    memset(&sha256, 0, sizeof(sha256));
    gui_console_printf("[CEmu] SHA256 chip reset.\n");
}

static uint8_t sha256_read(uint16_t pio, bool peek) {
    uint16_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;
    static const uint32_t unknown_value = 0x3CA2D5EE; // Unknown function

    if (mem.flash.locked) {
        index = sha256.last_index;
    } else if (!peek) {
        sha256.last_index = index;
    }

    if (index == 0x0C >> 2) {
        return read8(unknown_value, bit_offset);
    } else if (index >= 0x10 >> 2 && index < 0x50 >> 2) {
        return read8(sha256.hash_block[index - (0x10 >> 2)], bit_offset);
    } else if (index >= 0x60 >> 2 && index < 0x80 >> 2) {
        return read8(sha256.hash_state[index - (0x60 >> 2)], bit_offset);
    }
    /* Return 0 if invalid */
    return 0;
}

static void sha256_write(uint16_t pio, uint8_t byte, bool peek) {
    uint16_t index = pio >> 2;
    uint8_t bit_offset = (pio & 3) << 3;

    if (mem.flash.locked) {
        return; // writes are ignored when flash is locked
    } else if (!peek) {
        sha256.last_index = index;
    }

    if (!pio) {
        if (byte & 0x10) {
            memset(sha256.hash_state, 0, sizeof(sha256.hash_state));
        } else {
            if ((byte & 0xE) == 0xA) /* 0A or 0B: first block */
                initialize();
            if ((byte & 0xA) == 0xA) /* 0E or 0F: subsequent blocks */
                process_block();
        }
    } else if (index >= 0x10 >> 2 && index < 0x50 >> 2) {
        write8(sha256.hash_block[index - (0x10 >> 2)], bit_offset, byte);
    }
}

static const eZ80portrange_t device = {
    .read_in    = sha256_read,
    .write_out  = sha256_write
};

eZ80portrange_t init_sha256(void) {
    gui_console_printf("[CEmu] Initialized SHA256 Chip...\n");
    return device;
}

bool sha256_save(emu_image *s) {
    s->sha256 = sha256;
    return true;
}

bool sha256_restore(const emu_image *s) {
    sha256 = s->sha256;
    return true;
}

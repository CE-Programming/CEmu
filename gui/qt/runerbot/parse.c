#include "runerbot.h"

#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint8_t bits[7];
    uint8_t mask[7];
    char mnemonic[20];
    char bytes[5];
    char cycles[30];
    char flags[10];
    char description[290];
} entry_t;
static entry_t entry[475];

extern unsigned char ops_txt[];
extern unsigned int ops_txt_len;

void runerbot_init(void) {
    return; // finish implementing me first...
    unsigned int i = 0;
    unsigned int s = 0;
    entry_t *e;
    bool newline = false;
    char mask[56];

    while (i < ops_txt_len) {
        char x = (char)ops_txt[i];
        if (newline == false && x != '\r' && x != '\n' && x != '\0') {
            if (x == ';') {
                newline = true;
            } else {
                int inc = 0;
                e = &entry[s];
                inc = sscanf((const char*)&ops_txt[i], "%[^\t] %[^\t] %[^\t] %[^\t] %[^\t] %[^\n]",
                    mask, e->mnemonic, e->bytes, e->cycles, e->flags, e->description);
                if (inc > 0) {
                    i += (unsigned int)inc;
                } else {
                    i++;
                }
                newline = true;
                s++;
            }
        } else {
            i++;
        }
        if (x == '\n') {
            newline = false;
        }
    }
}
/* need to implement assembly, but for now just instruction decoding should be fine */
char *runerbot(const char *str) {
    (void)str;
    return NULL;
}

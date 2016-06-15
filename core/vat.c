#include <string.h>
#include <stdio.h>

#include "vat.h"
#include "mem.h"
#include "debug/debug.h"

const char *calc_var_type_names[0x40] = {
    "Real",
    "Real List",
    "Matrix",
    "Equation",
    "String",
    "Program",
    "Prot. Prog.",
    "Picture",
    "Graph Database",
    "Unknown",
    "Unknown Equation",
    "New Equation",
    "Complex",
    "Complex List",
    "Undefined",
    "Window",
    "Recall Window",
    "Table Range",
    "LCD",
    "Backup",
    "Application",
    "AppVar",
    "Temp Program",
    "Group",
    "Real Fraction",
    "Unknown #1",
    "Image",
    "Complex Fraction",
    "Real Radical",
    "Complex Radical",
    "Complex Pi",
    "Complex Pi Fraction",
    "Real Pi",
    "Real Pi Fraction",
    "Unknown #2",
    "Operating System",
    "Flash App",
    "Certificate",
    "Unknown #3",
    "Certificate Memory",
    "Unknown #4",
    "Clock",
    "Unknown #5",
    "Unknown #6",
    "Unknown #7",
    "Unknown #8",
    "Unknown #9",
    "Unknown #10",
    "Unknown #11",
    "Unknown #12",
    "Unknown #13",
    "Unknown #14",
    "Unknown #15",
    "Unknown #16",
    "Unknown #17",
    "Unknown #18",
    "Unknown #19",
    "Unknown #20",
    "Unknown #21",
    "Unknown #22",
    "Unknown #23",
    "Unknown #24",
    "Flash License",
    "Unknown #25",
};

const char *calc_var_name_to_utf8(uint8_t name[8]) {
    static char buffer[17];
    char *dest = buffer;
    uint8_t i;
    for (i = 0; i < 8 && ((name[i] >= 'A' && name[i] <= 'Z' + 1)  ||
                          (i && name[i] >= 'a' && name[i] <= 'z') ||
                          (i && name[i] >= '0' && name[i] <= '9')); i++) {
        if (name[i] == 'Z' + 1) {
            *dest++ = '\xCE';
            *dest++ = '\xB8';
        } else {
            *dest++ = name[i];
        }
    }
    if (!i) {
        switch (name[0]) {
            case '!':
            case '#':
            case '.':
            case '@':
                *dest++ = name[0];
                break;
            case 0x3C:
                *dest++ = 'I';
                *dest++ = 'm';
                *dest++ = 'a';
                *dest++ = 'g';
                *dest++ = 'e';
                *dest++ = '0' + (name[1] + 1) % 10;
                break;
            case 0x5C:
                *dest++ = '[';
                *dest++ = 'A' + name[1];
                *dest++ = ']';
                break;
            case 0x5D:
                *dest++ = 'L';
                *dest++ = '\xE2';
                *dest++ = '\x82';
                *dest++ = '\x81' + name[1];
                break;
            case 0x5E:
                if (name[1] < 0x20) {
                    *dest++ = 'Y';
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x80' + ((name[1] & 0xF) + 1) % 10;
                } else if (name[1] < 0x40) {
                    *dest++ = 'X' + (name[1] & 1);
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x81' + (name[1] >> 1 & 0xF);
                    *dest++ = '\xE1';
                    *dest++ = '\xB4';
                    *dest++ = '\x9B';
                } else if (name[1] < 0x80) {
                    *dest++ = 'r';
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x81' + (name[1] & 0xF);
                } else {
                    *dest++ = 'u' + (name[1] & 3);
                }
                break;
            case 0x60:
                *dest++ = 'P';
                *dest++ = 'i';
                *dest++ = 'c';
                *dest++ = '0' + (name[1] + 1) % 10;
                break;
            case 0x61:
                *dest++ = 'G';
                *dest++ = 'D';
                *dest++ = 'B';
                *dest++ = '0' + (name[1] + 1) % 10;
                break;
            case 0x62:
                if (name[1] == 0x21) {
                    *dest++ = 'n';
                }
                break;
            case 0x72:
                *dest++ = 'A';
                *dest++ = 'n';
                *dest++ = 's';
                break;
            case 0xAA:
                *dest++ = 'S';
                *dest++ = 't';
                *dest++ = 'r';
                *dest++ = '0' + (name[1] + 1) % 10;
                break;
            default:
                for (i = 0; i < 8 && name[i]; i++) {
                    dest += snprintf(dest, 3, "%02hhX", name[i]);
                }
                break;
        }
    }
    *dest = '\0';
    return buffer;
}

void vat_search_init(calc_var_t *var) {
    memset(var, 0, sizeof *var);
    var->vat = 0xD3FFFF;
}

bool vat_search_next(calc_var_t *var) {
    const uint32_t userMem  = 0xD1A881,
                   pTemp    = mem_peek_long(0xD0259A),
                   progPtr  = mem_peek_long(0xD0259D),
                   symTable = 0xD3FFFF;
    uint8_t i;
    bool prog = var->vat <= progPtr;
    if (!var->vat || var->vat < userMem || var->vat <= pTemp || var->vat > symTable) {
        return false; // Some sanity check failed
    }
    var->type1    = mem_peek_byte(var->vat--);
    var->type2    = mem_peek_byte(var->vat--);
    var->version  = mem_peek_byte(var->vat--);
    var->address  = mem_peek_byte(var->vat--);
    var->address |= mem_peek_byte(var->vat--) << 8;
    var->address |= mem_peek_byte(var->vat--) << 16;
    if (prog) {
        var->namelen = mem_peek_byte(var->vat--);
        if (!var->namelen || var->namelen > 8) {
            return false; // Invalid name length
        }
    } else {
        var->namelen = 3;
    }
    var->archived = var->address > 0xC0000 && var->address < 0x400000;
    if (var->archived) {
        var->address += 9 + prog + var->namelen;
    } else if (var->address < 0xD1A881 || var->address >= 0xD40000) {
        return false;
    }
    var->type = (calc_var_type_t)(var->type1 & 0x3F);
    switch (var->type) {
        case CALC_VAR_TYPE_REAL:
            var->size = 9;
            break;
        case CALC_VAR_TYPE_REAL_LIST:
            var->size = 2 + mem_peek_short(var->address) * 9;
            break;
        case CALC_VAR_TYPE_MATRIX:
            var->size = 2 + mem_peek_byte(var->address)
                          * mem_peek_byte(var->address + 1) * 9;
            break;
        case CALC_VAR_TYPE_CPLX:
            var->size = 18;
            break;
        case CALC_VAR_TYPE_CPLX_LIST:
            var->size = 2 + mem_peek_short(var->address) * 18;
            break;
        default:
            var->size = 2 + mem_peek_short(var->address);
            break;
    }
    var->data = phys_mem_ptr(var->address, var->size);
    for (i = 0; i != var->namelen; i++) {
        var->name[i] = mem_peek_byte(var->vat--);
    }
    memset(&var->name[i], 0, sizeof var->name - i);

    return true;
}

bool vat_search_find(const calc_var_t *target, calc_var_t *result) {
    vat_search_init(result);
    while (vat_search_next(result)) {
        if (result->type == target->type &&
            result->namelen == target->namelen &&
            !memcmp(result->name, target->name, target->namelen)) {
            return true;
        }
    }
    return false;
}

bool calc_var_is_prog(const calc_var_t *var) {
    return var && (var->type == CALC_VAR_TYPE_PROG || var->type == CALC_VAR_TYPE_PROT_PROG);
}

bool calc_var_is_asmprog(const calc_var_t *var) {
    return var && (calc_var_is_prog(var) && var->size >= 2 && var->data[2] == 0xEF && var->data[3] == 0x7B);
}

bool calc_var_is_internal(const calc_var_t *var) {
    return var && (!strcmp((const char*)var->name, "#")
                || (var->type == CALC_VAR_TYPE_EQU  && !strcmp((const char*)var->name, "."))
                || (var->type == CALC_VAR_TYPE_PROG && !strcmp((const char*)var->name, "!")));
}

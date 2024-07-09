#include "vat.h"
#include "mem.h"
#include "debug/debug.h"
#include "defines.h"

#include <string.h>

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

static char hex_char(uint8_t nibble) {
    nibble &= 0xF;
    return nibble < 10 ? '0' + nibble : 'A' + nibble - 10;
}
static void hex_byte(char **dest, uint8_t byte) {
    *(*dest)++ = hex_char(byte >> 4);
    *(*dest)++ = hex_char(byte >> 0);
}

const char *calc_var_name_to_utf8(uint8_t name[8], bool named) {
    static char buffer[20];
    char *dest = buffer;
    uint8_t i = 0;
    if (name[0] == 0x5D) {
        *dest++ = '\xCA';
        *dest++ = '\x9F';
        i++;
    }
    for (; i < 8 && ((name[i] >= 'A' && name[i] <= 'Z' + 1)  ||
                     (i && name[i] >= 'a' && name[i] <= 'z') ||
                     (i && name[i] >= '0' && name[i] <= '9')); i++) {
        if (name[i] == 'Z' + 1) {
            *dest++ = '\xCE';
            *dest++ = '\xB8';
        } else {
            *dest++ = name[i];
        }
    }
    if (!(i - (name[0] == 0x5D))) {
        switch (name[0]) {
            case '!':
            case '#':
            case '-':
            case '.':
            case '@':
                *dest++ = name[0];
                break;
            case '$':
                *dest++ = name[0];
                hex_byte(&dest, name[2]);
                hex_byte(&dest, name[1]);
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
                dest -= 2;
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
            case 0xAA:
                *dest++ = 'S';
                *dest++ = 't';
                *dest++ = 'r';
                *dest++ = '0' + (name[1] + 1) % 10;
                break;
            case 0x72:
                if (!named) {
                    *dest++ = 'A';
                    *dest++ = 'n';
                    *dest++ = 's';
                    break;
                }
                fallthrough;
            default:
                for (i = 0; i < 8 && ((name[i] >= 'A' && name[i] <= 'Z' + 1)  ||
                                      (name[i] >= 'a' && name[i] <= 'z') ||
                                      (name[i] >= '0' && name[i] <= '9')); i++) {
                    if (name[i] == 'Z' + 1) {
                        *dest++ = '\xCE';
                        *dest++ = '\xB8';
                    } else {
                        *dest++ = name[i];
                    }
                }
                if (!i) {
                    for (; i < 8 && name[i]; i++) {
                        hex_byte(&dest, name[i]);
                    }
                }
                break;
        }
    }
    *dest = '\0';
    return buffer;
}

static uint8_t calc_tokenized_var_version(const calc_var_t *var) {
    static const uint8_t twoByteTokens[] = {0x5C, 0x5D, 0x5E, 0x60, 0x61, 0x62, 0x63, 0x7E, 0xBB, 0xAA, 0xEF};
    bool usesRTC = false;
    int maxBB = -1;
    int maxEF = -1;
    uint16_t offset = 2;
    while (offset < var->size) {
        uint8_t firstByte = var->data[offset++];
        if (memchr(twoByteTokens, firstByte, sizeof(twoByteTokens)) != NULL) {
            if (offset >= var->size) {
                break;
            }
            uint8_t secondByte = var->data[offset++];
            if (firstByte == 0xBB) {
                if (secondByte > maxBB) maxBB = secondByte;
            } else if (firstByte == 0xEF) {
                if (secondByte <= 0x10) usesRTC = true;
                if (secondByte > maxEF) maxEF = secondByte;
            }
        }
    }
    uint8_t version = usesRTC ? 0x20 : 0x00;
    if (maxBB > 0xF5 || maxEF > 0xA6) {
        version = 0xFF;
    } else if (maxEF > 0x98) {
        version |= 0x0C;
    } else if (maxEF > 0x75) {
        version |= 0x0B;
    } else if (maxEF > 0x40) {
        version |= 0x0A;
    } else if (maxEF > 0x3E) {
        version |= 0x07;
    } else if (maxEF > 0x16) {
        version |= 0x06;
    } else if (maxEF > 0x12) {
        version |= 0x05;
    } else if (maxEF > -1) {
        version |= 0x04;
    } else if (maxBB > 0xDA) {
        version |= 0x03;
    } else if (maxBB > 0xCE) {
        version |= 0x02;
    } else if (maxBB > 0x67) {
        version |= 0x01;
    }
    return version;
}

void vat_search_init(calc_var_t *var) {
    memset(var, 0, sizeof *var);
    var->vat = 0xD3FFFF;
}

bool vat_search_next(calc_var_t *var) {
    const uint32_t userMem  = 0xD1A881,
                   OPBase   = mem_peek_long(0xD02590),
                   pTemp    = mem_peek_long(0xD0259A),
                   progPtr  = mem_peek_long(0xD0259D),
                   symTable = 0xD3FFFF;
    uint8_t i;
    if (!var->vat || var->vat < userMem || var->vat <= OPBase || var->vat > symTable) {
        return false; /* some sanity check failed */
    }
    var->type1    = mem_peek_byte(var->vat--);
    var->type2    = mem_peek_byte(var->vat--);
    var->version  = mem_peek_byte(var->vat--);
    var->address  = mem_peek_byte(var->vat--);
    var->address |= mem_peek_byte(var->vat--) << 8;
    var->address |= mem_peek_byte(var->vat--) << 16;
    if ((var->named = var->vat > pTemp && var->vat <= progPtr)) {
        var->namelen = mem_peek_byte(var->vat--);
        if (!var->namelen || var->namelen > 8) {
            return false; /* invalid name length */
        }
    } else {
        var->namelen = 3;
    }
    var->archived = var->address > 0xC0000 && var->address < 0x400000;
    if (var->archived) {
        var->address += 9 + var->named + var->namelen;
    } else if (var->address < 0xD1A881 || var->address >= 0xD40000) {
        return false;
    }
    var->type = (calc_var_type_t)(var->type1 & 0x3F);
    switch (var->type) {
        case CALC_VAR_TYPE_REAL:
        case CALC_VAR_TYPE_REAL_FRAC:
        case CALC_VAR_TYPE_MIXED_FRAC:
        case CALC_VAR_TYPE_REAL_RADICAL:
        case CALC_VAR_TYPE_REAL_PI:
        case CALC_VAR_TYPE_REAL_PI_FRAC:
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
        case CALC_VAR_TYPE_CPLX_FRAC:
        case CALC_VAR_TYPE_CPLX_RADICAL:
        case CALC_VAR_TYPE_CPLX_PI:
        case CALC_VAR_TYPE_CPLX_PI_FRAC:
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
    if (var->data == NULL) {
        return false;
    }
    for (i = 0; i != var->namelen; i++) {
        var->name[i] = mem_peek_byte(var->vat--);
    }
    memset(&var->name[i], 0, sizeof var->name - i);

    if (!var->archived) {
        switch (var->type) {
        case CALC_VAR_TYPE_REAL:
        case CALC_VAR_TYPE_REAL_FRAC:
        case CALC_VAR_TYPE_MIXED_FRAC:
            var->type = CALC_VAR_TYPE_REAL;
            i = var->data[0] & 0x3F;
            if (i == CALC_VAR_TYPE_REAL) {
                var->version = 0x00;
            } else {
                var->version = 0x06;
            }
            break;

        case CALC_VAR_TYPE_CPLX:
        case CALC_VAR_TYPE_CPLX_FRAC:
        case CALC_VAR_TYPE_CPLX_RADICAL:
        case CALC_VAR_TYPE_CPLX_PI:
        case CALC_VAR_TYPE_CPLX_PI_FRAC:
            i = var->data[0] & 0x3F;
            if (i == CALC_VAR_TYPE_CPLX) {
                var->version = 0x00;
            } else if (i == CALC_VAR_TYPE_CPLX_FRAC) {
                var->version = 0x0B;
            } else {
                var->version = 0x10;
            }
            break;

        case CALC_VAR_TYPE_REAL_LIST:
        case CALC_VAR_TYPE_MATRIX:
        case CALC_VAR_TYPE_CPLX_LIST:
            var->version = 0x00;
            for (uint16_t offset = 2; offset < var->size; offset += 9) {
                i = var->data[offset] & 0x3F;
                if (i > CALC_VAR_TYPE_CPLX_FRAC) {
                    var->version = 0x10;
                    break;
                } else if (i == CALC_VAR_TYPE_CPLX_FRAC) {
                    if (var->version < 0x0B) var->version = 0x0B;
                } else if (i == CALC_VAR_TYPE_REAL_FRAC || i == CALC_VAR_TYPE_MIXED_FRAC) {
                    if (var->version < 0x06) var->version = 0x06;
                }
            }
            break;

        case CALC_VAR_TYPE_EQU:
        case CALC_VAR_TYPE_STRING:
        case CALC_VAR_TYPE_NEW_EQU:
            var->version = calc_tokenized_var_version(var);
            break;

        default:
            if (var->type > CALC_VAR_TYPE_CPLX_FRAC) {
                var->version = 0x10;
            }
            break;
        }
    }

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

bool calc_var_is_tokenized(const calc_var_t *var) {
    return calc_var_is_prog(var) || var->type == CALC_VAR_TYPE_EQU || var->type == CALC_VAR_TYPE_STRING;
}

bool calc_var_is_python_appvar(const calc_var_t *var) {
    return var && var->type == CALC_VAR_TYPE_APP_VAR
               && var->size > 6
               && memcmp("PYCD", var->data + 2, 4) == 0;
}


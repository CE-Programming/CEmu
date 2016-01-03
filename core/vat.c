#include "vat.h"
#include "mem.h"

#include <string.h>
#include <stdio.h>

const char *calc_var_type_names[0x20] = {
    "Real",
    "List",
    "Matrix",
    "Equation",
    "String",
    "Program",
    "Protected Program",
    "Picture",
    "Graph Database",
    "Unknown",
    "Unknown Equation",
    "New Equation",
    "Complex",
    "Complex List",
    "Undefined",
    "Zoom Store",
    "Table Range",
    "LCD",
    "Backup",
    "Application",
    "Application Variable",
    "Temp Program",
    "Group",
    "Unknown #1",
    "Unknown #2",
    "Unknown #3",
    "Image",
    "Unknown #5",
    "Unknown #6",
    "Unknown #7",
    "Unknown #8",
    "Unknown #9",
};

const char *calc_var_name_to_utf8(uint8_t name[8]) {
    char buffer[17], *dest = buffer;
    uint8_t i;
    for (i = 0; i < 8 && name[i] >= 'A' && name[i] <= 'Z' + 1; i++) {
        if (name[i] <= 'Z') {
            *dest++ = name[i];
        } else {
            *dest++ = '\xCE';
            *dest++ = '\xB8';
        }
    }
    if (!i) {
        switch (name[0]) {
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
                *dest++ = '\x80' + name[1];
                break;
            case 0x5E:
                if (name[1] < 0x20) {
                    *dest++ = 'Y';
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x80' + (name[1] & 0xF);
                } else if (name[1] < 0x40) {
                    *dest++ = 'X' + (name[1] & 1);
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x80' + (name[1] >> 1 & 0xF);
                } else if (name[1] < 0x80) {
                    *dest++ = 'r';
                    *dest++ = '\xE2';
                    *dest++ = '\x82';
                    *dest++ = '\x80' + (name[1] & 0xF);
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
    return strdup(buffer);
}

void vat_search_init(calc_var_t *var) {
    memset(var, 0, sizeof *var);
    var->vat = phys_mem_ptr(0xD3FFFF, 1);
}

bool vat_search_next(calc_var_t *var) {
    const uint8_t *userMem  = phys_mem_ptr(0xD1A881, 1),
                  *pTemp    = phys_mem_ptr(*(uint32_t *)phys_mem_ptr(0xD0259A, 4) & 0xFFFFFF, 1),
                  *progPtr  = phys_mem_ptr(*(uint32_t *)phys_mem_ptr(0xD0259D, 4) & 0xFFFFFF, 1),
                  *symTable = phys_mem_ptr(0xD3FFFF, 1);
    uint32_t address;
    uint8_t i, *name = var->name;
    bool prog = var->vat <= progPtr;
    if (!var->vat || var->vat < userMem || var->vat <= pTemp || var->vat > symTable) {
        return false;
    }
    var->type1   = *var->vat--;
    var->type2   = *var->vat--;
    var->version = *var->vat--;
    address      = *var->vat--;
    address     |= *var->vat-- << 8;
    address     |= *var->vat-- << 16;
    if (prog) {
        var->namelen = *var->vat--;
        if (!var->namelen || var->namelen > 8) {
            return false;
        }
    } else {
        var->namelen = 3;
    }
    if (address > 0xC0000 && address < 0x400000) {
        address += 9 + prog + var->namelen;
    } else if (!(address >= 0xD1A881 && address < 0xD40000)) {
        return false;
    }
    var->data = phys_mem_ptr(address, 2);
    if (!var->data) {
        return false;
    }
    switch (var->type = var->type1 & 0x1F) {
        case CALC_VAR_TYPE_REAL:
            var->size  = 9;
            break;
        case CALC_VAR_TYPE_LIST:
            var->size  = *var->data++;
            var->size |= *var->data++ << 8;
            var->size *= 9;
            break;
        case CALC_VAR_TYPE_MATRIX:
            var->size  = *var->data++;
            var->size *= *var->data++;
            var->size *= 9;
            break;
        case CALC_VAR_TYPE_CPLX:
            var->size  = 18;
            break;
        case CALC_VAR_TYPE_CPLX_LIST:
            var->size  = *var->data++;
            var->size |= *var->data++ << 8;
            var->size *= 18;
            break;
        default:
            var->size  = *var->data++;
            var->size |= *var->data++ << 8;
            break;
    }
    for (i = var->namelen; i; i--) {
        *name++ = *var->vat--;
    }
    return true;
}

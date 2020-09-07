#include "device.h"

#include "../defines.h"
#include "../emu.h"
#include "../vat.h"

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define DUSB_ATTR_HEADER_SIZE               4

#define DUSB_ATTR_VAR_SIZE                  0x0001
#define DUSB_ATTR_VAR_TYPE                  0x0002
#define DUSB_ATTR_VAR_TYPE_SIZE             4
#define DUSB_ATTR_ARCHIVED                  0x0003
#define DUSB_ATTR_ARCHIVED_SIZE             1
#define DUSB_ATTR_VAR_VERSION               0x0008
#define DUSB_ATTR_VAR_VERSION_SIZE          4


#define DUSB_RPKT_HEADER_SIZE               5

#define DUSB_RPKT_BUF_SIZE_REQ              1
#define DUSB_RPKT_BUF_SIZE_REQ_SIZE         4
#define DUSB_RPKT_BUF_SIZE_ALLOC            2
#define DUSB_RPKT_BUF_SIZE_ALLOC_SIZE       4

#define DUSB_RPKT_VIRT_DATA                 3
#define DUSB_RPKT_VIRT_DATA_LAST            4
#define DUSB_RPKT_VIRT_DATA_ACK             5
#define DUSB_RPKT_VIRT_DATA_ACK_SIZE        2


#define DUSB_VPKT_HEADER_SIZE               6

#define DUSB_VPKT_PING                      0x0001
#define DUSB_VPKT_PING_SIZE                 10
#define DUSB_VPKT_OS_BEGIN                  0x0002
#define DUSB_VPKT_OS_BEGIN_SIZE             11
#define DUSB_VPKT_OS_ACK                    0x0003
#define DUSB_VPKT_OS_ACK_SIZE               8
#define DUSB_VPKT_OS_HEADER                 0x0004
#define DUSB_VPKT_OS_HEADER_SIZE            0x100
#define DUSB_VPKT_OS_DATA                   0x0005
#define DUSB_VPKT_OS_DATA_HEADER_SIZE       4
#define DUSB_VPKT_EOT_ACK                   0x0006
#define DUSB_VPKT_EOT_ACK_SIZE              5
#define DUSB_VPKT_PARM_REQ                  0x0007
#define DUSB_VPKT_PARM_DATA                 0x0008
#define DUSB_VPKT_DIR_REQ                   0x0009
#define DUSB_VPKT_VAR_HDR                   0x000A
#define DUSB_VPKT_RTS                       0x000B
#define DUSB_VPKT_RTS_SIZE                  10
#define DUSB_VPKT_RTS_SILENT                1
#define DUSB_VPKT_RTS_NON_SILENT            2
#define DUSB_VPKT_VAR_REQ                   0x000C
#define DUSB_VPKT_VAR_CNTS                  0x000D
#define DUSB_VPKT_PARM_SET                  0x000E
#define DUSB_VPKT_MODIF_VAR                 0x0010
#define DUSB_VPKT_EXECUTE                   0x0011
#define DUSB_VPKT_MODE_SET                  0x0012
#define DUSB_VPKT_MODE_SET_SIZE             4

#define DUSB_VPKT_DATA_ACK                  0xAA00
#define DUSB_VPKT_DATA_ACK_SIZE             1
#define DUSB_VPKT_DELAY_ACK                 0xBB00
#define DUSB_VPKT_DELAY_ACK_SIZE            4
#define DUSB_VPKT_EOT                       0xDD00
#define DUSB_VPKT_EOT_SIZE                  0
#define DUSB_VPKT_ERROR                     0xEE00


#define DUSB_CONTROL_ENDPOINT               0
#define DUSB_IN_ENDPOINT                    1
#define DUSB_OUT_ENDPOINT                   2
#define DUSB_IN_DIRECTION                   0x80
#define DUSB_OUT_DIRECTION                  0x00


#define DUSB_FILE_MAGIC_SIZE                8

#define DUSB_FLASH_FILE_NAME_OFFSET         16
#define DUSB_FLASH_FILE_DEVICE_TYPE_OFFSET  48
#define DUSB_FLASH_FILE_DATA_TYPE_OFFSET    49
#define DUSB_FLASH_FILE_LENGTH_OFFSET       74
#define DUSB_FLASH_FILE_DATA_OFFSET         78

#define DUSB_VAR_FILE_DATA_LENGTH_OFFSET    53
#define DUSB_VAR_FILE_DATA_OFFSET           55
#define DUSB_VAR_FILE_VAR_VERSION_OFFSET    11
#define DUSB_VAR_FILE_VAR_FLAG_OFFSET       12
#define DUSB_VAR_FILE_VAR_FLAG_ARCHIVED     0x80
#define DUSB_VAR_FILE_VAR_HEADER_SIZE       13
#define DUSB_VAR_FILE_CHECKSUM_SIZE         2

#define DUSB_OS_BASE_ADDR                   0x30000


typedef enum dusb_command_type {
    DUSB_DONE_COMMAND,
    DUSB_SCREENSHOT_COMMAND,
    DUSB_ROMDUMP_COMMAND,
    DUSB_LIST_COMMAND,
    DUSB_SEND_COMMAND,
    DUSB_RECEIVE_COMMAND,
    DUSB_NUM_COMMANDS,
} dusb_command_type_t;

static const char *command_names[DUSB_NUM_COMMANDS] = {
    "done", "screenshot", "romdump", "list", "send", "receive"
};

typedef struct dusb_command {
    FILE *file;
    dusb_command_type_t type;
    calc_var_type_t vartype;
    uint8_t varname_length, varname[8], varname_utf8_length, varname_utf8[8 * 3];
} dusb_command_t;

typedef enum dusb_state {
    DUSB_INIT_STATE,
    DUSB_RESET_STATE,
    DUSB_RESET_RECOVERY_STATE,
    DUSB_SET_ADDRESS_STATE,
    DUSB_SET_CONFIG_STATE,
    DUSB_BUF_SIZE_REQ_STATE,
    DUSB_BUF_SIZE_ALLOC_STATE,
    DUSB_COMMAND_STATE,
    DUSB_NEXT_COMMAND_STATE,

    DUSB_SOS_PING_STATE,
    DUSB_SOS_PING_ACK_STATE,
    DUSB_SOS_MODE_SET_STATE,
    DUSB_SOS_MODE_SET_ACK_STATE,
    DUSB_SOS_BEGIN_STATE,
    DUSB_SOS_BEGIN_ACK_STATE,
    DUSB_SOS_ACK_BEGIN_STATE,
    DUSB_SOS_ACK_BEGIN_ACK_STATE,
    DUSB_SOS_HEADER_STATE,
    DUSB_SOS_HEADER_ACK_STATE,
    DUSB_SOS_ACK_HEADER_STATE,
    DUSB_SOS_ACK_HEADER_ACK_STATE,
    DUSB_SOS_DATA_STATE,
    DUSB_SOS_DATA_ACK_STATE,
    DUSB_SOS_ACK_DATA_STATE,
    DUSB_SOS_ACK_DATA_ACK_STATE,
    DUSB_SOS_NEXT_DATA_STATE,
    DUSB_SOS_EOT_STATE,
    DUSB_SOS_EOT_ACK_STATE,
    DUSB_SOS_ACK_EOT_STATE,
    DUSB_SOS_ACK_EOT_ACK_STATE,
    DUSB_SOS_DONE_STATE,

    DUSB_SEND_PING_STATE,
    DUSB_SEND_PING_ACK_STATE,
    DUSB_SEND_MODE_SET_STATE,
    DUSB_SEND_MODE_SET_ACK_STATE,
    DUSB_SEND_RTS_STATE,
    DUSB_SEND_RTS_ACK_STATE,
    DUSB_SEND_ACK_RTS_STATE,
    DUSB_SEND_ACK_RTS_ACK_STATE,
    DUSB_SEND_VAR_CNTS_STATE,
    DUSB_SEND_VAR_CNTS_ACK_STATE,
    DUSB_SEND_NEXT_VAR_CNTS_STATE,
    DUSB_SEND_ACK_VAR_CNTS_STATE,
    DUSB_SEND_ACK_VAR_CNTS_ACK_STATE,
    DUSB_SEND_EOT_STATE,
    DUSB_SEND_EOT_ACK_STATE,
    DUSB_SEND_NEXT_STATE,

    DUSB_INVALID_STATE,
} dusb_state_t;

typedef struct dusb_context {
    dusb_state_t state;
    uint32_t start, position, offset, length, max_rpkt_size, max_vpkt_size;
    uint8_t version, flag, buffer[8];
    dusb_command_t *command, commands[];
} dusb_context_t;

static uint32_t min32(uint32_t x, uint32_t y) {
    return x < y ? x : y;
}

static bool dusb_convert_varname_to_utf8(dusb_command_t *command) {
    static const uint8_t tiascii_to_utf8[0x100][4] = {
        u8"\x0000", u8"\uF00E", u8"\x0000", u8"\x0000", u8"\x0000", u8"\uF014", u8"\x0000", u8"\uF016",
        u8"\u222B", u8"\u00D7", u8"\u25AB", u8"\u207A", u8"\u2022", u8"\uF038", u8"\u00B3", u8"\uF02E",
        u8"\u221A", u8"\uF005", u8"\u00B2", u8"\u2220", u8"\u00B0", u8"\uF001", u8"\uF002", u8"\u2264",
        u8"\u2260", u8"\u2265", u8"\u00AF", u8"\uF000", u8"\u2192", u8"\uF01D", u8"\u2191", u8"\u2193",
        u8"\x0020", u8"\x0021", u8"\x0022", u8"\x0023", u8"\x0024", u8"\x0025", u8"\x0026", u8"\x0027",
        u8"\x0028", u8"\x0029", u8"\x002A", u8"\x002B", u8"\x002C", u8"\x002D", u8"\x002E", u8"\x002F",
        u8"\x0030", u8"\x0031", u8"\x0032", u8"\x0033", u8"\x0034", u8"\x0035", u8"\x0036", u8"\x0037",
        u8"\x0038", u8"\x0039", u8"\x003A", u8"\x003B", u8"\x003C", u8"\x003D", u8"\x003E", u8"\x003F",
        u8"\x0040", u8"\x0041", u8"\x0042", u8"\x0043", u8"\x0044", u8"\x0045", u8"\x0046", u8"\x0047",
        u8"\x0048", u8"\x0049", u8"\x004A", u8"\x004B", u8"\x004C", u8"\x004D", u8"\x004E", u8"\x004F",
        u8"\x0050", u8"\x0051", u8"\x0052", u8"\x0053", u8"\x0054", u8"\x0055", u8"\x0056", u8"\x0057",
        u8"\x0058", u8"\x0059", u8"\x005A", u8"\u03B8", u8"\x005C", u8"\x005D", u8"\x005E", u8"\x005F",
        u8"\x0060", u8"\x0061", u8"\x0062", u8"\x0063", u8"\x0064", u8"\x0065", u8"\x0066", u8"\x0067",
        u8"\x0068", u8"\x0069", u8"\x006A", u8"\x006B", u8"\x006C", u8"\x006D", u8"\x006E", u8"\x006F",
        u8"\x0070", u8"\x0071", u8"\x0072", u8"\x0073", u8"\x0074", u8"\x0075", u8"\x0076", u8"\x0077",
        u8"\x0078", u8"\x0079", u8"\x007A", u8"\x007B", u8"\x007C", u8"\x007D", u8"\x007E", u8"\x0000",
        u8"\u2080", u8"\u2081", u8"\u2082", u8"\u2083", u8"\u2084", u8"\u2085", u8"\u2086", u8"\u2087",
        u8"\u2088", u8"\u2089", u8"\u00C1", u8"\u00C0", u8"\u00C2", u8"\u00C4", u8"\u00E1", u8"\u00E0",
        u8"\u00E2", u8"\u00E4", u8"\u00C9", u8"\u00C8", u8"\u00CA", u8"\u00CB", u8"\u00E9", u8"\u00E8",
        u8"\u00EA", u8"\u00EB", u8"\u00CD", u8"\u00CC", u8"\u00CE", u8"\u00CF", u8"\u00ED", u8"\u00EC",
        u8"\u00EE", u8"\u00EF", u8"\u00D3", u8"\u00D2", u8"\u00D4", u8"\u00D6", u8"\u00F3", u8"\u00F2",
        u8"\u00F4", u8"\u00F6", u8"\u00DA", u8"\u00D9", u8"\u00DB", u8"\u00DC", u8"\u00FA", u8"\u00F9",
        u8"\u00FB", u8"\u00FC", u8"\u00C7", u8"\u00E7", u8"\u00D1", u8"\u00F1", u8"\u00B4", u8"\x0000",
        u8"\u00A8", u8"\u00BF", u8"\u00A1", u8"\u03B1", u8"\u03B2", u8"\u03B3", u8"\u0394", u8"\u03B4",
        u8"\u03B5", u8"\x005B", u8"\u03BB", u8"\u00B5", u8"\u03C0", u8"\u03C1", u8"\u03A3", u8"\u03C3",
        u8"\u03C4", u8"\u03C6", u8"\u03A9", u8"\uF003", u8"\uF004", u8"\uF006", u8"\u2026", u8"\uF00B",
        u8"\x0000", u8"\uF00A", u8"\x0000", u8"\u00B2", u8"\u00B0", u8"\u00B3", u8"\x000A", u8"\uF02F",
        u8"\uF022", u8"\u03C7", u8"\uF021", u8"\u212F", u8"\u230A", u8"\x0000", u8"\x0000", u8"\x0000",
        u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000",
        u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000",
        u8"\x0000", u8"\x0000", u8"\u2074", u8"\uF015", u8"\u00DF", u8"\x0000", u8"\x0000", u8"\x0000",
        u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000", u8"\x0000",
    };

    char tiascii[9];
    if (command->varname_length > sizeof command->varname) {
        return false;
    }
    switch (command->vartype) {
        case CALC_VAR_TYPE_REAL:
        case CALC_VAR_TYPE_CPLX:
            if (command->varname_length != 1) {
                return false;
            }
            /* fallthrough */
        default:
            if (command->varname[0] < 'A' || command->varname[0] > 'Z' + 1) {
                return false;
            }
            memcpy(tiascii, command->varname, command->varname_length);
            tiascii[command->varname_length] = '\0';
            break;
        case CALC_VAR_TYPE_REAL_LIST:
        case CALC_VAR_TYPE_CPLX_LIST:
            if (command->varname[0] != 0x5D) {
                return false;
            }
            if (command->varname_length <= 2 && command->varname[1] < 6) {
                strcpy(tiascii, "L\x81");
                tiascii[1] += command->varname[1];
            } else if (command->varname_length == 2 && command->varname[1] == 0x40) {
                strcpy(tiascii, "IDList");
            } else if (command->varname_length <= 6 &&
                       command->varname[1] >= 'A' && command->varname[1] <= 'Z' + 1) {
                memcpy(tiascii, command->varname + 1, command->varname_length - 1);
                tiascii[command->varname_length - 1] = '\0';
            } else {
                return false;
            }
            break;
        case CALC_VAR_TYPE_MATRIX:
            if (command->varname_length > 2 ||
                command->varname[0] != 0x5C || command->varname[1] > 9) {
                return false;
            }
            strcpy(tiascii, "\xC1" "A]");
            tiascii[1] += command->varname[1];
            break;
        case CALC_VAR_TYPE_EQU:
            if (command->varname_length != 2 || command->varname[0] != 0x5E) {
                return false;
            }
            if (command->varname[1] >= 0x10 && command->varname[1] <= 0x19) {
                strcpy(tiascii, "Y\x80");
                if (command->varname[1] != 0x19) {
                    tiascii[1] += 1 + command->varname[1] - 0x10;
                }
            } else if (command->varname[1] >= 0x20 && command->varname[1] <= 0x2B) {
                strcpy(tiascii, "X\x81\x0D");
                tiascii[0] += (command->varname[1] & 1);
                tiascii[1] += ((command->varname[1] - 0x20) >> 1);
            } else if (command->varname[1] >= 0x40 && command->varname[1] <= 0x45) {
                strcpy(tiascii, "r\x81");
                tiascii[1] += command->varname[1] - 0x40;
            } else if (command->varname[1] >= 0x80 && command->varname[1] <= 0x82) {
                strcpy(tiascii, "u");
                tiascii[0] += command->varname[1] - 0x80;
            } else {
                return false;
            }
            break;
        case CALC_VAR_TYPE_STRING:
            if (command->varname_length > 2 ||
                command->varname[0] != 0xAA || command->varname[1] > 9) {
                return false;
            }
            strcpy(tiascii, "Str0");
            if (command->varname[1] != 9) {
                tiascii[3] += 1 + command->varname[1];
            }
            break;
        case CALC_VAR_TYPE_PICTURE:
            if (command->varname_length > 2 ||
                command->varname[0] != 0x60 || command->varname[1] > 9) {
                return false;
            }
            strcpy(tiascii, "Pic0");
            if (command->varname[1] != 9) {
                tiascii[3] += 1 + command->varname[1];
            }
            break;
        case CALC_VAR_TYPE_GDB:
            if (command->varname_length > 2 ||
                command->varname[0] != 0x61 || command->varname[1] > 9) {
                return false;
            }
            strcpy(tiascii, "GDB0");
            if (command->varname[1] != 9) {
                tiascii[3] += 1 + command->varname[1];
            }
            break;
        case CALC_VAR_TYPE_WINDOW:
            strcpy(tiascii, "Window");
            break;
        case CALC_VAR_TYPE_RCL_WINDOW:
            strcpy(tiascii, "RclWin");
            break;
        case CALC_VAR_TYPE_TABLE_RANGE:
            strcpy(tiascii, "TblSet");
            break;
        case CALC_VAR_TYPE_IMAGE:
            if (command->varname_length > 2 ||
                command->varname[0] != 0x3C || command->varname[1] > 9) {
                return false;
            }
            strcpy(tiascii, "Image0");
            if (command->varname[1] != 9) {
                tiascii[5] += 1 + command->varname[1];
            }
            break;
    }

    command->varname_utf8_length = 0;
    for (uint8_t i = 0, j, c; tiascii[i]; ++i) {
        for (j = 0; (c = tiascii_to_utf8[(uint8_t)tiascii[i]][j]); ++j) {
            command->varname_utf8[command->varname_utf8_length++] = c;
        }
        if (!j) {
            return false;
        }
    }
    return true;
}

static bool dusb_verify_checksum(FILE *file) {
    uint16_t sum = 0, checksum = 0;
    uint8_t buffer[0x1000];
    while (!feof(file)) {
        if (ferror(file)) {
            return false;
        }
        size_t bytes = fread(buffer, 1, sizeof buffer, file);
        if (bytes == 1) {
            checksum >>= 8;
            checksum |= buffer[0] << 8;
        } else if (bytes > 1) {
            checksum = buffer[bytes - 2] << 0 | buffer[bytes - 1] << 8;
        }
        while (bytes--) {
            sum += buffer[bytes];
        }
    }
    sum -= checksum >> 8 & 0xFF;
    sum -= checksum >> 0 & 0xFF;
    return sum == checksum;
}

static dusb_state_t dusb_detect(dusb_context_t *context) {
    dusb_command_t *command = context->command;
    FILE *file = command->file;
    char magic[DUSB_FILE_MAGIC_SIZE];
    if (fseek(file, 0, SEEK_SET) ||
        fread(magic, sizeof magic, 1, file) != 1) {
        return DUSB_INVALID_STATE;
    }
    if (!memcmp("**TIFL**", magic, sizeof magic)) {
        if (fseek(file, DUSB_FLASH_FILE_NAME_OFFSET, SEEK_SET) ||
            fread(&command->varname_length, sizeof command->varname_length, 1, file) != 1 ||
            fread(command->varname, command->varname_length, 1, file) != 1 ||
            !dusb_convert_varname_to_utf8(command) ||
            fseek(file, DUSB_FLASH_FILE_DEVICE_TYPE_OFFSET, SEEK_SET) ||
            fread(&context->version, sizeof context->version, 1, file) != 1 || context->version != 0x73 ||
            fread(&command->vartype, sizeof command->vartype, 1, file) != 1 ||
            fseek(file, DUSB_FLASH_FILE_LENGTH_OFFSET, SEEK_SET) ||
            fread(&context->length, sizeof context->length, 1, file) != 1 ||
            fseek(file, 0, SEEK_END) || DUSB_FLASH_FILE_DATA_OFFSET + context->length != ftell(file)) {
            return DUSB_INVALID_STATE;
        }
        context->start = DUSB_FLASH_FILE_DATA_OFFSET;
        switch (command->vartype) {
            case CALC_VAR_TYPE_OPERATING_SYSTEM:
                return DUSB_SOS_PING_STATE;
            case CALC_VAR_TYPE_FLASH_APP:
                context->flag = DUSB_VAR_FILE_VAR_FLAG_ARCHIVED;
                return DUSB_SEND_PING_STATE;
            default:
                return DUSB_INVALID_STATE;
        }
    }
    if (!memcmp("**TI83F*", magic, sizeof magic)) {
        uint16_t data_length;
        if (fread(magic, 3, 1, file) != 1 ||
            magic[0] != 0x1A || magic[1] != 0x0A || magic[2] != 0x00 ||
            fseek(file, DUSB_VAR_FILE_DATA_LENGTH_OFFSET, SEEK_SET) ||
            fread(&data_length, sizeof data_length, 1, file) != 1 ||
            !dusb_verify_checksum(file) || DUSB_VAR_FILE_DATA_OFFSET +
            data_length + DUSB_VAR_FILE_CHECKSUM_SIZE != ftell(file) ||
            fseek(file, DUSB_VAR_FILE_DATA_OFFSET, SEEK_SET)) {
            return DUSB_INVALID_STATE;
        }
        context->start = DUSB_VAR_FILE_DATA_OFFSET;
        return DUSB_SEND_NEXT_STATE;
    }
    return DUSB_INVALID_STATE;
}

static bool dusb_detect_var(dusb_context_t *context) {
    dusb_command_t *command = context->command;
    FILE *file = command->file;
    uint16_t header_size, data_size;
    context->length = context->version = context->flag = 0;
    return !fseek(file, context->start, SEEK_SET) &&
        fread(&header_size, sizeof header_size, 1, file) == 1 &&
        header_size >= DUSB_VAR_FILE_VAR_VERSION_OFFSET &&
        fread(&data_size, sizeof data_size, 1, file) == 1 &&
        fread(&command->vartype, sizeof command->vartype, 1, file) == 1 &&
        fread(&command->varname, sizeof command->varname, 1, file) == 1 &&
        (command->varname_length = strnlen((const char *)command->varname, sizeof command->varname),
         dusb_convert_varname_to_utf8(command)) &&
        (header_size <= DUSB_VAR_FILE_VAR_VERSION_OFFSET ||
         fread(&context->version, sizeof context->version, 1, file) == 1) &&
        (header_size <= DUSB_VAR_FILE_VAR_FLAG_OFFSET ||
         fread(&context->flag, sizeof context->flag, 1, file) == 1) &&
        !fseek(file, (context->start += sizeof header_size + header_size +
                      sizeof data_size) - sizeof data_size, SEEK_SET) &&
        fread(&context->length, sizeof data_size, 1, file) == 1 &&
        context->length == data_size;
}

static uint8_t dusb_transfer_endpoint(dusb_state_t state) {
    switch (state) {
        default:
            return -1;
        case DUSB_SET_ADDRESS_STATE:
        case DUSB_SET_CONFIG_STATE:
            return DUSB_IN_DIRECTION | DUSB_CONTROL_ENDPOINT;
        case DUSB_BUF_SIZE_ALLOC_STATE:
        case DUSB_SOS_PING_ACK_STATE:
        case DUSB_SOS_MODE_SET_STATE:
        case DUSB_SOS_BEGIN_ACK_STATE:
        case DUSB_SOS_ACK_BEGIN_STATE:
        case DUSB_SOS_HEADER_ACK_STATE:
        case DUSB_SOS_ACK_HEADER_STATE:
        case DUSB_SOS_DATA_ACK_STATE:
        case DUSB_SOS_ACK_DATA_STATE:
        case DUSB_SOS_EOT_ACK_STATE:
        case DUSB_SOS_ACK_EOT_STATE:
        case DUSB_SEND_PING_ACK_STATE:
        case DUSB_SEND_MODE_SET_STATE:
        case DUSB_SEND_RTS_ACK_STATE:
        case DUSB_SEND_ACK_RTS_STATE:
        case DUSB_SEND_VAR_CNTS_ACK_STATE:
        case DUSB_SEND_ACK_VAR_CNTS_STATE:
        case DUSB_SEND_EOT_ACK_STATE:
            return DUSB_IN_DIRECTION | DUSB_IN_ENDPOINT;
        case DUSB_BUF_SIZE_REQ_STATE:
        case DUSB_SOS_PING_STATE:
        case DUSB_SOS_MODE_SET_ACK_STATE:
        case DUSB_SOS_BEGIN_STATE:
        case DUSB_SOS_ACK_BEGIN_ACK_STATE:
        case DUSB_SOS_HEADER_STATE:
        case DUSB_SOS_ACK_HEADER_ACK_STATE:
        case DUSB_SOS_DATA_STATE:
        case DUSB_SOS_ACK_DATA_ACK_STATE:
        case DUSB_SOS_EOT_STATE:
        case DUSB_SOS_ACK_EOT_ACK_STATE:
        case DUSB_SEND_PING_STATE:
        case DUSB_SEND_MODE_SET_ACK_STATE:
        case DUSB_SEND_RTS_STATE:
        case DUSB_SEND_ACK_RTS_ACK_STATE:
        case DUSB_SEND_VAR_CNTS_STATE:
        case DUSB_SEND_ACK_VAR_CNTS_ACK_STATE:
        case DUSB_SEND_EOT_STATE:
            return DUSB_OUT_DIRECTION | DUSB_OUT_ENDPOINT;
    }
}

static uint32_t dusb_transfer_length(dusb_context_t *context) {
    switch (context->state) {
        default:
            return -1;
        case DUSB_SET_ADDRESS_STATE:
        case DUSB_SET_CONFIG_STATE:
            return 0;
        case DUSB_BUF_SIZE_REQ_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_RPKT_BUF_SIZE_REQ_SIZE;
        case DUSB_BUF_SIZE_ALLOC_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_RPKT_BUF_SIZE_ALLOC_SIZE;
        case DUSB_SOS_PING_STATE:
        case DUSB_SEND_PING_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_PING_SIZE;
        case DUSB_SOS_PING_ACK_STATE:
        case DUSB_SOS_MODE_SET_ACK_STATE:
        case DUSB_SOS_BEGIN_ACK_STATE:
        case DUSB_SOS_ACK_BEGIN_ACK_STATE:
        case DUSB_SOS_HEADER_ACK_STATE:
        case DUSB_SOS_ACK_HEADER_ACK_STATE:
        case DUSB_SOS_DATA_ACK_STATE:
        case DUSB_SOS_ACK_DATA_ACK_STATE:
        case DUSB_SOS_EOT_ACK_STATE:
        case DUSB_SOS_ACK_EOT_ACK_STATE:
        case DUSB_SEND_PING_ACK_STATE:
        case DUSB_SEND_MODE_SET_ACK_STATE:
        case DUSB_SEND_RTS_ACK_STATE:
        case DUSB_SEND_ACK_RTS_ACK_STATE:
        case DUSB_SEND_VAR_CNTS_ACK_STATE:
        case DUSB_SEND_ACK_VAR_CNTS_ACK_STATE:
        case DUSB_SEND_EOT_ACK_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_RPKT_VIRT_DATA_ACK_SIZE;
        case DUSB_SOS_MODE_SET_STATE:
        case DUSB_SEND_MODE_SET_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_MODE_SET_SIZE;
        case DUSB_SOS_BEGIN_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_BEGIN_SIZE;
        case DUSB_SOS_ACK_BEGIN_STATE:
        case DUSB_SOS_ACK_HEADER_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_ACK_SIZE;
        case DUSB_SOS_HEADER_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_HEADER_SIZE;
        case DUSB_SOS_DATA_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE +
                min32(context->max_vpkt_size, DUSB_VPKT_OS_DATA_HEADER_SIZE +
                      context->length - context->position);
        case DUSB_SOS_ACK_DATA_STATE:
        case DUSB_SEND_ACK_RTS_STATE:
        case DUSB_SEND_ACK_VAR_CNTS_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_DATA_ACK_SIZE;
        case DUSB_SOS_EOT_STATE:
        case DUSB_SEND_EOT_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_EOT_SIZE;
        case DUSB_SOS_ACK_EOT_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_EOT_ACK_SIZE;
        case DUSB_SEND_RTS_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_RTS_SIZE +
                context->command->varname_utf8_length +
                DUSB_ATTR_HEADER_SIZE + DUSB_ATTR_VAR_TYPE_SIZE +
                DUSB_ATTR_HEADER_SIZE + DUSB_ATTR_ARCHIVED_SIZE +
                (context->command->vartype == CALC_VAR_TYPE_FLASH_APP ? 0 :
                 DUSB_ATTR_HEADER_SIZE + DUSB_ATTR_VAR_VERSION_SIZE);
        case DUSB_SEND_VAR_CNTS_STATE:
            return min32(context->max_rpkt_size, DUSB_RPKT_HEADER_SIZE +
                         (context->position ? 0 : DUSB_VPKT_HEADER_SIZE) +
                         context->length - context->position);
    }
}

static uint32_t dusb_transfer_header_length(dusb_context_t *context) {
    switch (context->state) {
        default:
            return -1;
        case DUSB_SOS_HEADER_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE;
        case DUSB_SOS_DATA_STATE:
            return DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_DATA_HEADER_SIZE;
        case DUSB_SEND_VAR_CNTS_STATE:
            return DUSB_RPKT_HEADER_SIZE + (context->position ? 0 : DUSB_VPKT_HEADER_SIZE);
    }
}

static uint32_t dusb_transfer_remaining(dusb_context_t *context) {
    return dusb_transfer_length(context) - dusb_transfer_header_length(context) - context->offset;
}

static int dusb_transition(usb_event_t *event, dusb_state_t state) {
    dusb_context_t *context = event->context;
    usb_transfer_info_t *transfer = &event->info.transfer;
    usb_timer_info_t *timer = &event->info.timer;
    dusb_state_t original_state = context->state;
    (void)original_state;

    while (true) {
        switch ((context->state = state)) {
            case DUSB_INIT_STATE:
            case DUSB_RESET_RECOVERY_STATE:
                event->type = USB_TIMER_EVENT;
                timer->mode = USB_TIMER_ABSOLUTE_MODE;
                timer->useconds = 10000;
                break;
            case DUSB_RESET_STATE:
                event->type = USB_RESET_EVENT;
                break;
            case DUSB_SET_ADDRESS_STATE:
            case DUSB_SET_CONFIG_STATE:
                event->type = USB_TRANSFER_EVENT;
                memset(transfer->buffer = context->buffer, 0, transfer->length = 8);
                context->buffer[1] = context->state == DUSB_SET_ADDRESS_STATE ? 5 : 9;
                context->buffer[2] = 1;
                transfer->endpoint = DUSB_CONTROL_ENDPOINT;
                transfer->setup = true;
                transfer->direction = false;
                break;
            case DUSB_BUF_SIZE_REQ_STATE:
            case DUSB_SOS_PING_STATE:
            case DUSB_SOS_MODE_SET_ACK_STATE:
            case DUSB_SOS_BEGIN_STATE:
            case DUSB_SOS_ACK_BEGIN_ACK_STATE:
            case DUSB_SOS_HEADER_STATE:
            case DUSB_SOS_ACK_HEADER_ACK_STATE:
            case DUSB_SOS_DATA_STATE:
            case DUSB_SOS_ACK_DATA_ACK_STATE:
            case DUSB_SOS_EOT_STATE:
            case DUSB_SOS_ACK_EOT_ACK_STATE:
            case DUSB_SEND_PING_STATE:
            case DUSB_SEND_MODE_SET_ACK_STATE:
            case DUSB_SEND_RTS_STATE:
            case DUSB_SEND_ACK_RTS_ACK_STATE:
            case DUSB_SEND_VAR_CNTS_STATE:
            case DUSB_SEND_ACK_VAR_CNTS_ACK_STATE:
            case DUSB_SEND_EOT_STATE:
                event->type = USB_TRANSFER_EVENT;
                transfer->length = dusb_transfer_length(context);
                transfer->endpoint = DUSB_OUT_ENDPOINT;
                transfer->setup = false;
                transfer->direction = false;
                break;
            case DUSB_BUF_SIZE_ALLOC_STATE:
            case DUSB_SOS_DATA_ACK_STATE:
            case DUSB_SOS_ACK_DATA_STATE:
            case DUSB_SOS_EOT_ACK_STATE:
            case DUSB_SOS_ACK_EOT_STATE:
            case DUSB_SEND_RTS_ACK_STATE:
            case DUSB_SEND_ACK_RTS_STATE:
            case DUSB_SEND_VAR_CNTS_ACK_STATE:
            case DUSB_SEND_ACK_VAR_CNTS_STATE:
            case DUSB_SEND_EOT_ACK_STATE:
                break;
            case DUSB_NEXT_COMMAND_STATE:
                ++context->command;
                /* fallthrough */
            case DUSB_COMMAND_STATE:
                switch (context->command->type) {
                    case DUSB_DONE_COMMAND:
                        event->type = USB_DESTROY_EVENT;
                        break;
                    case DUSB_SCREENSHOT_COMMAND:
                        state = DUSB_INVALID_STATE;
                        continue;
                    case DUSB_ROMDUMP_COMMAND:
                        state = DUSB_INVALID_STATE;
                        continue;
                    case DUSB_SEND_COMMAND:
                        state = dusb_detect(context);
                        continue;
                    case DUSB_RECEIVE_COMMAND:
                        state = DUSB_INVALID_STATE;
                        continue;
                    default:
                        state = DUSB_INVALID_STATE;
                        continue;
                }
                break;
            case DUSB_SOS_PING_ACK_STATE:
            case DUSB_SOS_MODE_SET_STATE:
            case DUSB_SOS_BEGIN_ACK_STATE:
            case DUSB_SOS_ACK_BEGIN_STATE:
            case DUSB_SOS_HEADER_ACK_STATE:
            case DUSB_SOS_ACK_HEADER_STATE:
            case DUSB_SEND_PING_ACK_STATE:
            case DUSB_SEND_MODE_SET_STATE:
                context->position = context->offset = 0;
                break;
            case DUSB_SOS_NEXT_DATA_STATE:
            case DUSB_SEND_NEXT_VAR_CNTS_STATE:
                context->position += context->offset;
                context->offset = 0;
                if (context->position != context->length) {
                    switch (state) {
                        case DUSB_SOS_NEXT_DATA_STATE:
                            state = DUSB_SOS_DATA_STATE;
                            break;
                        case DUSB_SEND_NEXT_VAR_CNTS_STATE:
                            state = DUSB_SEND_VAR_CNTS_STATE;
                            break;
                        default:
                            unreachable();
                    }
                } else {
                    context->start += context->position;
                    context->position = 0;
                    ++state;
                }
                continue;
            case DUSB_SOS_DONE_STATE:
                if (context->command->type == DUSB_DONE_COMMAND) {
                    state = DUSB_NEXT_COMMAND_STATE;
                    continue;
                }
                /* Wait for calculator to reboot. */
                ++context->command;
                context->state = DUSB_INIT_STATE;
                event->type = USB_TIMER_EVENT;
                timer->mode = USB_TIMER_ABSOLUTE_MODE;
                timer->useconds = 2000000;
                break;
            case DUSB_SEND_NEXT_STATE:
                state = dusb_detect_var(context) ? DUSB_SEND_PING_STATE : DUSB_NEXT_COMMAND_STATE;
                continue;
            case DUSB_INVALID_STATE:
                event->type = USB_DESTROY_EVENT;
                break;
        }
        break;
    }
    return 0;
}

static int parse_hex_digit(char c) {
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' - 10;
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' - 10;
    }
    return -1;
}

static int parse_hex_digits(const char *c) {
    return parse_hex_digit(c[0]) << 4 | parse_hex_digit(c[1]) << 0;
}

int usb_dusb_device(usb_event_t *event) {
    dusb_context_t *context = event->context;
    dusb_command_t *command;
    usb_event_type_t type = event->type;
    usb_init_info_t *init = &event->info.init;
    usb_transfer_info_t *transfer = &event->info.transfer;
    usb_timer_info_t *timer = &event->info.timer;
    uint8_t *buffer;
    uint32_t length, transfer_length;
    uint16_t attribute_count;
    uint8_t endpoint;
    event->type = USB_INIT_EVENT;
    switch (type) {
        case USB_INIT_EVENT:
            if (init->argc < 1) {
                return EINVAL;
            }
            context = calloc(1, sizeof(dusb_context_t) + sizeof(dusb_command_t) * init->argc);
            if (!context) {
                return ENOMEM;
            }
            command = context->command = context->commands;
            for (const char *const *argv = &init->argv[1]; --init->argc; ++argv, ++command) {
                const char *arg = *argv;
                command->type = DUSB_NUM_COMMANDS;
                while (--command->type) {
                    const char *argp = arg, *command_name = command_names[command->type];
                    char c;
                    do {
                        c = *argp++;
                        if (c >= 'A' && c <= 'Z') {
                            c += 'a' - 'A';
                        }
                    } while (c == *command_name && *++command_name);
                    if (!*command_name) {
                        arg = argp;
                        break;
                    }
                }
                if (!command->type) {
                    free(context);
                    return EINVAL;
                }
                while (*arg == ',') {
                    int type = parse_hex_digits(++arg);
                    if (type < 0) {
                        free(context);
                        return EINVAL;
                    }
                    command->vartype = type;
                    do {
                        int c = parse_hex_digits(arg += 2);
                        if (c < 0) {
                            break;
                        }
                        command->varname[command->varname_length++] = c;
                    } while (command->varname_length != sizeof command->varname);
                    while (!command->varname[command->varname_length - 1]) {
                        --command->varname_length;
                    }
                }
                if (*arg == ':' ?
                    !(command->file = fopen(++arg, command->type == DUSB_SEND_COMMAND ? "rb" : "wb")) : *arg) {
                    free(context);
                    return EINVAL;
                }
            }
            event->context = context;
            return dusb_transition(event, DUSB_INIT_STATE);
        case USB_RESET_EVENT:
            switch (context->state) {
                case DUSB_RESET_STATE:
                    return dusb_transition(event, DUSB_RESET_RECOVERY_STATE);
                default:
                    return dusb_transition(event, DUSB_INVALID_STATE);
            }
        case USB_TRANSFER_EVENT:
            command = context->command;
            buffer = transfer->buffer;
            length = transfer->length;
            if (transfer->setup) {
                return EINVAL;
            }
            if (transfer->endpoint == DUSB_IN_ENDPOINT && transfer->direction &&
                length >= DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_DELAY_ACK_SIZE &&
                buffer[4] == DUSB_RPKT_VIRT_DATA_LAST && (buffer[9] << 8 | buffer[10] << 0) == DUSB_VPKT_DELAY_ACK) {
                event->type = USB_TIMER_EVENT;
                timer->mode = USB_TIMER_ABSOLUTE_MODE;
                timer->useconds = buffer[11] << 24 | buffer[12] << 16 | buffer[13] << 8 | buffer[14] << 0;
                return 0;
            }
            transfer_length = dusb_transfer_length(context);
            endpoint = dusb_transfer_endpoint(context->state);
            if ((length != transfer->max_pkt_size && length != transfer_length % transfer->max_pkt_size) ||
                transfer->endpoint != (endpoint & ~DUSB_IN_DIRECTION) || transfer->direction != endpoint >> 7) {
                return dusb_transition(event, DUSB_INVALID_STATE);
            }
            switch (context->state) {
                case DUSB_SET_ADDRESS_STATE:
                case DUSB_SET_CONFIG_STATE:
                    break;
                case DUSB_BUF_SIZE_REQ_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_BUF_SIZE_REQ;

                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 4;
                    *buffer++ = 0;
                    break;
                case DUSB_BUF_SIZE_ALLOC_STATE:
                    if (buffer[4] != DUSB_RPKT_BUF_SIZE_ALLOC) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    context->max_rpkt_size = buffer[5] << 24 | buffer[6] << 16 | buffer[7] << 8 | buffer[8] << 0;
                    break;
                case DUSB_SOS_PING_ACK_STATE:
                case DUSB_SOS_BEGIN_ACK_STATE:
                case DUSB_SOS_HEADER_ACK_STATE:
                case DUSB_SOS_DATA_ACK_STATE:
                case DUSB_SOS_EOT_ACK_STATE:
                case DUSB_SEND_PING_ACK_STATE:
                case DUSB_SEND_RTS_ACK_STATE:
                case DUSB_SEND_VAR_CNTS_ACK_STATE:
                case DUSB_SEND_EOT_ACK_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_ACK) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    break;
                case DUSB_SOS_PING_STATE:
                case DUSB_SEND_PING_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;

                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_VPKT_PING >> 8 & 0xFF;
                    *buffer++ = DUSB_VPKT_PING >> 0 & 0xFF;

                    *buffer++ = 0;
                    *buffer++ = context->state == DUSB_SOS_PING_STATE ? 2 : 3;
                    *buffer++ = 0;
                    *buffer++ = 1;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = context->state == DUSB_SOS_PING_STATE ? 0x0f : 0x07;
                    *buffer++ = context->state == DUSB_SOS_PING_STATE ? 0xa0 : 0xd0;
                    break;
                case DUSB_SOS_MODE_SET_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_LAST || (buffer[9] << 8 | buffer[10] << 0) != DUSB_VPKT_MODE_SET ||
                        buffer[11] || buffer[12] || buffer[13] != 0x0f || buffer[14] != 0xa0) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    break;
                case DUSB_SOS_MODE_SET_ACK_STATE:
                case DUSB_SOS_ACK_BEGIN_ACK_STATE:
                case DUSB_SOS_ACK_HEADER_ACK_STATE:
                case DUSB_SOS_ACK_DATA_ACK_STATE:
                case DUSB_SOS_ACK_EOT_ACK_STATE:
                case DUSB_SEND_MODE_SET_ACK_STATE:
                case DUSB_SEND_ACK_RTS_ACK_STATE:
                case DUSB_SEND_ACK_VAR_CNTS_ACK_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_VIRT_DATA_ACK;
                    *buffer++ = 0xE0;
                    *buffer++ = 0;
                    break;
                case DUSB_SOS_BEGIN_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_VPKT_OS_BEGIN >> 8 & 0xFF;
                    *buffer++ = DUSB_VPKT_OS_BEGIN >> 0 & 0xFF;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = 0;
                    *buffer++ = context->length >> 24 & 0xFF;
                    *buffer++ = context->length >> 16 & 0xFF;
                    *buffer++ = context->length >>  8 & 0xFF;
                    *buffer++ = context->length >>  0 & 0xFF;
                    break;
                case DUSB_SOS_ACK_BEGIN_STATE:
                case DUSB_SOS_ACK_HEADER_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_LAST || (buffer[9] << 8 | buffer[10] << 0) != DUSB_VPKT_OS_ACK) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    context->max_vpkt_size = buffer[11] << 24 | buffer[12] << 16 | buffer[13] << 8 | buffer[14] << 0;
                    break;
                case DUSB_SOS_HEADER_STATE:
                    if (!length) {
                        break;
                    }
                    if (!context->offset) {
                        if (length < DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE) {
                            return dusb_transition(event, DUSB_INVALID_STATE);
                        }
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                        *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                        *buffer++ = DUSB_VPKT_OS_HEADER >> 8 & 0xFF;
                        *buffer++ = DUSB_VPKT_OS_HEADER >> 0 & 0xFF;
                        length -= DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE;
                    }
                    length = min32(dusb_transfer_remaining(context), length);
                    if (fseek(command->file, DUSB_FLASH_FILE_DATA_OFFSET + context->offset, SEEK_SET) ||
                        fread(buffer, length, 1, command->file) != 1) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    context->offset += length;
                    if ((transfer->length = dusb_transfer_remaining(context)) ||
                        !(transfer_length % transfer->max_pkt_size)) {
                        event->type = USB_TRANSFER_EVENT;
                        return 0;
                    }
                    break;
                case DUSB_SOS_DATA_STATE:
                    if (!length) {
                        break;
                    }
                    if (!context->offset) {
                        if (length < DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_DATA_HEADER_SIZE) {
                            return dusb_transition(event, DUSB_INVALID_STATE);
                        }
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                        *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                        *buffer++ = DUSB_VPKT_OS_DATA >> 8 & 0xFF;
                        *buffer++ = DUSB_VPKT_OS_DATA >> 0 & 0xFF;
                        *buffer++ = (DUSB_OS_BASE_ADDR + context->position) >>  0 & 0xFF;
                        *buffer++ = (DUSB_OS_BASE_ADDR + context->position) >>  8 & 0xFF;
                        *buffer++ = (DUSB_OS_BASE_ADDR + context->position) >> 16 & 0xFF;
                        *buffer++ = (DUSB_OS_BASE_ADDR + context->position) >> 24 & 0xFF;
                        length -= DUSB_RPKT_HEADER_SIZE + DUSB_VPKT_HEADER_SIZE + DUSB_VPKT_OS_DATA_HEADER_SIZE;
                    }
                    length = min32(dusb_transfer_remaining(context), length);
                    if (fseek(command->file, DUSB_FLASH_FILE_DATA_OFFSET + context->position + context->offset, SEEK_SET) ||
                        fread(buffer, length, 1, command->file) != 1) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    context->offset += length;
                    if ((transfer->length = dusb_transfer_remaining(context)) ||
                        !(transfer_length % transfer->max_pkt_size)) {
                        event->type = USB_TRANSFER_EVENT;
                        return 0;
                    }
                    break;
                case DUSB_SOS_ACK_DATA_STATE:
                case DUSB_SEND_ACK_RTS_STATE:
                case DUSB_SEND_ACK_VAR_CNTS_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_LAST || (buffer[9] << 8 | buffer[10] << 0) != DUSB_VPKT_DATA_ACK) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    break;
                case DUSB_SOS_EOT_STATE:
                case DUSB_SEND_EOT_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_VPKT_EOT >> 8 & 0xFF;
                    *buffer++ = DUSB_VPKT_EOT >> 0 & 0xFF;
                    break;
                case DUSB_SOS_ACK_EOT_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_LAST || (buffer[9] << 8 | buffer[10] << 0) != DUSB_VPKT_EOT_ACK) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    break;
                case DUSB_SEND_MODE_SET_STATE:
                    if (buffer[4] != DUSB_RPKT_VIRT_DATA_LAST || (buffer[9] << 8 | buffer[10] << 0) != DUSB_VPKT_MODE_SET ||
                        buffer[11] || buffer[12] || buffer[13] != 0x07 || buffer[14] != 0xd0) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    break;
                case DUSB_SEND_RTS_STATE:
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_RPKT_VIRT_DATA_LAST;

                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 24 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >> 16 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  8 & 0xFF;
                    *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE - DUSB_VPKT_HEADER_SIZE) >>  0 & 0xFF;
                    *buffer++ = DUSB_VPKT_RTS >> 8 & 0xFF;
                    *buffer++ = DUSB_VPKT_RTS >> 0 & 0xFF;

                    *buffer++ = 0;
                    *buffer++ = command->varname_utf8_length;
                    memcpy(buffer, command->varname_utf8, command->varname_utf8_length);
                    buffer += command->varname_utf8_length;
                    *buffer++ = '\0';
                    *buffer++ = context->length >> 24 & 0xFF;
                    *buffer++ = context->length >> 16 & 0xFF;
                    *buffer++ = context->length >>  8 & 0xFF;
                    *buffer++ = context->length >>  0 & 0xFF;
                    *buffer++ = DUSB_VPKT_RTS_SILENT;
                    attribute_count = command->vartype == CALC_VAR_TYPE_FLASH_APP ? 2 : 3;
                    *buffer++ = attribute_count >> 8 & 0xFF;
                    *buffer++ = attribute_count >> 0 & 0xFF;

                    *buffer++ = DUSB_ATTR_VAR_TYPE >> 8 & 0xFF;
                    *buffer++ = DUSB_ATTR_VAR_TYPE >> 0 & 0xFF;
                    *buffer++ = DUSB_ATTR_VAR_TYPE_SIZE >> 8 & 0xFF;
                    *buffer++ = DUSB_ATTR_VAR_TYPE_SIZE >> 0 & 0xFF;
                    *buffer++ = 0xf0;
                    *buffer++ = command->vartype == CALC_VAR_TYPE_FLASH_APP ? 0x0f : 0x07;
                    *buffer++ = 0;
                    *buffer++ = command->vartype;

                    *buffer++ = DUSB_ATTR_ARCHIVED >> 8 & 0xFF;
                    *buffer++ = DUSB_ATTR_ARCHIVED >> 0 & 0xFF;
                    *buffer++ = DUSB_ATTR_ARCHIVED_SIZE >> 8 & 0xFF;
                    *buffer++ = DUSB_ATTR_ARCHIVED_SIZE >> 0 & 0xFF;
                    *buffer++ = context->flag >> 7 & 1;

                    if (command->vartype != CALC_VAR_TYPE_FLASH_APP) {
                        *buffer++ = DUSB_ATTR_VAR_VERSION >> 8 & 0xFF;
                        *buffer++ = DUSB_ATTR_VAR_VERSION >> 0 & 0xFF;
                        *buffer++ = DUSB_ATTR_VAR_VERSION_SIZE >> 8 & 0xFF;
                        *buffer++ = DUSB_ATTR_VAR_VERSION_SIZE >> 0 & 0xFF;
                        *buffer++ = 0;
                        *buffer++ = 0;
                        *buffer++ = 0;
                        *buffer++ = context->version;
                    }
                    break;
                case DUSB_SEND_VAR_CNTS_STATE:
                    if (!length) {
                        break;
                    }
                    if (!context->offset) {
                        if (length < DUSB_RPKT_HEADER_SIZE) {
                            return dusb_transition(event, DUSB_INVALID_STATE);
                        }
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 24 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >> 16 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  8 & 0xFF;
                        *buffer++ = (transfer_length - DUSB_RPKT_HEADER_SIZE) >>  0 & 0xFF;
                        *buffer++ = context->position + dusb_transfer_remaining(context) != context->length ?
                            DUSB_RPKT_VIRT_DATA : DUSB_RPKT_VIRT_DATA_LAST;
                        length -= DUSB_RPKT_HEADER_SIZE;
                        if (!context->position) {
                            if (length < DUSB_VPKT_HEADER_SIZE) {
                                return dusb_transition(event, DUSB_INVALID_STATE);
                            }
                            *buffer++ = context->length >> 24 & 0xFF;
                            *buffer++ = context->length >> 16 & 0xFF;
                            *buffer++ = context->length >>  8 & 0xFF;
                            *buffer++ = context->length >>  0 & 0xFF;
                            *buffer++ = DUSB_VPKT_VAR_CNTS >> 8 & 0xFF;
                            *buffer++ = DUSB_VPKT_VAR_CNTS >> 0 & 0xFF;
                            length -= DUSB_VPKT_HEADER_SIZE;
                        }
                    }
                    length = min32(dusb_transfer_remaining(context), length);
                    if (fseek(command->file, context->start + context->position + context->offset, SEEK_SET) ||
                        fread(buffer, length, 1, command->file) != 1) {
                        return dusb_transition(event, DUSB_INVALID_STATE);
                    }
                    context->offset += length;
                    if ((transfer->length = dusb_transfer_remaining(context)) ||
                        !(transfer_length % transfer->max_pkt_size)) {
                        event->type = USB_TRANSFER_EVENT;
                        return 0;
                    }
                    break;
                default:
                    return dusb_transition(event, DUSB_INVALID_STATE);
            }
            return dusb_transition(event, context->state + 1);
        case USB_TIMER_EVENT:
            switch (context->state) {
                case DUSB_INIT_STATE:
                    return dusb_transition(event, DUSB_RESET_STATE);
                case DUSB_RESET_RECOVERY_STATE:
                    return dusb_transition(event, DUSB_SET_ADDRESS_STATE);
                default:
                    return 0;
            }
        case USB_DESTROY_EVENT:
            if (context) {
                return 0;
            }
            for (command = context->command; command && command->type; ++command) {
                if (command->file) {
                    fclose(command->file);
                }
            }
            free(context);
            event->context = NULL;
            return 0;
        default:
            return EINVAL;
    }
}

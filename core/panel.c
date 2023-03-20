#include "panel.h"
#include "bus.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

panel_state_t panel;

static bool panel_scan_line(uint16_t row) {
    if (unlikely(row > PANEL_LAST_ROW)) {
        panel.mode |= PANEL_MODE_IGNORE;
        return false;
    }
    panel.mode &= ~PANEL_MODE_IGNORE;
    if (unlikely(panel.mode & PANEL_MODE_PARTIAL) &&
        panel.partialStart > panel.partialEnd ?
        panel.partialStart > row && row > panel.partialEnd :
        panel.partialStart > row || row > panel.partialEnd) {
        panel.mode |= PANEL_MODE_BLANK;
    } else {
        panel.mode &= ~PANEL_MODE_BLANK;
    }
    panel.row = panel.dstRow = panel.srcRow = row;
    if (unlikely(panel.mode & PANEL_MODE_SCROLL)) {
        uint16_t top = panel.topArea, bot = PANEL_LAST_ROW - panel.bottomArea;
        if (row >= top && row <= bot) {
            panel.srcRow += panel.scrollStart - top;
            if (panel.srcRow > bot) {
                panel.srcRow -= PANEL_NUM_ROWS - panel.topArea - panel.bottomArea;
            }
            panel.srcRow &= 0x1FF;
        }
    }
    if (unlikely(panel.gateConfig & PANEL_GATE_INTERLACE)) {
        panel.dstRow *= 2;
        if (panel.dstRow >= PANEL_NUM_ROWS) {
            panel.dstRow -= (PANEL_NUM_ROWS - 1);
        }
    }
    if (unlikely(panel.mac & PANEL_MAC_VRO)) {
        panel.dstRow = PANEL_LAST_ROW - panel.dstRow;
        panel.srcRow = PANEL_LAST_ROW - panel.srcRow;
    }
    if (unlikely(panel.mac & PANEL_MAC_HRO)) {
        panel.col = PANEL_LAST_COL;
        panel.colDir = -1;
    } else {
        panel.col = 0;
        panel.colDir = 1;
    }
    return true;
}

bool panel_hsync(void) {
    return panel_scan_line(panel.row + 1);
}

static void panel_reset_mregs(void) {
    if (unlikely(panel.mac & PANEL_MAC_RCX)) {
        panel.rowReg = panel.rowStart;
        panel.colReg = panel.colStart;
    } else {
        panel.rowReg = panel.colStart;
        panel.colReg = panel.rowStart;
    }
}

bool panel_vsync(void) {
    if (likely(panel.ifCtl & PANEL_IC_CTRL_DATA)) {
        panel_reset_mregs();
    }
    return panel_scan_line(0);
}

bool panel_refresh_pixel(void) {
    uint8_t *pixel, red, green, blue;
    if (unlikely(panel.mode & PANEL_MODE_IGNORE)) {
        return false;
    }
    if (unlikely(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF | PANEL_MODE_BLANK))) {
        red = green = blue = ~0;
    } else {
        if (unlikely(panel.srcRow > PANEL_LAST_ROW)) {
            red = bus_rand();
            green = bus_rand();
            blue = bus_rand();
        } else {
            pixel = panel.frame[panel.srcRow][panel.col];
            red = pixel[PANEL_RED];
            green = pixel[PANEL_GREEN];
            blue = pixel[PANEL_BLUE];
        }
        if (!likely(panel.mac & PANEL_MAC_BGR)) { /* eor */
            uint8_t temp = red;
            red = blue;
            blue = temp;
        }
        if (unlikely(panel.mode & PANEL_MODE_INVERT)) {
            red = ~red;
            green = ~green;
            blue = ~blue;
        }
        if (unlikely(panel.mode & PANEL_MODE_IDLE)) {
            red = (int8_t)red >> 7;
            green = (int8_t)green >> 7;
            blue = (int8_t)blue >> 7;
        }
    }
    pixel = panel.display[panel.col][panel.dstRow];
    pixel[PANEL_RED] = red;
    pixel[PANEL_GREEN] = green;
    pixel[PANEL_BLUE] = blue;
    pixel[PANEL_ALPHA] = ~0;
    panel.col += panel.colDir;
    if (unlikely(panel.col > PANEL_LAST_COL)) {
        panel.mode |= PANEL_MODE_IGNORE;
        return false;
    }
    return true;
}

static void panel_update_pixel(uint8_t red, uint8_t green, uint8_t blue) {
    if (likely(panel.rowReg < 320 && panel.colReg < 240)) {
        uint8_t *pixel = panel.frame[panel.rowReg][panel.colReg];
        pixel[PANEL_RED] = red;
        pixel[PANEL_GREEN] = green;
        pixel[PANEL_BLUE] = blue;
    }
    if (unlikely(panel.mac & PANEL_MAC_RCX)) {
        if (unlikely(panel.colReg == panel.colEnd)) {
            if (unlikely(panel.rowReg == panel.rowEnd && panel.rowStart <= panel.rowEnd)) {
                panel.rowReg = panel.colReg = ~0;
            } else {
                panel.colReg = panel.colStart;
                panel.rowReg = (panel.rowReg + 1 - (panel.mac >> 6 & 2)) & 0x1FF;
            }
        } else if (panel.colReg < 0x100) {
            panel.colReg = (panel.colReg + 1 - (panel.mac >> 5 & 2)) & 0xFF;
        }
    } else {
        if (unlikely(panel.rowReg == panel.colEnd)) {
            if (unlikely(panel.colReg == panel.rowEnd && panel.rowStart <= panel.rowEnd)) {
                panel.rowReg = panel.colReg = ~0;
            } else {
                panel.rowReg = panel.colStart;
                panel.colReg = (panel.colReg + 1 - (panel.mac >> 5 & 2)) & 0xFF;
            }
        } else if (panel.rowReg < 0x200) {
            panel.rowReg = (panel.rowReg + 1 - (panel.mac >> 6 & 2)) & 0x1FF;
        }
    }
}

void panel_update_pixel_18bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 64 && green < 64 && blue < 64);
    panel_update_pixel(red << 2 | red >> 4, green << 2 | green >> 4, blue << 2 | blue >> 4);
}

void panel_update_pixel_16bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 32 && green < 64 && blue < 32);
    panel_update_pixel(panel.lut[red + 0], panel.lut[green + 32], panel.lut[blue + 96]);
}

void panel_update_pixel_12bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 16 && green < 16 && blue < 16);
    panel_update_pixel(panel.lut[(red << 1) + 0], panel.lut[(green << 2) + 32], panel.lut[(blue << 1) + 96]);
}

static void panel_sw_reset(void) {
    panel.cmd = 0;
    panel.param = 0;
    panel.gamma = 1;
    panel.mode = PANEL_MODE_SLEEP | PANEL_MODE_OFF | PANEL_MODE_IGNORE;
    panel.colStart = 0;
    panel.colEnd = panel.mac & PANEL_MAC_RCX ? PANEL_LAST_COL : PANEL_LAST_ROW;
    panel.rowStart = 0;
    panel.rowEnd = panel.mac & PANEL_MAC_RCX ? PANEL_LAST_ROW : PANEL_LAST_COL;
    panel.topArea = 0;
    panel.scrollArea = PANEL_NUM_ROWS;
    panel.bottomArea = 0;
    panel.partialStart = 0;
    panel.partialEnd = PANEL_LAST_ROW;
    panel.scrollStart = 0;
    panel.gateCount = PANEL_NUM_ROWS / 8 - 1;
    panel.gateStart = 0 / 8;
    panel.gateConfig = PANEL_GATE_MIRROR;
    panel.tear = false;
}

static void panel_hw_reset(void) {
    panel.mac = 0;
    panel_sw_reset();
}

static void panel_write_cmd(uint8_t value) {
    panel.cmd = value;
    panel.param = 0;

    switch (panel.cmd) {
        case 0x00:
            break;
        case 0x01:
            panel_sw_reset();
            break;
        case 0x10:
            panel.mode |= PANEL_MODE_SLEEP;
            break;
        case 0x11:
            panel.mode &= ~PANEL_MODE_SLEEP;
            break;
        case 0x12:
            panel.mode |= PANEL_MODE_PARTIAL;
            panel.scrollStart = 0;
            break;
        case 0x13:
            panel.mode &= ~(PANEL_MODE_PARTIAL | PANEL_MODE_SCROLL);
            panel.scrollStart = 0;
            break;
        case 0x20:
            panel.mode &= ~PANEL_MODE_INVERT;
            break;
        case 0x21:
            panel.mode |= PANEL_MODE_INVERT;
            break;
        case 0x28:
            panel.mode |= PANEL_MODE_OFF;
            break;
        case 0x29:
            panel.mode &= ~PANEL_MODE_OFF;
            break;
        case 0x2C:
            panel_reset_mregs();
            break;
        case 0x34:
            panel.tear = false;
            break;
        case 0x35:
            panel.tear = true;
            break;
        case 0x38:
            panel.mode &= ~PANEL_MODE_IDLE;
            break;
        case 0x39:
            panel.mode |= PANEL_MODE_IDLE;
            break;
        default:
            break;
    }
}

static void panel_write_param(uint8_t value) {
    uint8_t word_param = panel.param >> 1;
    uint8_t bit_offset = ~panel.param << 3 & 8;

    switch (panel.cmd) {
        case 0x26:
            if (panel.param == 0) {
                panel.gamma = value;
            }
            break;
        case 0x2A:
            switch (word_param) {
                case 0:
                    write8(panel.colStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(panel.colEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x2B:
            switch (word_param) {
                case 0:
                    write8(panel.rowStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(panel.rowEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x2C:
        case 0x3C:
            if (unlikely(!(panel.ifCtl & PANEL_IC_CTRL_DATA))) {
                switch (panel.ifBpp & 7) {
                    default:
                    case 6: /* 18bpp */
                        switch (panel.param) {
                            case 0:
                                panel.ifBlue = value >> 2;
                                break;
                            case 1:
                                panel.ifGreen = value >> 2;
                                break;
                            case 2:
                                panel.ifRed = value >> 2;
                                panel_update_pixel_18bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                                panel.param = (uint8_t)-1;
                                break;
                        }
                        break;
                    case 5: /* 16bpp */
                        switch (panel.param) {
                            case 0:
                                panel.ifBlue = value >> 3;
                                panel.ifGreen = value << 3 & 0x38;
                                break;
                            case 1:
                                panel.ifGreen |= value >> 5;
                                panel.ifRed = value & 0x1F;
                                panel_update_pixel_16bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                                panel.param = (uint8_t)-1;
                                break;
                        }
                        break;
                    case 3: /* 12bpp */
                        switch (panel.param) {
                            case 0:
                                panel.ifBlue = value >> 4;
                                panel.ifGreen = value & 0xF;
                                break;
                            case 1:
                                panel.ifRed = value >> 4;
                                panel_update_pixel_12bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                                panel.ifBlue = value & 0xF;
                                break;
                            case 2:
                                panel.ifGreen = value >> 4;
                                panel.ifRed = value & 0xF;
                                panel_update_pixel_12bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                                panel.param = (uint8_t)-1;
                                break;
                        }
                        break;
                }
            }
            break;
        case 0x2D:
            break;
        case 0x30:
            switch (word_param) {
                case 0:
                    write8(panel.partialStart, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(panel.partialEnd, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x33:
            switch (word_param) {
                case 0:
                    write8(panel.topArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 1:
                    write8(panel.scrollArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                case 2:
                    write8(panel.bottomArea, bit_offset, value & 0x1FF >> bit_offset);
                    break;
                default:
                    break;
            }
            break;
        case 0x36:
            if (panel.param == 0) {
                panel.mac = value;
            }
            break;
        case 0x37:
            switch (word_param) {
                case 0:
                    write8(panel.scrollStart, bit_offset, value & 0x1FF >> bit_offset);
                    panel.mode |= PANEL_MODE_SCROLL;
                    break;
                default:
                    break;
            }
            break;
        case 0x3A:
            switch (panel.param) {
                case 0:
                    panel.ifBpp = value;
                    break;
                default:
                    break;
            }
            break;
        case 0xB0:
            switch (panel.param) {
                case 0:
                    panel.ifCtl = value;
                    break;
                case 1:
                    break;
                default:
                    break;
            }
            break;
        case 0xE0:
            if (panel.param < 16) {
                panel.gammaCorrection[0][panel.param] = value;
            }
            break;
        case 0xE1:
            if (panel.param < 16) {
                panel.gammaCorrection[1][panel.param] = value;
            }
            break;
        case 0xE4:
            switch (panel.param) {
                case 0:
                    panel.gateConfig = value;
                    break;
                case 1:
                    panel.gateStart = value;
                    break;
                case 2:
                    panel.gateConfig = value;
                    break;
                default:
                    break;
            }
            break;
        default:
            break;
    }

    panel.param++;
}

uint8_t panel_spi_select(uint32_t* rxData) {
    (void)rxData;
    /* The first transfer frame is always 9 bits */
    return 9;
}

uint8_t panel_spi_transfer(uint32_t txData, uint32_t* rxData) {
    (void)rxData;
    (txData & 0x100 ? panel_write_param : panel_write_cmd)((uint8_t)txData);
    /* TODO: return different frame length after read commands */
    return 9;
}

void panel_spi_deselect(void) {
}

static void panel_init_luts(void) {
    uint8_t i = 0, c;
    for (c = 0; c < 1 << 5; c++) {
        panel.lut[i++] = c << 3 | c >> 2;
    }
    for (c = 0; c < 1 << 6; c++) {
        panel.lut[i++] = c << 2 | c >> 4;
    }
    for (c = 0; c < 1 << 5; c++) {
        panel.lut[i++] = c << 3 | c >> 2;
    }
}

void panel_reset(void) {
    memset(&panel, 0, sizeof(panel));
    panel_init_luts();

    panel_hw_reset();
}

bool panel_save(FILE *image) {
    return fwrite(&panel, offsetof(panel_state_t, lut), 1, image) == 1;
}

bool panel_restore(FILE *image) {
    if (fread(&panel, offsetof(panel_state_t, lut), 1, image) == 1) {
        panel_init_luts();
        return true;
    }
    return false;
}

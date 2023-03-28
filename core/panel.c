#include "panel.h"
#include "bus.h"
#include "schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

panel_state_t panel;

static const panel_params_t panel_reset_params = {
    .CASET = { .XS = 0, .XE = PANEL_LAST_COL },
    .RASET = { .YS = 0, .YE = PANEL_LAST_ROW },
    .PTLAR = { .PSL = 0, .PEL = PANEL_LAST_ROW },
    .VSCRDEF = { .TFA = 0, .VSA = PANEL_NUM_ROWS, .BFA = 0 },
    .VSCRSADD = { .VSP = 0 },
    .TESCAN = { .N = 0 },
    .MADCTL = { .MH = 0, .RGB = 0, .ML = 0, .MV = 0, .MX = 0, .MY = 0 },
    .GAMSET = { .GC = 1 },
    .COLMOD = { .MCU = 6, .RGB = 6 },
    .DISBV = { .DBV = 0 },
    .CTRLD = { .BL = 0, .DD = 0, .BCTRL = 0 },
    .CACE = { .C = 0, .CE = 0, .CECTRL = 0 },
    .CABCMB = { .CMB = 0 },
    .RAMCTRL = { .DM = 0, .RM = 0, .MDT = 0, .RIM = 0, .ENDIAN = 0, .EPF = 3 },
    .RGBCTRL = { .EPL = 0, .DPL = 0, .HSPL = 0, .VSPL = 0, .RCM = 2, .WO = 0, .VBP = 2, .HBP = 20 },
    .PORCTRL = { .BPA = 12, .FPA = 12, .PSEN = 0, .FPB = 3, .BPB = 3, .FPC = 3, .BPC = 3 },
    .FRCTRL1 = { .DIV = 0, .FRSEN = 0, .RTNB = 15, .NLB = 0, .RTNC = 15, .NLC = 0 },
    .PARCTRL = { .ISC = 0, .PTGISC = 0, .NDL = 0 },
    .GCTRL = { .VGLS = 5, .VGHS = 3 },
    .GTADJ = { .PAD_2A = 0x2A, .PAD_2B = 0x2B, .GTA = 34, .GOF = 5, .GOFR = 7 },
    .DGMEN = { .DGMEN = 0 },
    .VCOMS = { .VCOMS = 32 },
    .POWSAVE = { .IS = 1, .NS = 1 },
    .DLPOFFSAVE = { .DOFSAVE = 1 },
    .LCMCTRL = { .XGS = 0, .XMV = 0, .XMH = 1, .XMX = 1, .XINV = 0, .XBGR = 1, .XMY = 0 },
    .IDSET = { .ID1 = 0x85, .ID2 = 0x85, .ID3 = 0x52 },
    .VDVVRHEN = { .CMDEN = 1, .PAD_FF = 0xFF },
    .VRHSET = { .VRHS = 11 },
    .VDVSET = { .VDVS = 32 },
    .VCMOFSET = { .VCMOFS = 32 },
    .FRCTRL2 = { .RTNA = 15, .NLA = 0 },
    .CABCCTRL = { .PWMPOL = 0, .PWMFIX = 0, .DPOFPWM = 0, .LEDONREV = 0 },
    .REGSEL1 = { .PAD_08 = 0x08 },
    .REGSEL2 = { .PAD_0F = 0x0F },
    .PWMFRSEL = { .CLK = 2, .CS = 0 },
    .PWCTRL1 = { .PAD_A4 = 0xA4, .VDS = 1, .AVCL = 2, .AVDD = 2 },
    .VAPVANEN = { .PAD_4C = 0x00 },
    .CMD2EN = { .PAD_5A = 0x5A, .PAD_69 = 0x69, .PAD_02 = 0x02, .EN = 0 },
    .GATECTRL = { .NL = PANEL_NUM_ROWS / 8 - 1, .SCN = 0, .GS = 0, .SM = 0, .TMG = 1 },
    .SPI2EN = { .SPIRD = 0, .SPI2EN = 0 },
    .PWCTRL2 = { .STP14CK = 3, .SBCLK = 1 },
    .EQCTRL = { .SEQ = 17, .SPRET = 17, .GEQ = 8 },
    .PROMCTRL = { .PAD_01 = 0x00 },
    .PROMEN = { .PAD_5A = 0x00, .PAD_69 = 0x00, .PAD_EE = 0x00, .PROMEN = 0 },
    .NVMSET = { .ADD = 0x00, .D = 0x00 },
    .PROMACT = { .PAD_29 = 0x00, .PAD_A5 = 0x00 },
    .PVGAMCTRL = { .V0 = 0, .V63 = 7, .V1 = 44, .V2 = 46, .V4 = 21, .V6 = 16, .V13 = 9, .J0 = 0, .V20 = 72, .V27 = 3,
                   .V36 = 3, .V43 = 83, .V50 = 11, .J1 = 0, .V57 = 25, .V59 = 24, .V61 = 32, .V62 = 37 },
    .NVGAMCTRL = { .V0 = 0, .V63 = 7, .V1 = 44, .V2 = 46, .V4 = 21, .V6 = 16, .V13 = 9, .J0 = 0, .V20 = 72, .V27 = 3,
                   .V36 = 3, .V43 = 83, .V50 = 11, .J1 = 0, .V57 = 25, .V59 = 24, .V61 = 32, .V62 = 37 },
    .DGMLUTR = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
                 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
                 0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
                 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC },
    .DGMLUTB = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
                 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
                 0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
                 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC }
};

static bool panel_row_scan_reverse(void) {
    return panel.params.MADCTL.ML ^ panel.params.GATECTRL.SCN;
}

static bool panel_col_scan_reverse(void) {
    return panel.params.MADCTL.MH ^ panel.params.LCMCTRL.XMH;
}

static bool panel_row_addr_reverse(void) {
    return panel.params.MADCTL.MY ^ panel.params.LCMCTRL.XMY;
}

static bool panel_col_addr_reverse(void) {
    return panel.params.MADCTL.MX ^ panel.params.LCMCTRL.XMX;
}

static bool panel_row_col_addr_swap(void) {
    return panel.params.MADCTL.MV ^ panel.params.LCMCTRL.XMV;
}

static bool panel_bgr_enabled(void) {
    return panel.params.MADCTL.RGB ^ panel.params.LCMCTRL.XBGR;
}

static bool panel_inv_enabled(void) {
    return panel.invert ^ panel.params.LCMCTRL.XINV;
}

static bool panel_start_line(uint16_t row) {
    panel.lastScanTick = (panel.linesPerFrame - (int16_t)row) * panel.ticksPerLine;
    if (unlikely(row >= PANEL_NUM_ROWS)) {
        panel.col = PANEL_NUM_COLS;
        if (likely(row == panel.linesPerFrame)) {
            assert(panel.nextLineTick == 0);
            return false;
        }
        panel.nextLineTick = panel.lastScanTick - panel.ticksPerLine;
        panel.row = row;
        return true;
    }
    panel.nextLineTick = panel.lastScanTick - panel.ticksPerLine;
    panel.row = panel.dstRow = panel.srcRow = row;
    /* Partial mode */
    if (unlikely(panel.mode & PANEL_MODE_PARTIAL) &&
        panel.partialStart > panel.partialEnd ?
        panel.partialStart > row && row > panel.partialEnd :
        panel.partialStart > row || row > panel.partialEnd) {
        panel.mode |= PANEL_MODE_BLANK;
    } else {
        panel.mode &= ~PANEL_MODE_BLANK;
    }
    /* Scroll mode */
    if (unlikely(panel.mode & PANEL_MODE_SCROLL)) {
        uint16_t top = panel.topArea, bot = panel.bottomArea;
        if (row >= top && row <= bot) {
            panel.srcRow += panel.scrollStart - top;
            if (panel.srcRow > bot) {
                panel.srcRow -= (bot + 1 - top);
            }
            panel.srcRow &= 0x1FF;
        }
    }
    /* Interlaced scan mode */
    if (unlikely(panel.params.GATECTRL.SM)) {
        panel.dstRow *= 2;
        if (panel.dstRow >= PANEL_NUM_ROWS) {
            panel.dstRow -= (PANEL_NUM_ROWS - 1);
        }
    }
    if (unlikely(panel_row_scan_reverse())) {
        panel.dstRow = PANEL_LAST_ROW - panel.dstRow;
        panel.srcRow = PANEL_LAST_ROW - panel.srcRow;
    }
    panel.col = -panel.horizBackPorch;
    return true;
}

static bool panel_start_frame() {
    /* Ensure all pixels were scanned, if scheduled vsync is still pending */
    if (sched_active(SCHED_PANEL)) {
        panel_refresh_pixels_until(0);
        sched_clear(SCHED_PANEL);
    }

    panel.mode = panel.pendingMode;
    panel.partialStart = panel.params.PTLAR.PSL;
    panel.partialEnd = panel.params.PTLAR.PEL;
    panel.topArea = panel.params.VSCRDEF.TFA;
    panel.bottomArea = PANEL_LAST_ROW - panel.params.VSCRDEF.BFA;
    panel.scrollStart = panel.params.VSCRSADD.VSP;
    panel.displayMode = panel.params.RAMCTRL.DM;

    uint8_t vertBackPorch, vertFrontPorch, horizBackPorch;
    uint16_t horizFrontPorch;
    if (likely(panel.displayMode == PANEL_DM_RGB)) {
        vertBackPorch = panel.params.RGBCTRL.VBP >= 2 ? panel.params.RGBCTRL.VBP : 2;
        vertFrontPorch = 1;

        horizBackPorch = panel.params.RGBCTRL.HBP >= 6 ? panel.params.RGBCTRL.HBP : 6;
        horizFrontPorch = 0;
    } else {
        if (unlikely(panel.params.PORCTRL.PSEN) && (panel.mode & (PANEL_MODE_IDLE | PANEL_MODE_PARTIAL))) {
            vertBackPorch = (panel.mode & PANEL_MODE_PARTIAL ? panel.params.PORCTRL.BPC : panel.params.PORCTRL.BPB) * 4;
            vertFrontPorch = (panel.mode & PANEL_MODE_PARTIAL ? panel.params.PORCTRL.FPC : panel.params.PORCTRL.FPB) * 4;
        } else {
            vertBackPorch = panel.params.PORCTRL.BPA;
            vertFrontPorch = panel.params.PORCTRL.FPA;
        }
        if (vertBackPorch < 1) {
            vertBackPorch = 1;
        }
        if (vertFrontPorch < 1) {
            vertFrontPorch = 1;
        }

        /* 10 additional clocks are divided between back and front porch, divide these into 6 and 4 arbitrarily */
        horizBackPorch = 6;
        if (unlikely(panel.params.FRCTRL1.FRSEN) && (panel.mode & (PANEL_MODE_IDLE | PANEL_MODE_PARTIAL))) {
            horizFrontPorch = panel.mode & PANEL_MODE_PARTIAL ? panel.params.FRCTRL1.RTNC : panel.params.FRCTRL1.RTNB;
        } else {
            horizFrontPorch = panel.params.FRCTRL2.RTNA;
        }
        horizFrontPorch = horizFrontPorch * 16 + 4;
    }

    panel.ticksPerLine = PANEL_NUM_COLS + horizFrontPorch;
    panel.linesPerFrame = PANEL_NUM_ROWS + vertFrontPorch;
    panel.horizBackPorch = horizBackPorch;
    return panel_start_line(-vertBackPorch);
}

bool panel_hsync(void) {
    assert(panel.displayMode == PANEL_DM_RGB);
    return panel_start_line(panel.row + 1);
}

static void panel_reset_mregs(void) {
    if (likely(panel_row_col_addr_swap())) {
        panel.rowReg = panel.params.CASET.XS;
        panel.colReg = panel.params.RASET.YS;
    } else {
        panel.rowReg = panel.params.RASET.YS;
        panel.colReg = panel.params.CASET.XS;
    }
}

void panel_vsync(void) {
    if (likely(panel.params.RAMCTRL.RM)) {
        /* RAM access from RGB interface resets memory registers on vsync */
        panel_reset_mregs();
    }

    /* Accept vsync only if already in RGB interface mode or frame has finished scanning */
    if (likely(panel.displayMode == PANEL_DM_RGB) ||
        likely(panel.row >= PANEL_NUM_ROWS && panel.row <= panel.linesPerFrame)) {
        /* If new display mode is RGB interface, simply start the frame */
        if (likely(panel.params.RAMCTRL.DM == PANEL_DM_RGB)) {
            panel_start_frame();
        /* If new display mode is vsync interface, start the frame and schedule */
        } else if (panel.params.RAMCTRL.DM == PANEL_DM_VSYNC) {
            panel_start_frame();
            sched_repeat_relative(SCHED_PANEL, SCHED_LCD, 0, panel.lastScanTick);
        }
    }
}

static void panel_event(enum sched_item_id id) {
    /* Ensure all pixels were scanned */
    panel_refresh_pixels_until(0);

    /* If the new display mode is MCU, start the next frame now */
    if (panel.params.RAMCTRL.DM == PANEL_DM_MCU) {
        panel_start_frame();
        sched_repeat(SCHED_PANEL, panel.lastScanTick);
    }
}

void panel_refresh_pixels_until(uint32_t currTick) {
    while (unlikely(currTick < panel.nextLineTick)) {
        panel_refresh_pixels(panel.ticksPerLine);
        panel_start_line(panel.row + 1);
    }
    if (likely(panel.lastScanTick > currTick)) {
        uint16_t count = panel.lastScanTick - currTick;
        panel.lastScanTick = currTick;
        panel_refresh_pixels(count);
    }
}

void panel_refresh_pixels(uint16_t count) {
    uint16_t col = panel.col;
    /* Check for back or front porches */
    if (unlikely(col >= PANEL_NUM_COLS)) {
        /* Return if front porch */
        if (likely(col == PANEL_NUM_COLS)) {
            return;
        }
        /* Consume cycles from back porch */
        if (col <= (uint16_t)-count) {
            panel.col = col + count;
            return;
        }
        /* Clip to start of line */
        count += col;
        col = 0;
    }
    /* Update column */
    panel.col = col + count;
    /* Clip to end of line */
    if (unlikely(panel.col > PANEL_NUM_COLS)) {
        panel.col = PANEL_NUM_COLS;
        count = PANEL_NUM_COLS - col;
    }
    /* Apply scan direction, but still internally scan forward */
    if (unlikely(panel_col_scan_reverse())) {
        col = PANEL_NUM_COLS - panel.col;
    }

    uint8_t (*dstPixel)[4] = &panel.display[col][panel.dstRow];
    uint8_t idleShift = (panel.mode & PANEL_MODE_IDLE) ? 7 : 0;
    if (unlikely(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF | PANEL_MODE_BLANK))) {
        while (count--) {
            memset(dstPixel, ~0, sizeof(*dstPixel));
            dstPixel += PANEL_NUM_ROWS;
        }
    } else if (unlikely(panel.srcRow > PANEL_LAST_ROW)) {
        while (count--) {
            (*dstPixel)[PANEL_RED] = (int8_t)bus_rand() >> idleShift;
            (*dstPixel)[PANEL_GREEN] = (int8_t)bus_rand() >> idleShift;
            (*dstPixel)[PANEL_BLUE] = (int8_t)bus_rand() >> idleShift;
            (*dstPixel)[PANEL_ALPHA] = ~0;
            dstPixel += PANEL_NUM_ROWS;
        }
    } else {
        uint8_t (*srcPixel)[3] = &panel.frame[panel.srcRow][col];
        uint8_t invMask = panel_inv_enabled() ? 0xFF : 0;
        uint8_t redIndex = panel_bgr_enabled() ? PANEL_BLUE : PANEL_RED;
        uint8_t blueIndex = redIndex ^ (PANEL_RED ^ PANEL_BLUE);
        while (count--) {
            (*dstPixel)[redIndex] = (int8_t)((*srcPixel)[PANEL_RED] ^ invMask) >> idleShift;
            (*dstPixel)[PANEL_GREEN] = (int8_t)((*srcPixel)[PANEL_GREEN] ^ invMask) >> idleShift;
            (*dstPixel)[blueIndex] = (int8_t)((*srcPixel)[PANEL_BLUE] ^ invMask) >> idleShift;
            (*dstPixel)[PANEL_ALPHA] = ~0;
            srcPixel++;
            dstPixel += PANEL_NUM_ROWS;
        }
    }
}

static void panel_update_pixel(uint8_t red, uint8_t green, uint8_t blue) {
    if (likely(panel.rowReg < 320 && panel.colReg < 240)) {
        uint8_t *pixel = panel.frame[panel.rowReg][panel.colReg];
        pixel[PANEL_RED] = red;
        pixel[PANEL_GREEN] = green;
        pixel[PANEL_BLUE] = blue;
    }
    if (likely(panel_row_col_addr_swap())) {
        if (unlikely(panel.rowReg == panel.params.CASET.XE)) {
            if (unlikely(panel.colReg == panel.params.RASET.YE && panel.params.RASET.YS <= panel.params.RASET.YE)) {
                panel.rowReg = panel.colReg = ~0;
            } else {
                panel.rowReg = panel.params.CASET.XS;
                panel.colReg = (panel.colReg + (panel_col_addr_reverse() ? -1 : 1)) & 0xFF;
            }
        } else if (panel.rowReg < 0x200) {
            panel.rowReg = (panel.rowReg + (panel_row_addr_reverse() ? -1 : 1)) & 0x1FF;
        }
    } else {
        if (unlikely(panel.colReg == panel.params.CASET.XE)) {
            if (unlikely(panel.rowReg == panel.params.RASET.YE && panel.params.RASET.YS <= panel.params.RASET.YE)) {
                panel.rowReg = panel.colReg = ~0;
            } else {
                panel.colReg = panel.params.CASET.XS;
                panel.rowReg = (panel.rowReg + (panel_row_addr_reverse() ? -1 : 1)) & 0x1FF;
            }
        } else if (panel.colReg < 0x100) {
            panel.colReg = (panel.colReg + (panel_col_addr_reverse() ? -1 : 1)) & 0xFF;
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

void panel_hw_reset(void) {
    memcpy(&panel.params, &panel_reset_params, sizeof(panel_params_t));
    panel.pendingMode = PANEL_MODE_SLEEP | PANEL_MODE_OFF;
    panel.cmd = panel.paramIter = panel.paramEnd = 0;
    panel.invert = panel.tear = false;
    /* The display mode is reset to MCU interface, so start the frame and schedule it */
    panel_start_frame();
    sched_set(SCHED_PANEL, panel.lastScanTick);
}

static void panel_sw_reset(void) {
    uint8_t madctl, colmod;
    memcpy(&madctl, &panel.params.MADCTL, sizeof(uint8_t));
    memcpy(&colmod, &panel.params.COLMOD, sizeof(uint8_t));
    panel_hw_reset();
    memcpy(&panel.params.MADCTL, &madctl, sizeof(uint8_t));
    memcpy(&panel.params.COLMOD, &colmod, sizeof(uint8_t));
    if (panel.params.MADCTL.MV) {
        panel.params.CASET.XE = PANEL_LAST_ROW;
        panel.params.RASET.YE = PANEL_LAST_COL;
    }
}

static void panel_write_cmd(uint8_t value) {
    panel.cmd = value;
    panel.paramIter = panel.paramEnd = 0;

#define PANEL_CMD_CASE(cmd_num, field)                                      \
        case cmd_num:                                                       \
            panel.paramIter = offsetof(panel_params_t, field);              \
            panel.paramEnd = panel.paramIter + sizeof(panel.params.field);

    switch (panel.cmd) {
        /* Command table 1 */
        case 0x00:
            break;
        case 0x01:
            panel_sw_reset();
            break;
        case 0x10:
            panel.pendingMode |= PANEL_MODE_SLEEP;
            break;
        case 0x11:
            panel.pendingMode &= ~PANEL_MODE_SLEEP;
            break;
        case 0x12:
            panel.pendingMode |= PANEL_MODE_PARTIAL;
            panel.params.VSCRSADD.VSP = 0;
            break;
        case 0x13:
            panel.pendingMode &= ~(PANEL_MODE_PARTIAL | PANEL_MODE_SCROLL);
            panel.params.VSCRSADD.VSP = 0;
            break;
        case 0x20:
            panel.invert = false;
            break;
        case 0x21:
            panel.invert = true;
            break;
        PANEL_CMD_CASE(0x26, GAMSET)
            break;
        case 0x28:
            panel.pendingMode |= PANEL_MODE_OFF;
            break;
        case 0x29:
            panel.pendingMode &= ~PANEL_MODE_OFF;
            break;
        PANEL_CMD_CASE(0x2A, CASET)
            break;
        PANEL_CMD_CASE(0x2B, RASET)
            break;
        case 0x2C:
            panel_reset_mregs();
            break;
        PANEL_CMD_CASE(0x30, PTLAR)
            break;
        PANEL_CMD_CASE(0x33, VSCRDEF)
            break;
        case 0x34:
            panel.tear = false;
            break;
        case 0x35:
            panel.tear = true;
            break;
        PANEL_CMD_CASE(0x36, MADCTL)
            break;
        PANEL_CMD_CASE(0x37, VSCRSADD)
            panel.pendingMode |= PANEL_MODE_SCROLL;
            break;
        case 0x38:
            panel.pendingMode &= ~PANEL_MODE_IDLE;
            break;
        case 0x39:
            panel.pendingMode |= PANEL_MODE_IDLE;
            break;
        PANEL_CMD_CASE(0x3A, COLMOD)
            break;
        PANEL_CMD_CASE(0x44, TESCAN)
            break;
        PANEL_CMD_CASE(0x51, DISBV)
            break;
        PANEL_CMD_CASE(0x53, CTRLD)
            break;
        PANEL_CMD_CASE(0x5E, CABCMB)
            break;
        PANEL_CMD_CASE(0x68, CTRLD)
            break;
        /* Command table 2 */
        PANEL_CMD_CASE(0xB0, RAMCTRL)
            break;
        PANEL_CMD_CASE(0xB1, RGBCTRL)
            break;
        PANEL_CMD_CASE(0xB2, PORCTRL)
            break;
        PANEL_CMD_CASE(0xB3, FRCTRL1)
            break;
        PANEL_CMD_CASE(0xB5, PARCTRL)
            break;
        PANEL_CMD_CASE(0xB7, GCTRL)
            break;
        PANEL_CMD_CASE(0xB8, GTADJ)
            break;
        PANEL_CMD_CASE(0xBA, DGMEN)
            break;
        PANEL_CMD_CASE(0xBB, VCOMS)
            break;
        PANEL_CMD_CASE(0xBC, POWSAVE)
            break;
        PANEL_CMD_CASE(0xBD, DLPOFFSAVE)
            break;
        PANEL_CMD_CASE(0xC0, LCMCTRL)
            break;
        PANEL_CMD_CASE(0xC1, IDSET)
            break;
        PANEL_CMD_CASE(0xC2, VDVVRHEN)
            break;
        PANEL_CMD_CASE(0xC3, VRHSET)
            break;
        PANEL_CMD_CASE(0xC4, VDVSET)
            break;
        PANEL_CMD_CASE(0xC5, VCMOFSET)
            break;
        PANEL_CMD_CASE(0xC6, FRCTRL2)
            break;
        PANEL_CMD_CASE(0xC7, CABCCTRL)
            break;
        PANEL_CMD_CASE(0xC8, REGSEL1)
            break;
        PANEL_CMD_CASE(0xCA, REGSEL2)
            break;
        PANEL_CMD_CASE(0xCC, PWMFRSEL)
            break;
        PANEL_CMD_CASE(0xD0, PWCTRL1)
            break;
        PANEL_CMD_CASE(0xD2, VAPVANEN)
            break;
        PANEL_CMD_CASE(0xDF, CMD2EN)
            break;
        PANEL_CMD_CASE(0xE0, PVGAMCTRL)
            break;
        PANEL_CMD_CASE(0xE1, NVGAMCTRL)
            break;
        PANEL_CMD_CASE(0xE2, DGMLUTR)
            break;
        PANEL_CMD_CASE(0xE3, DGMLUTB)
            break;
        PANEL_CMD_CASE(0xE4, GATECTRL)
            break;
        PANEL_CMD_CASE(0xE7, SPI2EN)
            break;
        PANEL_CMD_CASE(0xE8, PWCTRL2)
            break;
        PANEL_CMD_CASE(0xE9, EQCTRL)
            break;
        PANEL_CMD_CASE(0xEC, PROMCTRL)
            break;
        PANEL_CMD_CASE(0xFA, PROMEN)
            break;
        PANEL_CMD_CASE(0xFC, NVMSET)
            break;
        PANEL_CMD_CASE(0xFE, PROMACT)
            break;
        default:
            break;
    }

#undef PANEL_CMD_CASE
}

static void panel_write_param(uint8_t value) {
    if (likely(panel.paramIter < panel.paramEnd)) {
        uint8_t index = panel.paramIter++;
        /* Swap endianness of word parameters */
        index ^= (index < offsetof(panel_params_t, MADCTL));
        ((uint8_t*)&panel.params)[index] = value;
        if (unlikely(index == offsetof(panel_params_t, RAMCTRL))) {
            /* Handle display mode switch from RGB to MCU */
            if (unlikely(panel.params.RAMCTRL.DM == PANEL_DM_MCU) &&
                panel.displayMode == PANEL_DM_RGB) {
                panel_start_frame();
                sched_set(SCHED_PANEL, panel.lastScanTick);
            }
        }
    } else if (panel.cmd == 0x2C || panel.cmd == 0x3C) {
        if (unlikely(!panel.params.RAMCTRL.RM)) {
            switch (panel.params.COLMOD.MCU) {
                default:
                case 6: /* 18bpp */
                    switch (panel.paramIter++) {
                        case 0:
                            panel.ifBlue = value >> 2;
                            break;
                        case 1:
                            panel.ifGreen = value >> 2;
                            break;
                        case 2:
                            panel.ifRed = value >> 2;
                            panel_update_pixel_18bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                            panel.paramIter = 0;
                            break;
                    }
                    break;
                case 5: /* 16bpp */
                    switch (panel.paramIter++) {
                        case 0:
                            panel.ifBlue = value >> 3;
                            panel.ifGreen = value << 3 & 0x38;
                            break;
                        case 1:
                            panel.ifGreen |= value >> 5;
                            panel.ifRed = value & 0x1F;
                            panel_update_pixel_16bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
                            panel.paramIter = 0;
                            break;
                    }
                    break;
                case 3: /* 12bpp */
                    switch (panel.paramIter++) {
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
                            panel.paramIter = 0;
                            break;
                    }
                    break;
            }
        }
    }
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

    sched.items[SCHED_PANEL].callback.event = panel_event;
    sched.items[SCHED_PANEL].clock = CLOCK_10M;
    sched_clear(SCHED_PANEL);

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

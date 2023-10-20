#include "panel.h"
#include "bus.h"
#include "lcd.h"
#include "schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

panel_state_t panel;

#define GAMMA_PARAMS(j0, j1, v0, v1, v2, v20, v43, v61, v62, v63, v4, v6, v13, v27, v36, v50, v57, v59) \
    { .V0 = 129-v0, .V63 = 23-v63, .V1 = 128-v1, .V2 = 128-v2, .V4 = 57-v4, .V6 = 47-v6, .V13 = 21-v13, \
      .J0 = j0, .V20 = 128-v20, .V27 = 20-v27, .V36 = 11-v36, .V43 = 128-v43, .V50 = 54-v50, .J1 = j1,  \
      .V57 = 44-v57, .V59 = 34-v59, .V61 = 64-v61, .V62 = 64-v62 }

static const panel_params_t panel_reset_params = {
    .CASET = { .XS = 0, .XE = PANEL_LAST_COL },
    .RASET = { .YS = 0, .YE = PANEL_LAST_ROW },
    .PTLAR = { .PSL = 0, .PEL = PANEL_LAST_ROW },
    .VSCRDEF = { .TFA = 0, .VSA = PANEL_NUM_ROWS, .BFA = 0 },
    .VSCRSADD = { .VSP = 0 },
    .TESCAN = { .N = 0 },
    .MADCTL = { .MH = 0, .RGB = 0, .ML = 0, .MV = 0, .MX = 0, .MY = 0 },
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
    .GAMSET = { .GC = 1 },
    .DGMEN = { .DGMEN = 0 },
    .PVGAMCTRL = GAMMA_PARAMS(0, 0,  129, 84, 82, 56, 45, 32, 27, 16,  36, 31, 12,  17, 8,  43, 19, 10),
    .NVGAMCTRL = GAMMA_PARAMS(0, 0,  129, 84, 82, 56, 45, 32, 27, 16,  36, 31, 12,  17, 8,  43, 19, 10),
    .DGMLUTR = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
                 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
                 0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
                 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC },
    .DGMLUTB = { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C,
                 0x40, 0x44, 0x48, 0x4C, 0x50, 0x54, 0x58, 0x5C, 0x60, 0x64, 0x68, 0x6C, 0x70, 0x74, 0x78, 0x7C,
                 0x80, 0x84, 0x88, 0x8C, 0x90, 0x94, 0x98, 0x9C, 0xA0, 0xA4, 0xA8, 0xAC, 0xB0, 0xB4, 0xB8, 0xBC,
                 0xC0, 0xC4, 0xC8, 0xCC, 0xD0, 0xD4, 0xD8, 0xDC, 0xE0, 0xE4, 0xE8, 0xEC, 0xF0, 0xF4, 0xF8, 0xFC }
};

static const panel_gamma_t gamma_presets[4][2] = {
    /* G2.2 */
    { GAMMA_PARAMS(1, 3,  129, 128, 126, 83, 65, 45, 41, 10,  42, 32, 11,  16, 6,  43, 20, 11),
      GAMMA_PARAMS(0, 3,  129, 128, 126, 85, 64, 45, 41, 10,  43, 33, 12,  17, 7,  43, 20, 11) },
    /* G1.8 */
    { GAMMA_PARAMS(0, 0,  129, 128, 108, 65, 51, 34, 31, 16,  39, 32, 11,  16, 7,  43, 22, 12),
      GAMMA_PARAMS(0, 0,  129, 128, 108, 65, 51, 34, 31, 16,  39, 32, 11,  16, 7,  43, 22, 12) },
    /* G2.5 */
    { GAMMA_PARAMS(0, 0,  129, 128, 125, 69, 54, 40, 38, 16,  45, 37, 11,  15, 5,  39, 22, 11),
      GAMMA_PARAMS(0, 0,  129, 128, 125, 69, 54, 40, 38, 16,  45, 37, 11,  15, 5,  39, 22, 11) },
    /* G1.0 */
    { GAMMA_PARAMS(0, 0,  129, 84, 82, 56, 45, 32, 27, 16,  36, 31, 12,  17, 8,  43, 19, 10),
      GAMMA_PARAMS(0, 0,  129, 84, 82, 56, 45, 32, 27, 16,  36, 31, 12,  17, 8,  43, 19, 10) }
};

#undef GAMMA_PARAMS

static const uint8_t epfLut12[4][16] = {
    { 0x00, 0x04, 0x08, 0x0C, 0x10, 0x14, 0x18, 0x1C, 0x20, 0x24, 0x28, 0x2C, 0x30, 0x34, 0x38, 0x3C },
    { 0x03, 0x07, 0x0B, 0x0F, 0x13, 0x17, 0x1B, 0x1F, 0x23, 0x27, 0x2B, 0x2F, 0x33, 0x37, 0x3B, 0x3F },
    { 0x00, 0x04, 0x08, 0x0C, 0x11, 0x15, 0x19, 0x1D, 0x22, 0x26, 0x2A, 0x2E, 0x33, 0x37, 0x3B, 0x3F },
    { 0x00, 0x04, 0x08, 0x0C, 0x11, 0x15, 0x19, 0x1D, 0x22, 0x26, 0x2A, 0x2E, 0x33, 0x37, 0x3B, 0x3F }
};

static const uint8_t epfLut16[4][64] = {
    { 0x00, 0x00, 0x02, 0x02, 0x04, 0x04, 0x06, 0x06, 0x08, 0x08, 0x0A, 0x0A, 0x0C, 0x0C, 0x0E, 0x0E,
      0x10, 0x10, 0x12, 0x12, 0x14, 0x14, 0x16, 0x16, 0x18, 0x18, 0x1A, 0x1A, 0x1C, 0x1C, 0x1E, 0x1E,
      0x20, 0x20, 0x22, 0x22, 0x24, 0x24, 0x26, 0x26, 0x28, 0x28, 0x2A, 0x2A, 0x2C, 0x2C, 0x2E, 0x2E,
      0x30, 0x30, 0x32, 0x32, 0x34, 0x34, 0x36, 0x36, 0x38, 0x38, 0x3A, 0x3A, 0x3C, 0x3C, 0x3E, 0x3E },
    { 0x01, 0x01, 0x03, 0x03, 0x05, 0x05, 0x07, 0x07, 0x09, 0x09, 0x0B, 0x0B, 0x0D, 0x0D, 0x0F, 0x0F,
      0x11, 0x11, 0x13, 0x13, 0x15, 0x15, 0x17, 0x17, 0x19, 0x19, 0x1B, 0x1B, 0x1D, 0x1D, 0x1F, 0x1F,
      0x21, 0x21, 0x23, 0x23, 0x25, 0x25, 0x27, 0x27, 0x29, 0x29, 0x2B, 0x2B, 0x2D, 0x2D, 0x2F, 0x2F,
      0x31, 0x31, 0x33, 0x33, 0x35, 0x35, 0x37, 0x37, 0x39, 0x39, 0x3B, 0x3B, 0x3D, 0x3D, 0x3F, 0x3F },
    { 0x00, 0x00, 0x02, 0x02, 0x04, 0x04, 0x06, 0x06, 0x08, 0x08, 0x0A, 0x0A, 0x0C, 0x0C, 0x0E, 0x0E,
      0x10, 0x10, 0x12, 0x12, 0x14, 0x14, 0x16, 0x16, 0x18, 0x18, 0x1A, 0x1A, 0x1C, 0x1C, 0x1E, 0x1E,
      0x21, 0x21, 0x23, 0x23, 0x25, 0x25, 0x27, 0x27, 0x29, 0x29, 0x2B, 0x2B, 0x2D, 0x2D, 0x2F, 0x2F,
      0x31, 0x31, 0x33, 0x33, 0x35, 0x35, 0x37, 0x37, 0x39, 0x39, 0x3B, 0x3B, 0x3D, 0x3D, 0x3F, 0x3F },
    { 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0A, 0x0B, 0x0C, 0x0D, 0x0E, 0x0F,
      0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1A, 0x1B, 0x1C, 0x1D, 0x1E, 0x1F,
      0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2A, 0x2B, 0x2C, 0x2D, 0x2E, 0x2F,
      0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3A, 0x3B, 0x3C, 0x3D, 0x3E, 0x3F }
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

static void panel_invert_luts(void) {
    for (uint8_t c = 0; c < 32; c++) {
        uint8_t rc = 63 - c;
        for (uint8_t i = PANEL_RED; i <= PANEL_BLUE; i++) {
            uint8_t temp = panel.gammaLut[i][c];
            panel.gammaLut[i][c] = panel.gammaLut[i][rc];
            panel.gammaLut[i][rc] = temp;
        }
    }
}

static void lerp_batch(float curve[64], float a, float b, float scale, const uint8_t *indices, const uint8_t *amounts) {
    float step = (a - b) * scale;
    for (size_t idx = 0; indices[idx] != 0xFF; idx++) {
        curve[indices[idx]] = b + step * amounts[idx];
    }
}

static void lerp_between(float *curve, size_t length) {
    float a = *curve;
    float step = (curve[length] - a) / length;
    while (--length) {
        *(++curve) = (a += step);
    }
}

static float lerp_percent(float a, float b, uint8_t amount) {
    return b + (a - b) * amount * 0.01f;
}

static void panel_generate_gamma_curve(float curve[64], const panel_gamma_t *params) {
    static const uint8_t j0_percents[4][8] = {
        { 50, 50, 86, 71, 57, 43, 29, 14 },
        { 56, 44, 71, 57, 40, 29, 17, 6 },
        { 50, 50, 80, 63, 49, 34, 20, 9 },
        { 60, 42, 66, 49, 34, 23, 14, 6 }
    };
    static const uint8_t j1_percents[4][8] = {
        { 86, 71, 57, 43, 29, 14, 50, 50 },
        { 86, 71, 60, 46, 34, 17, 56, 50 },
        { 86, 77, 63, 46, 31, 14, 47, 50 },
        { 89, 80, 69, 51, 37, 20, 47, 53 }
    };

    static const uint8_t idx_main[] = { 0, 1, 2, 20, 43, 61, 62, 63, 0xFF };
    static const uint8_t idx_2_20[] = { 4, 6, 13, 0xFF };
    static const uint8_t idx_20_43[] = { 27, 36, 0xFF };
    static const uint8_t idx_43_61[] = { 50, 57, 59, 0xFF };
    static const uint8_t idx_6_13[] = { 7, 8, 9, 10, 11, 12, 0xFF };
    static const uint8_t idx_50_57[] = { 51, 52, 53, 54, 55, 56, 0xFF };

#define P(param, base) ((base) - (params->V##param))
    /* Level 1 */
    lerp_batch(curve, 0.0f, 1.0f, 1.0f / 129, idx_main, (uint8_t[]) {
        P(0, 129), P(1, 128), P(2, 128), P(20, 128), P(43, 128), P(61, 64), P(62, 64), P(63, 23)
    });
    /* Level 2 */
    lerp_batch(curve, curve[2], curve[20], 1.0f / 60, idx_2_20, (uint8_t[]) {
        P(4, 57), P(6, 47), P(13, 21)
    });
    lerp_batch(curve, curve[20], curve[43], 1.0f / 25, idx_20_43, (uint8_t[]) {
        P(27, 20), P(36, 11)
    });
    lerp_batch(curve, curve[43], curve[61], 1.0f / 60, idx_43_61, (uint8_t[]) {
        P(50, 54), P(57, 44), P(59, 34)
    });
#undef P

    /* Level 3 */
    const uint8_t *j0 = j0_percents[params->J0];
    curve[3] = lerp_percent(curve[2], curve[4], j0[0]);
    curve[5] = lerp_percent(curve[4], curve[6], j0[1]);
    lerp_batch(curve, curve[6], curve[13], 0.01f, idx_6_13, &j0[2]);

    lerp_between(&curve[13], 7);
    lerp_between(&curve[20], 7);
    lerp_between(&curve[27], 9);
    lerp_between(&curve[36], 7);
    lerp_between(&curve[43], 7);

    const uint8_t *j1 = j1_percents[params->J1];
    lerp_batch(curve, curve[50], curve[57], 0.01f, idx_50_57, &j1[0]);
    curve[58] = lerp_percent(curve[57], curve[59], j1[6]);
    curve[60] = lerp_percent(curve[59], curve[61], j1[7]);
}

static void panel_generate_luts(void) {
    /* Monotonic cubic spline mapping voltage to grayscale level */
    static const struct spline {
        float gamma, a, b, c, d;
    } spline[] = {
        { 0.0000000f,   0.9429345f,  0.9091704f, 0.4518956f, 0.00000000f + 0.5f / 255 },
        { 0.0620155f, -10.4345504f,  1.0846001f, 0.5755403f, 0.03174603f + 0.5f / 255 },
        { 0.1145995f,  11.5803859f, -0.5614705f, 0.6030486f, 0.06349206f + 0.5f / 255 },
        { 0.2843669f,  -8.2473606f,  5.3364470f, 1.4136841f, 0.20634921f + 0.5f / 255 },
        { 0.3488372f,  -6.5355189f,  3.7413179f, 1.9989302f, 0.31746032f + 0.5f / 255 },
        { 0.5000000f, -65.9879198f,  0.7775361f, 2.6820128f, 0.68253968f + 0.5f / 255 },
        { 0.5428295f,   1.1334623f, -7.7011443f, 2.3854784f, 0.79365079f + 0.5f / 255 },
        { 0.6234496f, -74.7865571f, -1.6524155f, 1.1658447f, 0.93650794f + 0.5f / 255 },
        { 0.6821705f,   1.1437826f, -0.8245875f, 0.1981567f, 0.98412698f + 0.5f / 255 },
        { 0.9224806f,   0.0000000f,  0.0000000f, 0.0000000f, 1.00000000f + 0.5f / 255 },
        { 1.0000000f,   0.0000000f,  0.0000000f, 0.0000000f, 1.00000000f + 0.5f / 255 }
    };

    if (!unlikely(panel.gammaDirty)) {
        return;
    }
    panel.gammaDirty = false;

    if (panel.accurateGamma) {
        float gamma_pos[64], gamma_neg[64];
        panel_generate_gamma_curve(gamma_pos, &panel.params.PVGAMCTRL);
        panel_generate_gamma_curve(gamma_neg, &panel.params.NVGAMCTRL);
        const struct spline *piece = &spline[0];
        for (uint8_t c = 0; c < 64; c++) {
            float gamma = (gamma_pos[c] + gamma_neg[c]) * 0.5f;
            assert(gamma >= 0.0f && gamma <= 1.0f);
            if (unlikely(gamma < piece->gamma)) {
                do {
                    piece--;
                } while (gamma < piece->gamma);
            } else {
                while (gamma > piece[1].gamma) {
                    piece++;
                }
            }
            float diff = gamma - piece->gamma;
            float grayscale = ((piece->a * diff + piece->b) * diff + piece->c) * diff + piece->d;
            assert(grayscale >= 0.0f && grayscale < 256.0f / 255.0f);
            panel.gammaLut[PANEL_GREEN][c] = (uint8_t)(grayscale * 255.0f);
        }
    } else {
        for (uint8_t c = 0; c < 64; c++) {
            panel.gammaLut[PANEL_GREEN][c] = c << 2 | c >> 4;
        }
    }

    if (unlikely(panel.mode & PANEL_MODE_IDLE)) {
        memset(&panel.gammaLut[PANEL_GREEN][1], panel.gammaLut[PANEL_GREEN][0], 31);
        memset(&panel.gammaLut[PANEL_GREEN][32], panel.gammaLut[PANEL_GREEN][63], 31);
    }

    if (unlikely(panel.params.DGMEN.DGMEN) && !(panel.mode & PANEL_MODE_IDLE)) {
        for (uint8_t c = 0; c < 64; c += 2) {
            /* Yes, the opposite DGMLUTs are used on purpose, the datasheet seemingly reversed them because of default BGR */
            panel.gammaLut[PANEL_RED][c] = panel.gammaLut[PANEL_RED][c + 1] = panel.gammaLut[PANEL_GREEN][panel.params.DGMLUTB[c] >> 2];
            panel.gammaLut[PANEL_BLUE][c] = panel.gammaLut[PANEL_BLUE][c + 1] = panel.gammaLut[PANEL_GREEN][panel.params.DGMLUTR[c] >> 2];
        }
    } else {
        memcpy(panel.gammaLut[PANEL_RED], panel.gammaLut[PANEL_GREEN], 64);
        memcpy(panel.gammaLut[PANEL_BLUE], panel.gammaLut[PANEL_GREEN], 64);
    }

    if (unlikely(panel_inv_enabled())) {
        panel_invert_luts();
    }
}

static bool panel_start_frame() {
    /* Ensure all pixels were scanned, if scheduled vsync is still pending */
    if (sched_active(SCHED_PANEL)) {
        panel_refresh_pixels_until(0);
        sched_clear(SCHED_PANEL);
    }

    lcd_gui_event();

    if ((panel.mode ^ panel.pendingMode) & PANEL_MODE_IDLE) {
        panel.gammaDirty = true;
    }
    panel.mode = panel.pendingMode;
    panel.partialStart = panel.params.PTLAR.PSL;
    panel.partialEnd = panel.params.PTLAR.PEL;
    panel.topArea = panel.params.VSCRDEF.TFA;
    panel.bottomArea = PANEL_LAST_ROW - panel.params.VSCRDEF.BFA;
    panel.scrollStart = panel.params.VSCRSADD.VSP;
    panel.displayMode = panel.params.RAMCTRL.DM;

    panel_generate_luts();

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

    /* Accept vsync only if the frame has finished scanning */
    if (likely(panel.row >= PANEL_NUM_ROWS && panel.row <= panel.linesPerFrame)) {
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
        sched_repeat(id, panel.lastScanTick);
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
    uint8_t redIndex = panel_bgr_enabled() ? PANEL_BLUE : PANEL_RED;
    uint8_t blueIndex = redIndex ^ (PANEL_RED ^ PANEL_BLUE);
    if (unlikely(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF | PANEL_MODE_BLANK))) {
        while (count--) {
            memset(dstPixel, ~0, sizeof(*dstPixel));
            dstPixel += PANEL_NUM_ROWS;
        }
    } else if (unlikely(panel.srcRow > PANEL_LAST_ROW)) {
        while (count--) {
            (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][bus_rand() % 64];
            (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][bus_rand() % 64];
            (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][bus_rand() % 64];
            (*dstPixel)[PANEL_ALPHA] = ~0;
            dstPixel += PANEL_NUM_ROWS;
        }
    } else {
        uint8_t (*srcPixel)[3] = &panel.frame[panel.srcRow][col];
        while (count--) {
            (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][(*srcPixel)[PANEL_RED]];
            (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][(*srcPixel)[PANEL_GREEN]];
            (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][(*srcPixel)[PANEL_BLUE]];
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

static void panel_update_pixel_18bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 64 && green < 64 && blue < 64);
    panel_update_pixel(red, green, blue);
}

static void panel_update_pixel_16bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 32 && green < 64 && blue < 32);
    const uint8_t *lut = epfLut16[panel.params.RAMCTRL.EPF];
    panel_update_pixel(lut[red << 1 | (green & 1)], green, lut[blue << 1 | (green & 1)]);
}

static void panel_update_pixel_12bpp(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 16 && green < 16 && blue < 16);
    const uint8_t *lut = epfLut12[panel.params.RAMCTRL.EPF];
    panel_update_pixel(lut[red], lut[green], lut[blue]);
}

void panel_update_pixel_rgb(uint8_t red, uint8_t green, uint8_t blue) {
    assert(red < 32 && green < 64 && blue < 32);
    if (unlikely(panel.params.COLMOD.RGB == 5)) {
        panel_update_pixel_16bpp(red, green, blue);
    } else {
        panel_update_pixel_18bpp((red << 1) | (red >> 4), green, (blue << 1) | (blue >> 4));
    }
}

void panel_hw_reset(void) {
    memcpy(&panel.params, &panel_reset_params, sizeof(panel_params_t));
    panel.gammaDirty = true;
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
            if (panel.invert) {
                panel.invert = false;
                panel_invert_luts();
            }
            break;
        case 0x21:
            if (!panel.invert) {
                panel.invert = true;
                panel_invert_luts();
            }
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
        uint8_t oldValue = ((uint8_t*)&panel.params)[index];
        ((uint8_t*)&panel.params)[index] = value;
        if (unlikely(index == offsetof(panel_params_t, RAMCTRL))) {
            /* Handle display mode switch from RGB to MCU */
            if (unlikely(panel.params.RAMCTRL.DM == PANEL_DM_MCU) &&
                panel.displayMode == PANEL_DM_RGB) {
                panel_start_frame();
                sched_set(SCHED_PANEL, panel.lastScanTick);
            }
        } else if (unlikely(index == offsetof(panel_params_t, LCMCTRL))) {
            if ((value ^ oldValue) & 1 << 4) {
                panel_invert_luts();
            }
        } else if (unlikely(index >= offsetof(panel_params_t, GAMSET))) {
            if (unlikely(index == offsetof(panel_params_t, GAMSET))) {
                const panel_gamma_t *gamma_preset;
                switch (panel.params.GAMSET.GC) {
                    case 8:
                        gamma_preset = gamma_presets[3];
                        break;
                    case 4:
                        gamma_preset = gamma_presets[2];
                        break;
                    case 2:
                        gamma_preset = gamma_presets[1];
                        break;
                    case 1:
                    default:
                        gamma_preset = gamma_presets[0];
                        break;
                }
                panel.params.PVGAMCTRL = gamma_preset[0];
                panel.params.NVGAMCTRL = gamma_preset[1];
            }
            panel.gammaDirty = true;
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
                    if (panel.paramIter ^ panel.params.RAMCTRL.ENDIAN) {
                        panel.ifGreen = (panel.ifGreen & 0x38) | value >> 5;
                        panel.ifRed = value & 0x1F;
                    } else {
                        panel.ifBlue = value >> 3;
                        panel.ifGreen = (panel.ifGreen & 7) | (value << 3 & 0x38);
                    }

                    if (!(panel.paramIter ^= 1)) {
                        panel_update_pixel_16bpp(panel.ifRed, panel.ifGreen, panel.ifBlue);
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

void panel_reset(void) {
    memset(&panel, 0, offsetof(panel_state_t, gammaLut));

    sched.items[SCHED_PANEL].callback.event = panel_event;
    sched.items[SCHED_PANEL].clock = CLOCK_10M;
    sched_clear(SCHED_PANEL);

    panel_hw_reset();
}

bool panel_save(FILE *image) {
    return fwrite(&panel, offsetof(panel_state_t, gammaLut), 1, image) == 1;
}

bool panel_restore(FILE *image) {
    if (fread(&panel, offsetof(panel_state_t, gammaLut), 1, image) == 1) {
        panel.gammaDirty = true;
        panel_generate_luts();
        return true;
    }
    return false;
}

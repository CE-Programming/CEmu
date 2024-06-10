#include "panel.h"
#include "backlight.h"
#include "bus.h"
#include "lcd.h"
#include "schedule.h"

#include <assert.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

panel_state_t panel;

static void panel_update_rgb_clock_method(void);
static void panel_clock_pixel_ram_bypass(uint16_t bgr565);
static void panel_reset_mem_ptr(panel_mem_ptr_t *memPtr);

#define GAMMA_PARAMS(j0, j1, v0, v1, v2, v20, v43, v61, v62, v63, v4, v6, v13, v27, v36, v50, v57, v59) \
    { .V0 = 129-v0, .V63 = 23-v63, .V1 = 128-v1, .V2 = 128-v2, .V4 = 57-v4, .V6 = 47-v6, .V13 = 21-v13, \
      .J0 = j0, .V20 = 128-v20, .V27 = 20-v27, .V36 = 11-v36, .V43 = 128-v43, .V50 = 54-v50, .J1 = j1,  \
      .V57 = 44-v57, .V59 = 34-v59, .V61 = 64-v61, .V62 = 64-v62 }

static const panel_params_t panel_reset_params = {
    .PTLAR = { .PSL = 0, .PEL = PANEL_LAST_ROW },
    .VSCRDEF = { .TFA = 0, .VSA = PANEL_NUM_ROWS, .BFA = 0 },
    .VSCRSADD = { .VSP = 0 },
    .TESCAN = { .N = 0 },
    .CASET = {.XS = 0, .XE = PANEL_LAST_COL },
    .RASET = {.YS = 0, .YE = PANEL_LAST_ROW },
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

static inline uint32_t panel_bgr444_epf0(uint16_t bgr444) {
    return (bgr444 << 6 & 0x3C000) | (bgr444 << 4 & 0xF00) | (bgr444 << 2 & 0x3C);
}

static inline uint32_t panel_bgr444_epf1(uint16_t bgr444) {
    return panel_bgr444_epf0(bgr444) + 0x30C3;
}

static inline uint32_t panel_bgr444_epf23(uint16_t bgr444) {
    uint32_t bgr666 = panel_bgr444_epf0(bgr444);
    return bgr666 + (bgr666 >> 4 & 0x30C3);
}

static uint32_t (*const panel_bgr444_epf_funcs[])(uint16_t bgr444) = {
    panel_bgr444_epf0,
    panel_bgr444_epf1,
    panel_bgr444_epf23,
    panel_bgr444_epf23
};

static inline uint32_t panel_bgr565_epf0(uint16_t bgr565) {
    uint32_t bgr666 = bgr565 + (bgr565 & 0xF800);
    return bgr666 << 1;
}

static inline uint32_t panel_bgr565_epf1(uint16_t bgr565) {
    return panel_bgr565_epf0(bgr565) + 0x1001;
}

static inline uint32_t panel_bgr565_epf2(uint16_t bgr565) {
    uint32_t bgr666 = panel_bgr565_epf0(bgr565);
    return bgr666 + (bgr666 >> 5 & 0x1001);
}

static inline uint32_t panel_bgr565_epf3(uint16_t bgr565) {
    return panel_bgr565_epf0(bgr565) + (bgr565 & 0x20 ? 0x1001 : 0);
}

static uint32_t (*const panel_bgr565_epf_funcs[])(uint16_t bgr565) = {
    panel_bgr565_epf0,
    panel_bgr565_epf1,
    panel_bgr565_epf2,
    panel_bgr565_epf3
};

static inline uint32_t panel_col_scan_reverse_mask(void) {
    return -(uint32_t)(panel.params.MADCTL.MH ^ panel.params.LCMCTRL.XMH);
}

static inline uint32_t panel_source_scan_reverse_mask(void) {
    return -(uint32_t)panel.params.GATECTRL.SS;
}

static inline uint32_t panel_row_scan_reverse_mask(void) {
    return -(uint32_t)panel.params.MADCTL.ML;
}

static inline uint32_t panel_gate_scan_reverse_mask(void) {
    return -(uint32_t)(panel.params.GATECTRL.GS ^ panel.params.LCMCTRL.XGS);
}

static inline uint32_t panel_row_addr_reverse_mask(void) {
    return -(uint32_t)(panel.params.MADCTL.MY ^ panel.params.LCMCTRL.XMY);
}

static inline uint32_t panel_col_addr_reverse_mask(void) {
    return -(uint32_t)(panel.params.MADCTL.MX ^ panel.params.LCMCTRL.XMX);
}

static inline bool panel_row_col_addr_swap(void) {
    return panel.params.MADCTL.MV ^ panel.params.LCMCTRL.XMV;
}

static inline bool panel_bgr_enabled(void) {
    return panel.params.MADCTL.RGB ^ panel.params.LCMCTRL.XBGR;
}

static inline bool panel_inv_enabled(void) {
    return panel.invert ^ panel.params.LCMCTRL.XINV;
}

static inline bool panel_ram_bypass_enabled(void) {
    return panel.params.RAMCTRL.RM && panel.params.RGBCTRL.WO;
}

static inline bool panel_hv_mode_enabled(void) {
    return panel.params.RGBCTRL.RCM & 1;
}

static inline int32_t panel_ram_col(void) {
    return panel.col + panel.horizBackPorch - 17;
}

static inline bool panel_next_dm_is_mcu(void) {
    return panel.params.RAMCTRL.DM == PANEL_DM_MCU || panel.params.RAMCTRL.DM == PANEL_DM_RESERVED;
}

static inline size_t panel_rgb_epf(void) {
    size_t epf = panel.params.RAMCTRL.EPF;
    if (panel.params.COLMOD.RGB != 5) {
        epf = 2;
    }
    return epf;
}

static inline uint32_t panel_reverse_addr(uint32_t addr, uint32_t upperBound, uint32_t dirMask) {
    assert(dirMask == 0 || dirMask == ~0);
    return (addr ^ dirMask) + (upperBound & dirMask);
}

static inline void panel_buffer_pixel_chunk(uint8_t (* restrict srcPixel)[3], uint8_t (* restrict dstPixel)[3], size_t redIndex) {
    size_t blueIndex = PANEL_RED + PANEL_BLUE - redIndex;
    (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][(*srcPixel)[PANEL_RED]];
    (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][(*srcPixel)[PANEL_GREEN]];
    (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][(*srcPixel)[PANEL_BLUE]];
    dstPixel += PANEL_NUM_COLS / 2;
    srcPixel += PANEL_NUM_COLS / 2;
    (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][(*srcPixel)[PANEL_RED]];
    (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][(*srcPixel)[PANEL_GREEN]];
    (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][(*srcPixel)[PANEL_BLUE]];
}

static void panel_buffer_pixels(void) {
    int32_t nextRamCol = panel.nextRamCol;
    int32_t ramCol = panel_ram_col();
    if (ramCol > PANEL_NUM_COLS) {
        ramCol = PANEL_NUM_COLS;
    }
    /* If next RAM access column hasn't been reached, return */
    if (nextRamCol >= ramCol) {
        return;
    }
    size_t redIndex = panel_bgr_enabled() ? PANEL_BLUE : PANEL_RED;
    uint8_t (*srcPixel)[3] = &panel.frame[panel.srcRow][nextRamCol >> 1];
    uint8_t (*dstPixel)[3] = &panel.lineBuffers[panel.currLineBuffer][nextRamCol >> 1];
    if (panel.srcRow < 300) {
        /* If the next read column is odd, read offsets 1, 3, 5 in the group of 6 */
        if (nextRamCol & 2) {
            panel_buffer_pixel_chunk(srcPixel, dstPixel, redIndex);
            panel_buffer_pixel_chunk(srcPixel + 2, dstPixel + 2, redIndex);
            panel_buffer_pixel_chunk(srcPixel + 4, dstPixel + 4, redIndex);
            srcPixel += 5;
            dstPixel += 5;
            nextRamCol += 10;
        }
        /* Read as many full groups of 6 as possible */
        while (nextRamCol + 2 < ramCol) {
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            nextRamCol += 12;
        }
        /* The next read column is even, if we reached it then read offsets 0, 2, 4 */
        if (nextRamCol < ramCol) {
            panel_buffer_pixel_chunk(srcPixel, dstPixel, redIndex);
            panel_buffer_pixel_chunk(srcPixel + 2, dstPixel + 2, redIndex);
            panel_buffer_pixel_chunk(srcPixel + 4, dstPixel + 4, redIndex);
            nextRamCol += 2;
        }
    } else {
        /* Read every other clock */
        do {
            panel_buffer_pixel_chunk(srcPixel++, dstPixel++, redIndex);
            nextRamCol += 2;
        } while (nextRamCol < ramCol);
    }
    panel.nextRamCol = nextRamCol;
}

static bool panel_start_line(uint16_t row) {
    /* Buffer any pixels not yet buffered from the previous line */
    panel_buffer_pixels();

    /* Copy from the line buffer to the previous line */
    if (likely(panel.dstRow < PANEL_NUM_ROWS)) {
        if (panel_ram_col() >= PANEL_NUM_COLS || panel.clock_pixel == panel_clock_pixel_ram_bypass || panel.row == 0) {
            panel.currLineBuffer = !panel.currLineBuffer;
        }
        uint32_t scanReverse = panel_source_scan_reverse_mask();
        uint8_t (*dstPixel)[4] = &panel.display[scanReverse & PANEL_LAST_COL][panel.dstRow];
        uint8_t (*srcPixel)[3] = &panel.lineBuffers[!panel.currLineBuffer][0];
        int32_t dstDelta = (scanReverse * 2 + 1) * PANEL_NUM_ROWS;
        uint8_t count = PANEL_NUM_COLS;
        do {
            memcpy(dstPixel, srcPixel++, 3);
            dstPixel += dstDelta;
        } while (--count);
    }

    panel.col = -panel.horizBackPorch;
    panel.row = panel.dstRow = panel.srcRow = row;
    panel_update_rgb_clock_method();
    panel.lineStartTick -= (panel.horizBackPorch + panel.ticksPerLine);
    if (unlikely(row >= PANEL_NUM_ROWS)) {
        panel.nextRamCol = PANEL_NUM_COLS;
        if (likely(row == panel.linesPerFrame)) {
            panel.lineStartTick = -panel.horizBackPorch;
            panel.row--;
            return false;
        }
        return true;
    }
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
            panel.srcRow &= PANEL_ADDR_MASK;
        }
    }
    /* Interlaced scan mode */
    if (unlikely(panel.params.GATECTRL.SM)) {
        panel.dstRow *= 2;
        if (panel.dstRow >= PANEL_NUM_ROWS) {
            panel.dstRow -= (PANEL_NUM_ROWS - 1);
        }
    }
    uint32_t rowDirMask = panel_row_scan_reverse_mask();
    uint32_t gateDirMask = panel_gate_scan_reverse_mask();
    panel.srcRow = panel_reverse_addr(panel.srcRow, PANEL_NUM_ROWS, rowDirMask);
    panel.dstRow = panel_reverse_addr(panel.dstRow, PANEL_NUM_ROWS, rowDirMask ^ gateDirMask);

    if (unlikely(panel.clock_pixel == panel_clock_pixel_ram_bypass)) {
        panel.nextRamCol = PANEL_NUM_COLS;
    } else if (unlikely(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF | PANEL_MODE_BLANK))) {
        memset(&panel.lineBuffers[panel.currLineBuffer], panel.blankLevel, sizeof(panel.lineBuffers[0]));
        panel.nextRamCol = PANEL_NUM_COLS;
    } else if (unlikely(panel.srcRow > PANEL_LAST_ROW)) {
        uint8_t (*dstPixel)[3] = &panel.lineBuffers[panel.currLineBuffer][0];
        size_t redIndex = panel_bgr_enabled() ? PANEL_BLUE : PANEL_RED;
        size_t blueIndex = PANEL_RED + PANEL_BLUE - redIndex;
        uint8_t count = PANEL_NUM_COLS;
        do {
            (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][bus_rand() % 64];
            (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][bus_rand() % 64];
            (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][bus_rand() % 64];
            dstPixel++;
        } while (--count);
        panel.nextRamCol = PANEL_NUM_COLS;
    } else {
        panel.nextRamCol = 0;
    }

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

    if (unlikely(panel.gammaDirty)) {
        panel.gammaDirty = false;

        float backlightFactor = backlight.factor < 1.0f ? backlight.factor : 1.0f;
        if (panel.accurateGamma) {
            backlightFactor *= 255.0f;
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
                panel.gammaLut[PANEL_GREEN][c] = (uint8_t)(grayscale * backlightFactor);
            }
        } else {
            backlightFactor *= 4.0625f;
            for (uint8_t c = 0; c < 64; c++) {
                panel.gammaLut[PANEL_GREEN][c] = (uint8_t)(c * backlightFactor);
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

    bool blankLevel = panel_inv_enabled() ^ (!(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF)) && panel.params.PARCTRL.NDL);
    panel.blankLevel = panel.gammaLut[PANEL_GREEN][blankLevel ? 0 : 63];
}

static uint32_t panel_start_frame() {
    /* Ensure all pixels were scanned, if scheduled vsync is still pending */
    if (sched_active(SCHED_PANEL)) {
        panel_scan_until(0);
        sched_clear(SCHED_PANEL);
    }

    lcd_gui_event();

    if ((panel.mode ^ panel.pendingMode) & PANEL_MODE_IDLE) {
        panel.gammaDirty = true;
    }
    panel.mode = panel.pendingMode;
    panel.partialStart = panel.params.PTLAR.PSL & PANEL_ADDR_MASK;
    panel.partialEnd = panel.params.PTLAR.PEL & PANEL_ADDR_MASK;
    panel.topArea = panel.params.VSCRDEF.TFA & PANEL_ADDR_MASK;
    panel.bottomArea = PANEL_LAST_ROW - (panel.params.VSCRDEF.BFA & PANEL_ADDR_MASK);
    panel.scrollStart = panel.params.VSCRSADD.VSP & PANEL_ADDR_MASK;
    panel.displayMode = panel.params.RAMCTRL.DM != PANEL_DM_RESERVED ? panel.params.RAMCTRL.DM : PANEL_DM_MCU;

    panel_generate_luts();

    uint16_t vertBackPorch, vertFrontPorch, horizBackPorch, horizFrontPorch;
    if (likely(panel.displayMode == PANEL_DM_RGB)) {
        if (unlikely(panel_hv_mode_enabled())) {
            vertBackPorch = panel.params.RGBCTRL.VBP;
            horizBackPorch = panel.params.RGBCTRL.HBP;
        } else {
            vertBackPorch = lcd.VSW + lcd.VBP + !panel.params.RGBCTRL.WO;
            horizBackPorch = lcd.HSW + lcd.HBP;
        }
        vertFrontPorch = 1;
        horizFrontPorch = 0;
    } else {
        if (unlikely(panel.params.PORCTRL.PSEN) && (panel.mode & (PANEL_MODE_IDLE | PANEL_MODE_PARTIAL))) {
            vertBackPorch = (panel.mode & PANEL_MODE_PARTIAL ? panel.params.PORCTRL.BPC : panel.params.PORCTRL.BPB) * 4;
            vertFrontPorch = (panel.mode & PANEL_MODE_PARTIAL ? panel.params.PORCTRL.FPC : panel.params.PORCTRL.FPB) * 4;
        } else {
            vertBackPorch = panel.params.PORCTRL.BPA;
            vertFrontPorch = panel.params.PORCTRL.FPA;
        }
        if (vertFrontPorch < 1 || panel.displayMode == PANEL_DM_VSYNC) {
            vertFrontPorch = 1;
        }

        /* Choose a feasible back porch to match the total clocks per line */
        horizBackPorch = 10;
        if (unlikely(panel.params.FRCTRL1.FRSEN) && (panel.mode & (PANEL_MODE_IDLE | PANEL_MODE_PARTIAL))) {
            horizFrontPorch = panel.mode & PANEL_MODE_PARTIAL ? panel.params.FRCTRL1.RTNC : panel.params.FRCTRL1.RTNB;
        } else {
            horizFrontPorch = panel.params.FRCTRL2.RTNA;
        }
        horizFrontPorch = horizFrontPorch * 16;
    }
    if (vertBackPorch < 1) {
        vertBackPorch = 1;
    }

    panel.ticksPerLine = PANEL_NUM_COLS + horizFrontPorch;
    panel.linesPerFrame = PANEL_NUM_ROWS + vertFrontPorch;
    panel.horizBackPorch = horizBackPorch;
    uint32_t ticksPerFrame = (horizBackPorch + panel.ticksPerLine) * (vertBackPorch + panel.linesPerFrame);
    panel.lineStartTick = ticksPerFrame + panel.ticksPerLine;
    panel_start_line(-vertBackPorch);
    return ticksPerFrame;
}

bool panel_hsync(void) {
    if (likely(panel.displayMode == PANEL_DM_RGB)) {
        return panel_start_line(panel.row + 1);
    }
    return panel_hv_mode_enabled();
}

void panel_vsync(void) {
    if (likely(panel.params.RAMCTRL.RM && panel.params.RAMCTRL.WEMODE1)) {
        /* Frame memory pointer for RGB interface resets on vsync */
        panel_reset_mem_ptr(&panel.rgbMemPtr);
        panel.windowFullRgb = false;
        panel_update_rgb_clock_method();
    }

    /* Accept vsync only if the frame has finished scanning */
    if (likely((int16_t)panel.row >= PANEL_NUM_ROWS)) {
        /* If new display mode is RGB interface, simply start the frame */
        if (likely(panel.params.RAMCTRL.DM == PANEL_DM_RGB)) {
            panel_start_frame();
        /* If new display mode is vsync interface, start the frame and schedule */
        } else if (panel.params.RAMCTRL.DM == PANEL_DM_VSYNC) {
            sched_repeat_relative(SCHED_PANEL, SCHED_LCD, 0, panel_start_frame());
        }
    }
}

static void panel_event(enum sched_item_id id) {
    /* Ensure all pixels were scanned */
    panel_scan_until(0);

    /* If the new display mode is MCU, start the next frame now */
    if (panel_next_dm_is_mcu()) {
        sched_repeat(id, panel_start_frame());
    }
}

static void panel_spi_catchup(void) {
    if (sched_active(SCHED_PANEL)) {
        panel_scan_until(sched_ticks_remaining_relative(SCHED_PANEL, SCHED_SPI, 0));
    }
}

void panel_scan_until(uint32_t currTick) {
    int32_t col = panel.lineStartTick - currTick;
    while (unlikely(col >= panel.ticksPerLine)) {
        panel.col = panel.ticksPerLine;
        panel_start_line(panel.row + 1);
        col = panel.lineStartTick - currTick;
    }
    panel.col = col;
}

static bool panel_next_y_addr(panel_mem_ptr_t *memPtr) {
    if (likely(memPtr->yAddr != panel.windowEnd.yAddr)) {
        memPtr->yAddr = (memPtr->yAddr + panel.yDir) & PANEL_ADDR_MASK;
        return true;
    }
    if (panel.params.RAMCTRL.RM && panel.params.RAMCTRL.WEMODE1) {
        panel.windowFullRgb = true;
        panel_update_rgb_clock_method();
        return false;
    }
    if (!panel.params.RAMCTRL.RM && !panel.params.RAMCTRL.WEMODE0) {
        assert(panel.cmd == 0x2C || panel.cmd == 0x3C);
        panel.cmd = 0x00;
        panel.spiMemFlags = PANEL_SPI_WINDOW_FULL;
        return false;
    }
    memPtr->yAddr = panel.windowStart.yAddr;
    return true;
}

static void panel_buffer_catchup(uint8_t col) {
    uint8_t colWithinHalf = (col < 120 ? col : col - 120) << 1;
    if (colWithinHalf >= panel.nextRamCol && colWithinHalf <= panel_ram_col() + 7) {
        panel_buffer_pixels();
    }
}

static inline void panel_write_pixel(uint8_t (*pixel)[3], uint32_t bgr666) {
    assert(bgr666 <= 0x3FFFF);
    (*pixel)[PANEL_RED] = bgr666 & 0x3F;
    (*pixel)[PANEL_GREEN] = bgr666 >> 6 & 0x3F;
    (*pixel)[PANEL_BLUE] = bgr666 >> 12;
}

static void panel_write_pixel_row_oob(panel_mem_ptr_t* memPtr, uint32_t bgr666);

static void panel_write_pixel_row(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    uint16_t row = memPtr->xAddr;
    if (likely(row < PANEL_NUM_ROWS)) {
        if (unlikely(row == panel.srcRow)) {
            panel_buffer_catchup(memPtr->yAddr);
        }
        panel_write_pixel(&panel.windowBasePtr[row * PANEL_NUM_COLS], bgr666);
    }
    if (likely(row != panel.windowEnd.xAddr)) {
        memPtr->xAddr = (row + panel.xDir) & PANEL_ADDR_MASK;
    } else if (likely(panel_next_y_addr(memPtr))) {
        uint8_t col = memPtr->yAddr;
        if (likely(col < PANEL_NUM_COLS)) {
            panel.windowBasePtr = &panel.frame[0][col];
        } else {
            panel.write_pixel = panel_write_pixel_row_oob;
        }
        memPtr->xAddr = panel.windowStart.xAddr;
    }
}

static void panel_write_pixel_row_oob(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    uint16_t row = memPtr->xAddr;
    if (likely(row != panel.windowEnd.xAddr)) {
        memPtr->xAddr = (row + panel.xDir) & PANEL_ADDR_MASK;
    } else if (likely(panel_next_y_addr(memPtr))) {
        uint8_t col = memPtr->yAddr;
        if (unlikely(col < PANEL_NUM_COLS)) {
            panel.windowBasePtr = &panel.frame[0][col];
            panel.write_pixel = panel_write_pixel_row;
        }
        memPtr->xAddr = panel.windowEnd.xAddr;
    }
}

static void panel_write_pixel_col_oob(panel_mem_ptr_t *memPtr, uint32_t bgr666);

static void panel_write_pixel_col(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    uint16_t col = memPtr->xAddr;
    if (likely((uint8_t)col < PANEL_NUM_COLS)) {
        if (unlikely(memPtr->yAddr == panel.srcRow)) {
            panel_buffer_catchup(col);
        }
        panel_write_pixel(&panel.windowBasePtr[(uint8_t)col], bgr666);
    }
    if (likely(col != panel.windowEnd.xAddr)) {
        memPtr->xAddr = (col + panel.xDir) & PANEL_ADDR_MASK;
    } else if (likely(panel_next_y_addr(memPtr))) {
        uint16_t row = memPtr->yAddr;
        if (likely(row < PANEL_NUM_ROWS)) {
            panel.windowBasePtr = &panel.frame[row][0];
        } else {
            panel.write_pixel = panel_write_pixel_col_oob;
        }
        memPtr->xAddr = panel.windowStart.xAddr;
    }
}

static void panel_write_pixel_col_oob(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    uint16_t col = memPtr->xAddr;
    if (likely(col != panel.windowEnd.xAddr)) {
        memPtr->xAddr = (col + panel.xDir) & PANEL_ADDR_MASK;
    } else if (likely(panel_next_y_addr(memPtr))) {
        uint16_t row = memPtr->yAddr;
        if (unlikely(row < PANEL_NUM_ROWS)) {
            panel.windowBasePtr = &panel.frame[row][0];
            panel.write_pixel = panel_write_pixel_col;
        }
        memPtr->xAddr = panel.windowStart.xAddr;
    }
}

static void panel_update_write_pixel_method(panel_mem_ptr_t *memPtr) {
    if (likely(panel_row_col_addr_swap())) {
        uint8_t col = memPtr->yAddr;
        if (likely(col < PANEL_NUM_COLS)) {
            panel.windowBasePtr = &panel.frame[0][col];
            panel.write_pixel = panel_write_pixel_row;
        } else {
            panel.write_pixel = panel_write_pixel_row_oob;
        }
    } else {
        uint16_t row = memPtr->yAddr;
        if (likely(row < PANEL_NUM_ROWS)) {
            panel.windowBasePtr = &panel.frame[row][0];
            panel.write_pixel = panel_write_pixel_col;
        } else {
            panel.write_pixel = panel_write_pixel_col_oob;
        }
    }
}

static void panel_write_pixel_reset_ptr(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    panel.spiMemFlags = 0;
    panel_reset_mem_ptr(memPtr);
    panel.write_pixel(memPtr, bgr666);
}

static void panel_write_pixel_continue_bug(panel_mem_ptr_t *memPtr, uint32_t bgr666) {
    panel.spiMemFlags = 0;
    panel_update_write_pixel_method(memPtr);
    uint16_t xAddr = memPtr->xAddr;
    if (likely(xAddr != panel.windowEnd.xAddr)) {
        panel.write_pixel(memPtr, bgr666);
        return;
    }
    /* Write Continue bug */
    /* If the first write wraps on X, write goes to the new Y address */
    /* If it also wraps on Y and fills the window, the write is discarded */
    if (!likely(panel_next_y_addr(memPtr))) {
        return;
    }
    panel_update_write_pixel_method(memPtr);
    memPtr->xAddr = panel.windowStart.xAddr;
    uint16_t row;
    uint8_t col;
    if (panel_row_col_addr_swap()) {
        row = xAddr;
        col = memPtr->yAddr;
    } else {
        col = xAddr;
        row = memPtr->yAddr;
    }
    if (likely(row < PANEL_NUM_ROWS && col < PANEL_NUM_COLS)) {
        panel_write_pixel(&panel.frame[row][col], bgr666);
    }
}

static inline void panel_write_pixel_18bpp(uint32_t bgr666) {
    panel_spi_catchup();
    panel.write_pixel(&panel.spiMemPtr, bgr666);
}

static inline void panel_write_pixel_16bpp(uint16_t bgr565) {
    panel_spi_catchup();
    panel.write_pixel(&panel.spiMemPtr, panel_bgr565_epf_funcs[panel.params.RAMCTRL.EPF](bgr565));
}

static inline void panel_write_pixel_12bpp(uint16_t bgr444) {
    panel_spi_catchup();
    panel.write_pixel(&panel.spiMemPtr, panel_bgr444_epf_funcs[panel.params.RAMCTRL.EPF](bgr444));
}

static inline void panel_write_pixel_rgb(uint32_t bgr666) {
    panel.write_pixel(&panel.rgbMemPtr, bgr666);
}

static void panel_clock_pixel_ignore(uint16_t bgr565) {
    (void)bgr565;
    panel.col += (panel.displayMode == PANEL_DM_RGB);
}

static void panel_clock_pixel_ram_bypass(uint16_t bgr565) {
    uint32_t col = panel.col;
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    if (likely(col < PANEL_NUM_COLS)) {
        col = panel_reverse_addr(col, PANEL_NUM_COLS, panel_col_scan_reverse_mask());
        uint8_t (*dstPixel)[3] = &panel.lineBuffers[panel.currLineBuffer][col];
        if (unlikely(panel.mode & PANEL_MODE_BLANK)) {
            if (panel.row != 0) {
                memset(dstPixel, panel.blankLevel, sizeof(*dstPixel));
                return;
            }
        } else {
            panel.ifPixel = panel_bgr565_epf_funcs[panel_rgb_epf()](bgr565);
        }
        size_t redIndex = panel_bgr_enabled() ? PANEL_BLUE : PANEL_RED;
        size_t blueIndex = PANEL_RED + PANEL_BLUE - redIndex;
        assert(panel.ifPixel <= 0x3FFFF);
        (*dstPixel)[redIndex] = panel.gammaLut[PANEL_RED][panel.ifPixel & 0x3F];
        (*dstPixel)[PANEL_GREEN] = panel.gammaLut[PANEL_GREEN][panel.ifPixel >> 6 & 0x3F];
        (*dstPixel)[blueIndex] = panel.gammaLut[PANEL_BLUE][panel.ifPixel >> 12];
    }
}

static void panel_clock_pixel_hv(uint16_t bgr565) {
    uint32_t col = panel.col;
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    if (likely(col < PANEL_NUM_COLS)) {
        panel_write_pixel_rgb(panel_bgr565_epf_funcs[panel_rgb_epf()](bgr565));
    }
}

static void panel_clock_pixel_epf0(uint16_t bgr565) {
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    panel_write_pixel_rgb(panel_bgr565_epf0(bgr565));
}

static void panel_clock_pixel_epf1(uint16_t bgr565) {
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    panel_write_pixel_rgb(panel_bgr565_epf1(bgr565));
}

static void panel_clock_pixel_epf2(uint16_t bgr565) {
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    panel_write_pixel_rgb(panel_bgr565_epf2(bgr565));
}

static void panel_clock_pixel_epf3(uint16_t bgr565) {
    panel.col += (panel.displayMode == PANEL_DM_RGB);
    panel_write_pixel_rgb(panel_bgr565_epf3(bgr565));
}

void panel_clock_porch(uint32_t clocks) {
    assert(clocks > 0);
    if (unlikely(panel_hv_mode_enabled()) && panel.clock_pixel != panel_clock_pixel_ignore) {
        do {
            panel.clock_pixel(0);
        } while (--clocks);
    } else if (likely(panel.displayMode == PANEL_DM_RGB)) {
        panel.col += clocks;
    }
}

static void panel_update_rgb_clock_method(void) {
    static void (*const panel_clock_pixel_epf_funcs[])(uint16_t bgr565) = {
        panel_clock_pixel_epf0,
        panel_clock_pixel_epf1,
        panel_clock_pixel_epf2,
        panel_clock_pixel_epf3
    };

    if (!panel.params.RAMCTRL.RM) {
        panel.clock_pixel = panel_clock_pixel_ignore;
    } else if (panel_ram_bypass_enabled()) {
        if (!(panel.mode & (PANEL_MODE_SLEEP | PANEL_MODE_OFF)) && panel.displayMode == PANEL_DM_RGB && panel.row < PANEL_NUM_ROWS) {
            panel.clock_pixel = panel_clock_pixel_ram_bypass;
        } else {
            panel.clock_pixel = panel_clock_pixel_ignore;
        }
    } else if (panel.windowFullRgb) {
        panel.clock_pixel = panel_clock_pixel_ignore;
    } else if (panel_hv_mode_enabled()) {
        panel.clock_pixel = panel.row < PANEL_NUM_ROWS ? panel_clock_pixel_hv : panel_clock_pixel_ignore;
    } else {
        panel.clock_pixel = panel_clock_pixel_epf_funcs[panel_rgb_epf()];
    }
}

static void panel_set_first_spi_write_pixel_method(void) {
    assert(panel.spiMemFlags & PANEL_SPI_FIRST_PIXEL);
    panel.write_pixel = (panel.cmd == 0x2C || (panel.spiMemFlags & PANEL_SPI_AUTO_RESET)) ? panel_write_pixel_reset_ptr : panel_write_pixel_continue_bug;
}

static void panel_update_write_pixel_bounds(void) {
    uint32_t xBound, xDirMask, yBound, yDirMask;
    if (panel_row_col_addr_swap()) {
        xBound = PANEL_NUM_ROWS;
        xDirMask = panel_row_addr_reverse_mask();
        yBound = PANEL_NUM_COLS;
        yDirMask = panel_col_addr_reverse_mask();
    } else {
        xBound = PANEL_NUM_COLS;
        xDirMask = panel_col_addr_reverse_mask();
        yBound = PANEL_NUM_ROWS;
        yDirMask = panel_row_addr_reverse_mask();
    }
    panel.windowStart.xAddr = panel_reverse_addr(panel.params.CASET.XS, xBound, xDirMask) & PANEL_ADDR_MASK;
    panel.windowStart.yAddr = panel_reverse_addr(panel.params.RASET.YS, yBound, yDirMask) & PANEL_ADDR_MASK;
    uint32_t xEnd = panel.params.CASET.XE & PANEL_ADDR_MASK;
    if (xEnd >= xBound) {
        xEnd = xBound - 1;
    }
    panel.windowEnd.xAddr = panel_reverse_addr(xEnd, xBound, xDirMask);
    uint32_t yEnd = panel.params.RASET.YE & PANEL_ADDR_MASK;
    if (yEnd >= yBound) {
        yEnd = yBound - 1;
    }
    panel.windowEnd.yAddr = panel_reverse_addr(yEnd, yBound, yDirMask);
    panel.xDir = (int32_t)xDirMask * 2 + 1;
    panel.yDir = (int32_t)yDirMask * 2 + 1;
    panel_update_write_pixel_method(&panel.rgbMemPtr);
}

static void panel_reset_mem_ptr(panel_mem_ptr_t *memPtr) {
    *memPtr = panel.windowStart;
    panel_update_write_pixel_method(memPtr);
}

void panel_update_clock_rate(void) {
    sched_set_clock(CLOCK_PANEL, panel.clockRate >> panel.params.FRCTRL1.DIV);
}

void panel_hw_reset(void) {
    memcpy(&panel.params, &panel_reset_params, sizeof(panel_params_t));
    panel.gammaDirty = true;
    panel.pendingMode = PANEL_MODE_SLEEP | PANEL_MODE_OFF;
    panel.cmd = panel.paramIter = panel.paramEnd = 0;
    panel.invert = panel.tear = false;
    panel.windowFullRgb = false;
    panel.spiMemFlags = PANEL_SPI_AUTO_RESET;
    panel.spiMemPtr = panel.rgbMemPtr = (panel_mem_ptr_t){ .xAddr = 0, .yAddr = 0 };
    panel_update_write_pixel_bounds();
    /* The display mode is reset to MCU interface, so start the frame and schedule it */
    uint32_t ticksPerFrame = panel_start_frame();
    panel_update_clock_rate();
    sched_set(SCHED_PANEL, ticksPerFrame);
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
    panel_update_write_pixel_bounds();
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
                panel_spi_catchup();
                panel_buffer_pixels();
                panel_invert_luts();
            }
            break;
        case 0x21:
            if (!panel.invert) {
                panel.invert = true;
                panel_spi_catchup();
                panel_buffer_pixels();
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
        case 0x3C:
            if (unlikely(panel.params.RAMCTRL.RM || (!panel.params.RAMCTRL.WEMODE0 && (panel.spiMemFlags & PANEL_SPI_WINDOW_FULL)))) {
                panel.cmd = 0x00;
            } else {
                if (panel.cmd == 0x2C) {
                    panel.spiMemFlags &= ~PANEL_SPI_AUTO_RESET;
                }
                panel.spiMemFlags |= PANEL_SPI_FIRST_PIXEL;
                panel_set_first_spi_write_pixel_method();
            }
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
        if (index >= offsetof(panel_params_t, CASET) && index < offsetof(panel_params_t, COLMOD)) {
            /* CASET, RASET, MADCTL */
            panel_update_write_pixel_bounds();
        } else if (index == offsetof(panel_params_t, COLMOD)) {
            panel_update_rgb_clock_method();
        } else if (index == offsetof(panel_params_t, RAMCTRL)) {
            /* Handle display mode switch from RGB to MCU */
            if (unlikely(panel_next_dm_is_mcu()) &&
                panel.displayMode == PANEL_DM_RGB) {
                sched_set(SCHED_PANEL, panel_start_frame());
            }
            if ((value ^ oldValue) & 1 << 4) {
                if (panel.params.RAMCTRL.RM) {
                    /* Handle frame memory pointer reset when switching to RGB interface */
                    panel_reset_mem_ptr(&panel.rgbMemPtr);;
                    panel.windowFullRgb = false;
                }
                panel_update_rgb_clock_method();
            }
        } else if (index == offsetof(panel_params_t, RAMCTRL) + 1) {
            panel_update_rgb_clock_method();
        } else if (index == offsetof(panel_params_t, RGBCTRL)) {
            if ((value ^ oldValue) & (1 << 7 | 1 << 5)) {
                /* Handle updates to WO and RCM */
                panel_update_rgb_clock_method();
            }
        } else if (index == offsetof(panel_params_t, FRCTRL1)) {
            if ((value ^ oldValue) & 3) {
                panel_update_clock_rate();
            }
        } else if (index == offsetof(panel_params_t, LCMCTRL)) {
            if ((value ^ oldValue) & 1 << 4) {
                panel_spi_catchup();
                panel_buffer_pixels();
                panel_invert_luts();
            }
            panel_update_write_pixel_bounds();
        } else if (index >= offsetof(panel_params_t, GAMSET)) {
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
        switch (panel.params.COLMOD.MCU) {
            default:
            case 6: /* 18bpp */
                value >>= 2;
                switch (panel.paramIter++) {
                    case 0:
                        panel.ifPixel = value << 12;
                        break;
                    case 1:
                        panel.ifPixel |= value << 6;
                        break;
                    case 2:
                        panel.ifPixel |= value;
                        panel_write_pixel_18bpp(panel.ifPixel);
                        panel.paramIter = 0;
                        break;
                }
                break;
            case 5: /* 16bpp */
                if (panel.paramIter ^ panel.params.RAMCTRL.ENDIAN) {
                    panel.ifPixel &= ~0xFF;
                    panel.ifPixel |= value;
                } else {
                    panel.ifPixel &= ~0xFF00;
                    panel.ifPixel |= value << 8;
                }

                if (!(panel.paramIter ^= 1)) {
                    panel_write_pixel_16bpp(panel.ifPixel);
                }
                break;
            case 3: /* 12bpp */
                switch (panel.paramIter++) {
                    case 0:
                        panel.ifPixel = value << 4;
                        break;
                    case 1:
                        panel.ifPixel |= value >> 4;
                        panel_write_pixel_12bpp(panel.ifPixel);
                        panel.ifPixel = value << 8;
                        break;
                    case 2:
                        panel.ifPixel |= value;
                        panel_write_pixel_12bpp(panel.ifPixel);
                        panel.paramIter = 0;
                        break;
                }
                break;
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

void init_panel(void) {
    panel.clockRate = 9800000;
}

void panel_reset(void) {
    memset(&panel, 0, offsetof(panel_state_t, display));
    memset(&panel.display, 0xFF, sizeof(panel.display));

    sched_init_event(SCHED_PANEL, CLOCK_PANEL, panel_event);
    panel_hw_reset();
}

bool panel_save(FILE *image) {
    return fwrite(&panel, offsetof(panel_state_t, gammaLut), 1, image) == 1;
}

bool panel_restore(FILE *image) {
    if (fread(&panel, offsetof(panel_state_t, gammaLut), 1, image) == 1) {
        panel.gammaDirty = true;
        panel_generate_luts();
        panel_update_rgb_clock_method();
        panel_update_write_pixel_bounds();
        if (!panel.params.RAMCTRL.RM) {
            if (panel.spiMemFlags & PANEL_SPI_FIRST_PIXEL) {
                panel_set_first_spi_write_pixel_method();
            } else {
                panel_update_write_pixel_method(&panel.spiMemPtr);
            }
        }
        return true;
    }
    return false;
}

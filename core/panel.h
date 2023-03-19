#ifndef PANEL_H
#define PANEL_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define PANEL_RED 0
#define PANEL_GREEN 1
#define PANEL_BLUE 2
#define PANEL_ALPHA 3
#define PANEL_NUM_ROWS 320
#define PANEL_LAST_ROW 319
#define PANEL_NUM_COLS 240
#define PANEL_LAST_COL 239

enum panel_mode {
    PANEL_MODE_SLEEP   = 1 << 0,
    PANEL_MODE_OFF     = 1 << 1,
    PANEL_MODE_BLANK   = 1 << 2,
    PANEL_MODE_PARTIAL = 1 << 3,
    PANEL_MODE_INVERT  = 1 << 4,
    PANEL_MODE_IDLE    = 1 << 5,
    PANEL_MODE_SCROLL  = 1 << 6,
    PANEL_MODE_IGNORE  = 1 << 7
};

enum panel_mac {
    PANEL_MAC_HRO = 1 << 2,
    PANEL_MAC_BGR = 1 << 3,
    PANEL_MAC_VRO = 1 << 4,
    PANEL_MAC_RCX = 1 << 5,
    PANEL_MAC_CAO = 1 << 6,
    PANEL_MAC_RAO = 1 << 7
};

enum panel_ic {
    PANEL_IC_CTRL_SYNC   = 1 << 0,
    PANEL_IC_CTRL_DATA   = 1 << 4,
    PANEL_IC_GRAM_BYPASS = 1 << 7
};

typedef struct panel_state {
    uint32_t param;
    uint16_t row, dstRow, srcRow;
    uint8_t cmd, col, colDir;

    uint32_t rowReg, colReg;
    uint16_t rowStart, rowEnd, colStart, colEnd;
    uint16_t partialStart, partialEnd, topArea, scrollArea, bottomArea, scrollStart;
    uint8_t mode, ifBpp, ifCtl, ifRed, ifGreen, ifBlue, mac, gamma;
    uint8_t frame[320][240][3], display[240][320][4];

    bool tear;
    uint8_t gammaCorrection[2][16];

    /* Below state is initialized at runtime */
    uint8_t lut[128];
} panel_state_t;

extern panel_state_t panel;

void panel_reset(void);
bool panel_restore(FILE *image);
bool panel_save(FILE *image);

bool panel_hsync(void);
bool panel_vsync(void);
bool panel_refresh_pixel(void);
void panel_update_pixel_18bpp(uint8_t r, uint8_t g, uint8_t b);
void panel_update_pixel_16bpp(uint8_t r, uint8_t g, uint8_t b);
void panel_update_pixel_12bpp(uint8_t r, uint8_t g, uint8_t b);

uint8_t panel_spi_select(uint32_t* rxData);
uint8_t panel_spi_transfer(uint32_t txData, uint32_t* rxData);
void panel_spi_deselect(void);

#ifdef __cplusplus
}
#endif

#endif

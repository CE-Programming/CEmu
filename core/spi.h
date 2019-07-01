#ifndef SPI_H
#define SPI_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define SPI_RED 0
#define SPI_GREEN 1
#define SPI_BLUE 2
#define SPI_ALPHA 3
#define SPI_NUM_ROWS 320
#define SPI_LAST_ROW 319
#define SPI_NUM_COLS 240
#define SPI_LAST_COL 239

enum spi_mode {
    SPI_MODE_SLEEP   = 1 << 0,
    SPI_MODE_OFF     = 1 << 1,
    SPI_MODE_BLANK   = 1 << 2,
    SPI_MODE_PARTIAL = 1 << 3,
    SPI_MODE_INVERT  = 1 << 4,
    SPI_MODE_IDLE    = 1 << 5,
    SPI_MODE_SCROLL  = 1 << 6,
    SPI_MODE_IGNORE  = 1 << 7
};

enum spi_mac {
    SPI_MAC_HRO = 1 << 2,
    SPI_MAC_BGR = 1 << 3,
    SPI_MAC_VRO = 1 << 4,
    SPI_MAC_RCX = 1 << 5,
    SPI_MAC_CAO = 1 << 6,
    SPI_MAC_RAO = 1 << 7
};

enum spi_ic {
    SPI_IC_CTRL_SYNC   = 1 << 0,
    SPI_IC_CTRL_DATA   = 1 << 4,
    SPI_IC_GRAM_BYPASS = 1 << 7
};

typedef struct spi_state {
    uint32_t param;
    uint16_t fifo, row, dstRow, srcRow;
    uint8_t regs[0x18];
    uint8_t cmd, col, colDir;

    uint32_t scanLine, rowReg, colReg;
    uint16_t rowStart, rowEnd, colStart, colEnd;
    uint16_t partialStart, partialEnd, topArea, scrollArea, bottomArea, scrollStart;
    uint8_t mode, ifBpp, ifCtl, ifRed, ifGreen, ifBlue, mac, gamma;
    uint8_t lut[128], frame[320][240][3], display[240][320][4];

    bool tear;
    uint16_t tearLine;
    uint8_t gammaCorrection[2][16];
} spi_state_t;

extern spi_state_t spi;

eZ80portrange_t init_spi(void);
void spi_reset(void);
bool spi_hsync(void);
bool spi_vsync(void);
bool spi_refresh_pixel(void);
void spi_update_pixel_18bpp(uint8_t r, uint8_t g, uint8_t b);
void spi_update_pixel_16bpp(uint8_t r, uint8_t g, uint8_t b);
void spi_update_pixel_12bpp(uint8_t r, uint8_t g, uint8_t b);
bool spi_restore(FILE *image);
bool spi_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif

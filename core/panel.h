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
#define PANEL_ADDR_MASK 0x1FF

enum panel_mode {
    PANEL_MODE_SLEEP   = 1 << 0,
    PANEL_MODE_OFF     = 1 << 1,
    PANEL_MODE_BLANK   = 1 << 2,
    PANEL_MODE_PARTIAL = 1 << 3,
    PANEL_MODE_IDLE    = 1 << 4,
    PANEL_MODE_SCROLL  = 1 << 5
};

enum panel_display_mode {
    PANEL_DM_MCU = 0,
    PANEL_DM_RGB = 1,
    PANEL_DM_VSYNC = 2
};

typedef struct panel_gamma {
    uint8_t V0 : 4;
    uint8_t V63 : 4;

    uint8_t V1 : 6;
    uint8_t : 2;

    uint8_t V2 : 6;
    uint8_t : 2;

    uint8_t V4 : 5;
    uint8_t : 3;

    uint8_t V6 : 5;
    uint8_t : 3;

    uint8_t V13 : 4;
    uint8_t J0 : 2;
    uint8_t : 2;

    uint8_t V20 : 7;
    uint8_t : 1;

    uint8_t V27 : 3;
    uint8_t : 1;
    uint8_t V36 : 3;
    uint8_t : 1;

    uint8_t V43 : 7;
    uint8_t : 1;

    uint8_t V50 : 4;
    uint8_t J1 : 2;
    uint8_t : 2;

    uint8_t V57 : 5;
    uint8_t : 3;

    uint8_t V59 : 5;
    uint8_t : 3;

    uint8_t V61 : 6;
    uint8_t : 2;

    uint8_t V62 : 6;
    uint8_t : 2;
} panel_gamma_t;

typedef struct panel_params {
    /* Word params */
    struct {
        uint16_t XS;
        uint16_t XE;
    } CASET;
    struct {
        uint16_t YS;
        uint16_t YE;
    } RASET;
    struct {
        uint16_t PSL;
        uint16_t PEL;
    } PTLAR;
    struct {
        uint16_t TFA;
        uint16_t VSA;
        uint16_t BFA;
    } VSCRDEF;
    struct {
        uint16_t VSP;
    } VSCRSADD;
    struct {
        uint16_t N;
    } TESCAN;
    /* Byte params */
    struct {
        uint8_t : 2;
        uint8_t MH : 1;
        uint8_t RGB : 1;
        uint8_t ML : 1;
        uint8_t MV : 1;
        uint8_t MX : 1;
        uint8_t MY : 1;
    } MADCTL;
    struct {
        uint8_t MCU : 3;
        uint8_t : 1;
        uint8_t RGB : 3;
        uint8_t : 1;
    } COLMOD;
    struct {
        uint8_t DBV;
    } DISBV;
    struct {
        uint8_t : 2;
        uint8_t BL : 1;
        uint8_t DD : 1;
        uint8_t : 1;
        uint8_t BCTRL : 1;
        uint8_t : 2;
    } CTRLD;
    struct {
        uint8_t C : 2;
        uint8_t : 2;
        uint8_t CE : 2;
        uint8_t : 1;
        uint8_t CECTRL : 1;
    } CACE;
    struct {
        uint8_t CMB;
    } CABCMB;
    struct {
        uint8_t DM : 2;
        uint8_t : 2;
        uint8_t RM : 1;
        uint8_t : 3;
        uint8_t MDT : 2;

        uint8_t RIM : 1;
        uint8_t ENDIAN : 1;
        uint8_t EPF : 2;
        uint8_t : 2;
    } RAMCTRL;
    struct {
        uint8_t EPL : 1;
        uint8_t DPL : 1;
        uint8_t HSPL : 1;
        uint8_t VSPL : 1;
        uint8_t : 1;
        uint8_t RCM : 2;
        uint8_t WO : 1;

        uint8_t VBP : 7;
        uint8_t : 1;

        uint8_t HBP : 5;
        uint8_t : 3;
    } RGBCTRL;
    struct {
        uint8_t BPA : 7;
        uint8_t : 1;

        uint8_t FPA : 7;
        uint8_t : 1;

        uint8_t PSEN : 1;
        uint8_t : 7;

        uint8_t FPB : 4;
        uint8_t BPB : 4;

        uint8_t FPC : 4;
        uint8_t BPC : 4;
    } PORCTRL;
    struct {
        uint8_t DIV : 2;
        uint8_t : 2;
        uint8_t FRSEN : 1;
        uint8_t : 3;

        uint8_t RTNB : 5;
        uint8_t NLB : 3;

        uint8_t RTNC : 5;
        uint8_t NLC : 3;
    } FRCTRL1;
    struct {
        uint8_t ISC : 4;
        uint8_t PTGISC : 1;
        uint8_t : 2;
        uint8_t NDL : 1;
    } PARCTRL;
    struct {
        uint8_t VGLS : 3;
        uint8_t : 1;
        uint8_t VGHS : 3;
        uint8_t : 1;
    } GCTRL;
    struct {
        uint8_t PAD_2A;
        uint8_t PAD_2B;

        uint8_t GTA : 6;
        uint8_t : 2;

        uint8_t GOF : 4;
        uint8_t GOFR : 4;
    } GTADJ;
    struct {
        uint8_t VCOMS : 6;
        uint8_t : 2;
    } VCOMS;
    struct {
        uint8_t IS : 1;
        uint8_t NS : 1;
        uint8_t : 6;
    } POWSAVE;
    struct {
        uint8_t DOFSAVE : 1;
        uint8_t : 7;
    } DLPOFFSAVE;
    struct {
        uint8_t XGS : 1;
        uint8_t XMV : 1;
        uint8_t XMH : 1;
        uint8_t XMX : 1;
        uint8_t XINV : 1;
        uint8_t XBGR : 1;
        uint8_t XMY : 1;
        uint8_t : 1;
    } LCMCTRL;
    struct {
        uint8_t ID1;
        uint8_t ID2;
        uint8_t ID3;
    } IDSET;
    struct {
        uint8_t CMDEN : 1;
        uint8_t : 7;

        uint8_t PAD_FF;
    } VDVVRHEN;
    struct {
        uint8_t VRHS : 6;
        uint8_t : 2;
    } VRHSET;
    struct {
        uint8_t VDVS : 6;
        uint8_t : 2;
    } VDVSET;
    struct {
        uint8_t VCMOFS : 6;
        uint8_t : 2;
    } VCMOFSET;
    struct {
        uint8_t RTNA : 5;
        uint8_t NLA : 3;
    } FRCTRL2;
    struct {
        uint8_t PWMPOL : 1;
        uint8_t PWMFIX : 1;
        uint8_t DPOFPWM : 1;
        uint8_t LEDONREV : 1;
        uint8_t : 4;
    } CABCCTRL;
    struct {
        uint8_t PAD_08;
    } REGSEL1;
    struct {
        uint8_t PAD_0F;
    } REGSEL2;
    struct {
        uint8_t CLK : 3;
        uint8_t CS : 3;
        uint8_t : 2;
    } PWMFRSEL;
    struct {
        uint8_t PAD_A4;

        uint8_t VDS : 2;
        uint8_t : 2;
        uint8_t AVCL : 2;
        uint8_t AVDD : 2;
    } PWCTRL1;
    struct {
        uint8_t PAD_4C;
    } VAPVANEN;
    struct {
        uint8_t PAD_5A;
        uint8_t PAD_69;
        uint8_t PAD_02;

        uint8_t EN : 1;
        uint8_t : 7;
    } CMD2EN;
    struct {
        uint8_t NL : 6;
        uint8_t : 2;

        uint8_t SCN : 6;
        uint8_t : 2;

        uint8_t GS : 1;
        uint8_t : 1;
        uint8_t SM : 1;
        uint8_t : 1;
        uint8_t TMG : 1;
        uint8_t : 3;
    } GATECTRL;
    struct {
        uint8_t SPIRD : 1;
        uint8_t : 3;
        uint8_t SPI2EN : 1;
        uint8_t : 3;
    } SPI2EN;
    struct {
        uint8_t STP14CK : 2;
        uint8_t : 2;
        uint8_t SBCLK : 2;
        uint8_t : 2;
    } PWCTRL2;
    struct {
        uint8_t SEQ : 5;
        uint8_t : 3;

        uint8_t SPRET : 5;
        uint8_t : 3;

        uint8_t GEQ : 4;
        uint8_t : 4;
    } EQCTRL;
    struct {
        uint8_t PAD_01;
    } PROMCTRL;
    struct {
        uint8_t PAD_5A;
        uint8_t PAD_69;
        uint8_t PAD_EE;

        uint8_t : 2;
        uint8_t PROMEN : 1;
        uint8_t : 5;
    } PROMEN;
    struct {
        uint8_t ADD;
        uint8_t D;
    } NVMSET;
    struct {
        uint8_t PAD_29;
        uint8_t PAD_A5;
    } PROMACT;
    /* Gamma-modifying params */
    struct {
        uint8_t GC : 4;
        uint8_t : 4;
    } GAMSET;
    struct {
        uint8_t : 2;
        uint8_t DGMEN : 1;
        uint8_t : 5;
    } DGMEN;
    panel_gamma_t PVGAMCTRL;
    panel_gamma_t NVGAMCTRL;
    uint8_t DGMLUTR[64];
    uint8_t DGMLUTB[64];
} panel_params_t;

typedef struct panel_state {
    uint32_t lastScanTick, nextLineTick;
    uint16_t ticksPerLine, linesPerFrame;
    uint16_t row, col, dstRow, srcRow;
    uint8_t cmd, paramIter, paramEnd;
    bool invert, tear, windowFull;

    uint32_t xAddr, yAddr;
    uint16_t partialStart, partialEnd, topArea, bottomArea, scrollStart;
    uint8_t displayMode, horizBackPorch;
    uint8_t mode, pendingMode, ifRed, ifGreen, ifBlue;

    panel_params_t params;
    uint8_t frame[320][240][3], display[240][320][4];

    /* Below state is initialized at runtime */
    uint8_t gammaLut[3][64];
    uint8_t blankLevel;
    bool accurateGamma;
    bool gammaDirty;
} panel_state_t;

extern panel_state_t panel;

void panel_reset(void);
bool panel_restore(FILE *image);
bool panel_save(FILE *image);

void panel_hw_reset(void);
bool panel_hsync(void);
void panel_vsync(void);
void panel_refresh_pixels_until(uint32_t currTick);
void panel_refresh_pixels(uint16_t count);
void panel_update_pixel_rgb(uint8_t r, uint8_t g, uint8_t b);

uint8_t panel_spi_select(uint32_t* rxData);
uint8_t panel_spi_transfer(uint32_t txData, uint32_t* rxData);
void panel_spi_deselect(void);

#ifdef __cplusplus
}
#endif

#endif

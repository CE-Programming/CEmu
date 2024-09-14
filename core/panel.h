#ifndef PANEL_H
#define PANEL_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define PANEL_RAM_RED 0
#define PANEL_RAM_GREEN 1
#define PANEL_RAM_BLUE 2
#define PANEL_DISP_RED 16
#define PANEL_DISP_GREEN 8
#define PANEL_DISP_BLUE 0
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
    PANEL_DM_VSYNC = 2,
    PANEL_DM_RESERVED = 3
};

enum panel_spi_mem_flags {
    PANEL_SPI_FIRST_PIXEL = 1 << 0,
    PANEL_SPI_WINDOW_FULL = 1 << 1,
    PANEL_SPI_AUTO_RESET  = 1 << 2
};

typedef struct panel_mem_ptr {
    uint16_t xAddr;
    uint16_t yAddr;
} panel_mem_ptr_t;

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
# define PARAM_BYTE_LOW(NAME, WIDTH)    \
    uint8_t NAME : WIDTH;               \
    uint8_t : 8 - WIDTH
#else
# define PARAM_BYTE_LOW(NAME, WIDTH)    \
    uint8_t : 8 - WIDTH;                \
    uint8_t NAME : WIDTH
#endif

typedef struct panel_gamma {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
    uint8_t V0 : 4;
    uint8_t V63 : 4;
#else
    uint8_t V63 : 4;
    uint8_t V0 : 4;
#endif

    PARAM_BYTE_LOW(V1, 6);
    PARAM_BYTE_LOW(V2, 6);
    PARAM_BYTE_LOW(V4, 5);
    PARAM_BYTE_LOW(V6, 5);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
    uint8_t V13 : 4;
    uint8_t J0 : 2;
    uint8_t : 2;
#else
    uint8_t : 2;
    uint8_t J0 : 2;
    uint8_t V13 : 4;
#endif

    PARAM_BYTE_LOW(V20, 7);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
    uint8_t V27 : 3;
    uint8_t : 1;
    uint8_t V36 : 3;
    uint8_t : 1;
#else
    uint8_t : 1;
    uint8_t V36 : 3;
    uint8_t : 1;
    uint8_t V27 : 3;
#endif

    PARAM_BYTE_LOW(V43, 7);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
    uint8_t V50 : 4;
    uint8_t J1 : 2;
    uint8_t : 2;
#else
    uint8_t : 2;
    uint8_t J1 : 2;
    uint8_t V50 : 4;
#endif

    PARAM_BYTE_LOW(V57, 5);
    PARAM_BYTE_LOW(V59, 5);
    PARAM_BYTE_LOW(V61, 6);
    PARAM_BYTE_LOW(V62, 6);
} panel_gamma_t;

typedef struct panel_params {
    /* Word params */
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
    struct {
        uint16_t XS;
        uint16_t XE;
    } CASET;
    struct {
        uint16_t YS;
        uint16_t YE;
    } RASET;
    /* Byte params */
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t : 2;
        uint8_t MH : 1;
        uint8_t RGB : 1;
        uint8_t ML : 1;
        uint8_t MV : 1;
        uint8_t MX : 1;
        uint8_t MY : 1;
#else
        uint8_t MY : 1;
        uint8_t MX : 1;
        uint8_t MV : 1;
        uint8_t ML : 1;
        uint8_t RGB : 1;
        uint8_t MH : 1;
        uint8_t : 2;
#endif
    } MADCTL;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t MCU : 3;
        uint8_t : 1;
        uint8_t RGB : 3;
        uint8_t : 1;
#else
        uint8_t : 1;
        uint8_t RGB : 3;
        uint8_t : 1;
        uint8_t MCU : 3;
#endif
    } COLMOD;
    struct {
        uint8_t DBV;
    } DISBV;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t : 2;
        uint8_t BL : 1;
        uint8_t DD : 1;
        uint8_t : 1;
        uint8_t BCTRL : 1;
        uint8_t : 2;
#else
        uint8_t : 2;
        uint8_t BCTRL : 1;
        uint8_t : 1;
        uint8_t DD : 1;
        uint8_t BL : 1;
        uint8_t : 2;
#endif
    } CTRLD;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t C : 2;
        uint8_t : 2;
        uint8_t CE : 2;
        uint8_t : 1;
        uint8_t CECTRL : 1;
#else
        uint8_t CECTRL : 1;
        uint8_t : 1;
        uint8_t CE : 2;
        uint8_t : 2;
        uint8_t C : 2;
#endif
    } CACE;
    struct {
        uint8_t CMB;
    } CABCMB;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t DM : 2;
        uint8_t : 2;
        uint8_t RM : 1;
        uint8_t : 3;

        uint8_t MDT : 2;
        uint8_t RIM : 1;
        uint8_t ENDIAN : 1;
        uint8_t EPF : 2;
        uint8_t WEMODE0 : 1;
        uint8_t WEMODE1 : 1;
#else
        uint8_t : 3;
        uint8_t RM : 1;
        uint8_t : 2;
        uint8_t DM : 2;

        uint8_t WEMODE1 : 1;
        uint8_t WEMODE0 : 1;
        uint8_t EPF : 2;
        uint8_t ENDIAN : 1;
        uint8_t RIM : 1;
        uint8_t MDT : 2;
#endif
    } RAMCTRL;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t EPL : 1;
        uint8_t DPL : 1;
        uint8_t HSPL : 1;
        uint8_t VSPL : 1;
        uint8_t : 1;
        uint8_t RCM : 2;
        uint8_t WO : 1;
#else
        uint8_t WO : 1;
        uint8_t RCM : 2;
        uint8_t : 1;
        uint8_t VSPL : 1;
        uint8_t HSPL : 1;
        uint8_t DPL : 1;
        uint8_t EPL : 1;
#endif

        PARAM_BYTE_LOW(VBP, 7);
        PARAM_BYTE_LOW(HBP, 5);
    } RGBCTRL;
    struct {
        PARAM_BYTE_LOW(BPA, 7);
        PARAM_BYTE_LOW(FPA, 7);
        PARAM_BYTE_LOW(PSEN, 1);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t FPB : 4;
        uint8_t BPB : 4;

        uint8_t FPC : 4;
        uint8_t BPC : 4;
#else
        uint8_t BPB : 4;
        uint8_t FPB : 4;

        uint8_t BPC : 4;
        uint8_t FPC : 4;
#endif
    } PORCTRL;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t DIV : 2;
        uint8_t : 2;
        uint8_t FRSEN : 1;
        uint8_t : 3;

        uint8_t RTNB : 5;
        uint8_t NLB : 3;

        uint8_t RTNC : 5;
        uint8_t NLC : 3;
#else
        uint8_t : 3;
        uint8_t FRSEN : 1;
        uint8_t : 2;
        uint8_t DIV : 2;

        uint8_t NLB : 3;
        uint8_t RTNB : 5;

        uint8_t NLC : 3;
        uint8_t RTNC : 5;
#endif
    } FRCTRL1;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t ISC : 4;
        uint8_t PTGISC : 1;
        uint8_t : 2;
        uint8_t NDL : 1;
#else
        uint8_t NDL : 1;
        uint8_t : 2;
        uint8_t PTGISC : 1;
        uint8_t ISC : 4;
#endif
    } PARCTRL;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t VGLS : 3;
        uint8_t : 1;
        uint8_t VGHS : 3;
        uint8_t : 1;
#else
        uint8_t : 1;
        uint8_t VGHS : 3;
        uint8_t : 1;
        uint8_t VGLS : 3;
#endif
    } GCTRL;
    struct {
        uint8_t PAD_2A;
        uint8_t PAD_2B;
        PARAM_BYTE_LOW(GTA, 6);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t GOF : 4;
        uint8_t GOFR : 4;
#else
        uint8_t GOFR : 4;
        uint8_t GOF : 4;
#endif
    } GTADJ;
    struct {
        PARAM_BYTE_LOW(VCOMS, 6);
    } VCOMS;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t IS : 1;
        uint8_t NS : 1;
        uint8_t : 6;
#else
        uint8_t : 6;
        uint8_t NS : 1;
        uint8_t IS : 1;
#endif
    } POWSAVE;
    struct {
        PARAM_BYTE_LOW(DOFSAVE, 1);
    } DLPOFFSAVE;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t XGS : 1;
        uint8_t XMV : 1;
        uint8_t XMH : 1;
        uint8_t XMX : 1;
        uint8_t XINV : 1;
        uint8_t XBGR : 1;
        uint8_t XMY : 1;
        uint8_t : 1;
#else
        uint8_t : 1;
        uint8_t XMY : 1;
        uint8_t XBGR : 1;
        uint8_t XINV : 1;
        uint8_t XMX : 1;
        uint8_t XMH : 1;
        uint8_t XMV : 1;
        uint8_t XGS : 1;
#endif
    } LCMCTRL;
    struct {
        uint8_t ID1;
        uint8_t ID2;
        uint8_t ID3;
    } IDSET;
    struct {
        PARAM_BYTE_LOW(CMDEN, 1);
        uint8_t PAD_FF;
    } VDVVRHEN;
    struct {
        PARAM_BYTE_LOW(VRHS, 6);
    } VRHSET;
    struct {
        PARAM_BYTE_LOW(VDVS, 6);
    } VDVSET;
    struct {
        PARAM_BYTE_LOW(VCMOFS, 6);
    } VCMOFSET;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t RTNA : 5;
        uint8_t NLA : 3;
#else
        uint8_t NLA : 3;
        uint8_t RTNA : 5;
#endif
    } FRCTRL2;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t PWMPOL : 1;
        uint8_t PWMFIX : 1;
        uint8_t DPOFPWM : 1;
        uint8_t LEDONREV : 1;
        uint8_t : 4;
#else
        uint8_t : 4;
        uint8_t LEDONREV : 1;
        uint8_t DPOFPWM : 1;
        uint8_t PWMFIX : 1;
        uint8_t PWMPOL : 1;
#endif
    } CABCCTRL;
    struct {
        uint8_t PAD_08;
    } REGSEL1;
    struct {
        uint8_t PAD_0F;
    } REGSEL2;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t CLK : 3;
        uint8_t CS : 3;
        uint8_t : 2;
#else
        uint8_t : 2;
        uint8_t CS : 3;
        uint8_t CLK : 3;
#endif
    } PWMFRSEL;
    struct {
        uint8_t PAD_A4;

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t VDS : 2;
        uint8_t : 2;
        uint8_t AVCL : 2;
        uint8_t AVDD : 2;
#else
        uint8_t AVDD : 2;
        uint8_t AVCL : 2;
        uint8_t : 2;
        uint8_t VDS : 2;
#endif
    } PWCTRL1;
    struct {
        uint8_t PAD_4C;
    } VAPVANEN;
    struct {
        uint8_t PAD_5A;
        uint8_t PAD_69;
        uint8_t PAD_02;
        PARAM_BYTE_LOW(EN, 1);
    } CMD2EN;
    struct {
        PARAM_BYTE_LOW(NL, 6);
        PARAM_BYTE_LOW(SCN, 6);

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t GS : 1;
        uint8_t SS : 1;
        uint8_t SM : 1;
        uint8_t : 1;
        uint8_t TMG : 1;
        uint8_t : 3;
#else
        uint8_t : 3;
        uint8_t TMG : 1;
        uint8_t : 1;
        uint8_t SM : 1;
        uint8_t SS : 1;
        uint8_t GS : 1;
#endif
    } GATECTRL;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t SPIRD : 1;
        uint8_t : 3;
        uint8_t SPI2EN : 1;
        uint8_t : 3;
#else
        uint8_t : 3;
        uint8_t SPI2EN : 1;
        uint8_t : 3;
        uint8_t SPIRD : 1;
#endif
    } SPI2EN;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t STP14CK : 2;
        uint8_t : 2;
        uint8_t SBCLK : 2;
        uint8_t : 2;
#else
        uint8_t : 2;
        uint8_t SBCLK : 2;
        uint8_t : 2;
        uint8_t STP14CK : 2;
#endif
    } PWCTRL2;
    struct {
        PARAM_BYTE_LOW(SEQ, 5);
        PARAM_BYTE_LOW(SPRET, 5);
        PARAM_BYTE_LOW(GEQ, 4);
    } EQCTRL;
    struct {
        uint8_t PAD_01;
    } PROMCTRL;
    struct {
        uint8_t PAD_5A;
        uint8_t PAD_69;
        uint8_t PAD_EE;

#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t : 2;
        uint8_t PROMEN : 1;
        uint8_t : 5;
#else
        uint8_t : 5;
        uint8_t PROMEN : 1;
        uint8_t : 2;
#endif
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
        PARAM_BYTE_LOW(GC, 4);
    } GAMSET;
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint8_t : 2;
        uint8_t DGMEN : 1;
        uint8_t : 5;
#else
        uint8_t : 5;
        uint8_t DGMEN : 1;
        uint8_t : 2;
#endif
    } DGMEN;
    panel_gamma_t PVGAMCTRL;
    panel_gamma_t NVGAMCTRL;
    uint8_t DGMLUTR[64];
    uint8_t DGMLUTB[64];
} panel_params_t;
#undef PARAM_BYTE_LOW

typedef struct panel_state {
    uint32_t lineStartTick;
    uint16_t ticksPerLine, linesPerFrame, horizBackPorch;
    int16_t col;
    uint16_t row, dstRow, srcRow;
    uint8_t nextRamCol, cmd, paramIter, paramEnd;
    bool invert, tear, currLineBuffer, windowFullRgb;

    panel_mem_ptr_t spiMemPtr, rgbMemPtr;
    uint16_t partialStart, partialEnd, topArea, bottomArea, scrollStart;
    uint8_t displayMode, mode, pendingMode, spiMemFlags;
    uint32_t ifPixel;

    panel_params_t params;
    uint8_t frame[PANEL_NUM_ROWS][PANEL_NUM_COLS][3];
    uint32_t lineBuffers[2][PANEL_NUM_COLS], display[PANEL_NUM_COLS][PANEL_NUM_ROWS];

    /* Below state is not affected by reset */
    uint32_t clockRate;

    /* Below state is initialized at runtime */
    uint8_t gammaLut[3][64];
    void (*clock_pixel)(uint16_t bgr565);
    void (*write_pixel)(panel_mem_ptr_t *memPtr, uint32_t bgr666);
    panel_mem_ptr_t windowStart, windowEnd;
    uint8_t (*windowBasePtr)[3];
    uint32_t blankPixel;
    int8_t xDir, yDir;
    bool accurateGamma;
    bool gammaDirty;
    bool skipFrame;
} panel_state_t;

extern panel_state_t panel;

void init_panel(void);
void panel_reset(void);
bool panel_restore(FILE *image);
bool panel_save(FILE *image);

void panel_hw_reset(void);
void panel_update_clock_rate(void);
bool panel_hsync(void);
void panel_vsync(void);
void panel_clock_porch(uint32_t clocks);
void panel_scan_until(uint32_t currTick);

void panel_spi_select(bool low);
uint8_t panel_spi_peek(uint32_t* rxData);
uint8_t panel_spi_transfer(uint32_t txData, uint32_t* rxData);

#ifdef __cplusplus
}
#endif

#endif

#ifndef LCD_H
#define LCD_H

#include "defines.h"

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define LCD_WIDTH      (320)
#define LCD_HEIGHT     (240)
#define LCD_SIZE       (LCD_WIDTH * LCD_HEIGHT)
#define LCD_BYTE_SIZE  (LCD_SIZE * 2)
#define LCD_RAM_OFFSET (0x40000)

enum lcd_comp {
    LCD_SYNC,
    LCD_BACK_PORCH,
    LCD_ACTIVE_VIDEO,
    LCD_FRONT_PORCH,
    LCD_LNBU
};

typedef struct lcd_crsr_state {
    uint32_t crsrXY;               /* Cursor XY position register */
    uint32_t crsrClip;             /* Cursor clip position register */
    uint32_t crsrImage;            /* Cursor image index */
} lcd_crsr_state_t;

typedef struct lcd_state {
    uint32_t timing[4];

    uint32_t control;              /* Control register */
    uint8_t imsc;                  /* Interrupt mask set/clear register, plus cursor interrupt bit */
    uint8_t ris;                   /* Interrupt raw status register, plus cursor interrupt bit */
    uint8_t crsrControl;           /* Cursor control register */
    uint8_t crsrConfig;            /* Cursor configuration register */

    uint32_t upbase;               /* Upper panel frame base address register */
    uint32_t lpbase;               /* Lower panel frame base address register */
    uint32_t upcurr;               /* Upper panel current frame address register */
    uint32_t lpcurr;               /* Lower panel current frame address register */

    /* 256x16-bit color palette registers */
    /* 256 palette entries organized as 128 locations of two entries per word */
    union {
        uint32_t paletteWords[0x80]; /* For alignment */
        uint8_t  paletteBytes[0x200];
    };

    /* Cursor image RAM registers */
    /* 256-word wide values defining images overlaid by the hw cursor mechanism */
    union {
        uint32_t crsrImageWords[0x100]; /* For alignment */
        uint8_t  crsrImageBytes[0x400];
    };

    uint32_t crsrPalette0;         /* Cursor palette registers */
    uint32_t crsrPalette1;
    lcd_crsr_state_t crsrRegs[2];  /* Async and frame sync cursor registers */

    /* Internal */
    bool prefill, crsrIntrptd;
    uint8_t pos;
    uint32_t curCol, curRow, curCrsrCol, curCrsrWidth, curCrsrOffset, fifo[64];
    enum lcd_comp compare;
    uint32_t PPL, HSW, HFP, HBP, LPP, VSW, VFP, VBP, PCD, ACB, CPL, LED, LCDBPP, BPP, PPF;
    bool CLKSEL, IVS, IHS, IPC, IOE, LEE, BGR, BEBO, BEPO, WTRMRK;
    uint16_t crsrPalette[4];       /* Cursor palette stored as BGR565, plus transparency/inversion masks */
    uint16_t palettes[2][0x100];   /* Palette stored as BGR565 and RGB565 in native endianness */

    /* Everything above here goes into the state */
    uint32_t *data;                /* Pointer to start of data to start extracting from */
    uint32_t *data_end;            /* End pointer that is allowed access */

    /* Everything after here persists through reset! */
    int useDma;
    bool (*gui_callback)(void*);
    void *gui_callback_data;
} lcd_state_t;

extern lcd_state_t lcd;

void lcd_reset(void);
void lcd_free(void);
eZ80portrange_t init_lcd(void);
bool lcd_restore(FILE *image);
bool lcd_save(FILE *image);
void lcd_update(void);
void lcd_disable(void);
bool lcd_gui_event(void);

/* api functions */
void emu_lcd_drawframe(void *output);
void emu_set_lcd_callback(bool (*callback)(void*), void *data);
void emu_set_lcd_dma(int enable);
void emu_set_lcd_gamma(int enable);

/* advanced api functions */
void emu_set_lcd_ptrs(uint32_t **dat, uint32_t **dat_end, int width, int height, uint32_t addr, uint32_t lcd_control, bool mask);
void emu_lcd_drawmem(void *output, void *data, void *data_end, uint32_t lcd_control, int size);

#ifdef __cplusplus
}
#endif

#endif

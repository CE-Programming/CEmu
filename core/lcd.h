#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include "spi.h"

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define LCD_WIDTH      (320)
#define LCD_HEIGHT     (240)
#define LCD_SIZE       (LCD_WIDTH * LCD_HEIGHT)
#define LCD_BYTE_SIZE  (LCD_SIZE * 2)
#define LCD_RAM_ADDR   (0xD40000)
#define LCD_RAM_OFFSET (0x40000)

/* Set this callback function pointer from the GUI. Called in lcd_event() */
extern void (*lcd_gui_callback)(void*);
extern void *lcd_gui_callback_data;

enum lcd_comp {
    LCD_SYNC,
    LCD_BACK_PORCH,
    LCD_ACTIVE_VIDEO,
    LCD_FRONT_PORCH,
    LCD_LNBU
};

/* Standard LCD state */
typedef struct lcd_state {
    uint32_t timing[4];

    uint32_t control;             /* Control register */
    uint32_t imsc;                /* Interrupt mask set/clear register */
    uint32_t ris;

    uint32_t upbase;               /* Upper panel frame base address register */
    uint32_t lpbase;               /* Lower panel frame base address register */
    uint32_t upcurr;               /* Upper panel current frame address register */
    uint32_t lpcurr;               /* Lower panel current frame address register */

    /* 256x16-bit color palette registers */
    /* 256 palette entries organized as 128 locations of two entries per word */
    uint16_t palette[0x100];

    /* Cursor image RAM registers */
    /* 256-word wide values defining images overlaid by the hw cursor mechanism */
    uint32_t crsrImage[0x100];
    uint32_t crsrControl;          /* Cursor control register */
    uint32_t crsrConfig;           /* Cursor configuration register */
    uint32_t crsrPalette0;         /* Cursor palette registers */
    uint32_t crsrPalette1;
    uint32_t crsrXY;               /* Cursor XY position register */
    uint32_t crsrClip;             /* Cursor clip position register */
    uint32_t crsrImsc;             /* Cursor interrupt mask set/clear register */
    uint32_t crsrIcr;              /* Cursor interrupt clear register */
    uint32_t crsrRis;              /* Cursor raw interrupt status register - const */

    /* Internal registers */
    bool prefill;
    uint8_t pos, fifo[256];
    uint32_t curCol, curRow;
    enum lcd_comp compare;
    uint32_t PPL, HSW, HFP, HBP, LPP, VSW, VFP, VBP, PCD, ACB, CPL, LED, LCDBPP, BPP, PPF;
    bool CLKSEL, IVS, IHS, IPC, IOE, LEE, BGR, BEBO, BEPO, WTRMRK;
    uint32_t *data;                /* Pointer to start of data to start extracting from */
    uint32_t *data_end;            /* End pointer that is allowed access */
    bool spi;
} lcd_state_t;

/* Global LCD state */
extern lcd_state_t lcd;

/* Available Functions */
void lcd_reset(void);
eZ80portrange_t init_lcd(void);

void lcd_drawframe(void *output, void *data, void *data_end, uint32_t control, uint32_t size);
void lcd_setptrs(uint32_t **dat, uint32_t **dat_end, uint32_t width, uint32_t height, uint32_t addr, uint32_t control, bool mask);

void lcd_update(void);
void lcd_disable(void);
void lcd_gui_event(void);

/* Save/Restore */
bool lcd_restore(FILE *image);
bool lcd_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif

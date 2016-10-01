#ifndef LCD_H
#define LCD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

/* Internal Use */
extern uint32_t lcd_framebuffer[320*240];

/* Standard LCD state */
PACK(typedef struct lcd_cntrl_state {
    uint32_t timing[4];

    uint32_t control;     /* Control register */
    uint32_t imsc;        /* Interrupt mask set/clear register */
    uint32_t ris;

    uint32_t upbase;        /* Upper panel frame base address register */
    uint32_t lpbase;        /* Lower panel frame base address register */
    uint32_t upcurr;        /* Upper panel current frame address register */
    uint32_t lpcurr;        /* Lower panel current frame address register */

    /* 256x16-bit color palette registers */
    /* 256 palette entries organized as 128 locations of two entries per word */
    uint16_t palette[0x100];


    /* Cursor image RAM registers (TODO) */
    /* 256-word wide values defining images overlaid by the hw cursor mechanism */
    uint32_t crsrImage[0x100];
    uint32_t crsrControl;        /* Cursor control register */
    uint32_t crsrConfig;         /* Cursor configuration register */
    uint32_t crsrPalette0;       /* Cursor palette registers */
    uint32_t crsrPalette1;
    uint32_t crsrXY;             /* Cursor XY position register */
    uint32_t crsrClip;           /* Cursor clip position register */
    uint32_t crsrImsc;           /* Cursor interrupt mask set/clear register */
    uint32_t crsrIcr;            /* Cursor interrupt clear register */
    uint32_t crsrRis;            /* Cursor raw interrupt status register - const */
}) lcd_state_t;

/* Global LCD state */
extern lcd_state_t lcd;

/* Available Functions */
void lcd_reset(void);
eZ80portrange_t init_lcd(void);

void lcd_drawframe(uint32_t *out, lcd_state_t*);

/* Set this callback function pointer from the GUI. Called in lcd_event() */
extern void (*lcd_event_gui_callback)(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool lcd_restore(const emu_image*);
bool lcd_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif

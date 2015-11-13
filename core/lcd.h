#ifndef LCD_H
#define LCD_H

#include <core/memory.h>

#ifdef __cplusplus
extern "C" {
#endif

// Type definitions
typedef struct lcd_cntrl_state lcd_cntrl_state_t;

// Global LCD state
extern lcd_cntrl_state_t lcd;

// Standard LCD state
struct lcd_cntrl_state {
  uint32_t timing0;     // Horizontal axis panel control register
  uint32_t timing1;     // Vertical axis panel control register
  uint32_t timing2;     // Clock and signal polarity control register
  uint32_t timing3;     // Line end control register
  uint32_t control;     // Control register
  uint32_t imsc;      // Interrupt mask set/clear register
  uint32_t ris;       // Raw interrupt status register - const
  uint32_t mis;       // Masked interrupt status register

  uint32_t upbase;        // Upper panel frame base address register
  uint32_t lpbase;        // Lower panel frame base address register
  uint32_t upcurr;        // Upper panel current frame address register
  uint32_t lpcurr;        // Lower panel current frame address register

  // 256x16-bit color palette registers
  // 256 palette entries organized as 128 locations of two entries per word
  uint16_t palette[0x103];
  // Cursor image RAM register
  // 256-word wide values defining images overlaid by the hw cursor mechanism
  int cursorimage[0x103];
  uint32_t crsrCtrl;           // Cursor control register
  uint32_t crsrConfig;         // Cursor configuration register
  uint32_t crsrPalette0;       // Cursor palette registers
  uint32_t crsrPalette1;
  uint32_t crsrXY;             // Cursor XY position register
  uint32_t crsrClip;           // Cursor clip position register
  uint32_t crsrimsc;  // Cursor interrupt mask set/clear register
  uint32_t crsricr;   // Cursor interrupt clear register
  uint32_t crsrris;   // Cursor raw interrupt status register - const
  uint32_t crsrmis;   // Cursor masked interrupt status register - const
  uint8_t bytesPerPixel;      // Bytes per pixel
  uint32_t startAddr;         // Frame buffer base address
  uint32_t maxAddr;           // Frame buffer max address
  int waterMark;              // DMA FIFO watermark

  mem_state_t *memory;
};

// Available Functions
void set_default_pal(void);
void set_pal(char *path);

void lcd_init(void);
eZ80portrange_t init_lcd(void);

void lcd_write(const uint16_t, const uint8_t);
uint8_t lcd_read(const uint16_t);
void lcd_drawframe(uint16_t *buffer, uint32_t *bitfields);

#ifdef __cplusplus
}
#endif

#endif

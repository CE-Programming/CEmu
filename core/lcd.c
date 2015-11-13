#include "core/lcd.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "core/memory.h"

// Global LCD state
lcd_cntrl_state_t lcd;

/* Draw the current screen into a 16bpp upside-down bitmap. */
void lcd_drawframe(uint16_t *buffer, uint32_t *bitfields) {

    uint32_t mode = lcd.control >> 1 & 7;
    uint32_t bpp;
    uint32_t *in;
    int row;

    if (mode <= 5) {
        bpp = 1 << mode;
    }
    else {
        bpp = 16;
    }

    if (mode == 7) {
        // 444 format
        bitfields[0] = 0x000F;
        bitfields[1] = 0x00F0;
        bitfields[2] = 0x0F00;
    } else {
        // 565 format
        bitfields[0] = 0x001F;
        bitfields[1] = 0x07E0;
        bitfields[2] = 0xF800;
    }
    if (lcd.control & (1 << 8)) {
        // BGR format (R high, B low)
        uint32_t tmp = bitfields[0];
        bitfields[0] = bitfields[2];
        bitfields[2] = tmp;
    }

    in = (uint32_t*)lcd.memory->vram;
    if (!in || !lcd.upbase || (((lcd.control>>11)&0x1) == 0)) {
        memset(buffer, 0x00, 320 * 240 * 2); return;
    }
    for (row = 0; row < 240; ++row) {
        uint16_t *out = buffer + (row * 320);
        uint32_t words = (320 / 32) * bpp;
        if (bpp < 16) {
            uint32_t mask = (1 << bpp) - 1;
            uint32_t bi = (lcd.control & (1 << 9)) ? 0 : 24;
            if (!(lcd.control & (1 << 10)))
                bi ^= (8 - bpp);
            do {
                uint32_t word = *in++;
                int bitpos = 32;
                do {
                    uint16_t color = lcd.palette[word >> ((bitpos -= bpp) ^ bi) & mask];
                    *out++ = color + (color & 0xFFE0) + (color >> 10 & 0x20);
                } while (bitpos != 0);
            } while (--words != 0);
        } else if (mode == 4) {
            uint32_t i, bi = lcd.control >> 9 & 1;
            for (i = 0; i < 320; i++) {
                uint16_t color = ((uint16_t *)in)[i ^ bi];
                out[i] = color + (color & 0xFFE0) + (color >> 10 & 0x20);
            }
            in += 160;
        } else if (mode == 5) {
            // 32bpp mode: Convert 888 to 565
            do {
                uint32_t word = *in++;
                *out++ = (word >> 8 & 0xF800) | (word >> 5 & 0x7E0) | (word >> 3 & 0x1F);
            } while (--words != 0);
        } else {
            if (!(lcd.control & (1 << 9))) {
                memcpy(out, in, 320*2);
                in += 160;
            } else {
                uint32_t *outw = (uint32_t *)out;
                do {
                    uint32_t word = *in++;
                    *outw++ = word << 16 | word >> 16;
                } while (--words != 0);
            }
        }
    }
}

uint8_t lcd_read(const uint16_t pio)
{
  uint16_t index = pio;
  uint8_t bit_offset = (index&3)<<3;
  uint8_t low_index = mask8(index);

  switch( upper16(index) ) {
    case 0x0:
      switch( low_index>>4 ) {
        case 0x0:
          switch( low_index ) {
            case 0x00: case 0x01: case 0x02: case 0x03:
              return read8(lcd.timing0,bit_offset);
            case 0x04: case 0x05: case 0x06: case 0x07:
              return read8(lcd.timing1,bit_offset);
            case 0x08: case 0x09: case 0x0A: case 0x0B:
              return read8(lcd.timing2,bit_offset);
            case 0x0C: case 0x0D: case 0x0E: case 0x0F:
              return read8(lcd.timing3,bit_offset);
          }
        case 0x1:
          switch( low_index ) {
            case 0x10: case 0x11: case 0x12: case 0x13:
              return read8(lcd.upbase,bit_offset);
            case 0x14: case 0x15: case 0x16: case 0x17:
              return read8(lcd.lpbase,bit_offset);
            case 0x18: case 0x19: case 0x1A: case 0x1B:
              return read8(lcd.control,bit_offset);
            case 0x1C: case 0x1D: case 0x1E: case 0x1F:
              return read8(lcd.imsc,bit_offset);
          }
        default:
          switch( low_index ) {
            case 0x20: case 0x21: case 0x22: case 0x23:
              return read8(lcd.ris,bit_offset);
            case 0x24: case 0x25: case 0x26: case 0x27:
              return read8(lcd.mis,bit_offset);
            case 0x28: case 0x29: case 0x2A: case 0x2B:
            /* LCDIcr */ return 0;
            case 0x2C: case 0x2D: case 0x2E: case 0x2F:
              return read8(lcd.upcurr,bit_offset);
            case 0x30: case 0x31: case 0x32: case 0x33:
              return read8(lcd.lpcurr,bit_offset);
            break;
          }
      }
    case 0x0C:
    default:
      index -= 0x200;
      if(index < 0x200) {
          return read8(lcd.palette[index],(index&1)<<3);
      }
      break;
  }
  return 0;
}

void lcd_write(const uint16_t pio, const uint8_t byte)
{
  uint16_t index = pio;
  uint8_t bit_offset = (pio&3)<<3;
  uint8_t low_index = mask8(index);

  switch( upper16(index) ) {
    case 0x0:
      switch( low_index>>4 ) {
        case 0x0:
          switch( low_index ) {
            case 0x00: case 0x01: case 0x02: case 0x03:
              write8(lcd.timing0,bit_offset,byte); return;
            case 0x04: case 0x05: case 0x06: case 0x07:
              write8(lcd.timing1,bit_offset,byte); return;
            case 0x08: case 0x09: case 0x0A: case 0x0B:
              write8(lcd.timing2,bit_offset,byte); return;
    case 0x0C: case 0x0D: case 0x0E: case 0x0F:
              write8(lcd.timing3,bit_offset,byte); return;
          }
        case 0x1:
          switch( low_index ) {
            case 0x10: case 0x11: case 0x12: case 0x13:
              write8(lcd.upbase,bit_offset,byte); return;
            case 0x14: case 0x15: case 0x16: case 0x17:
              write8(lcd.lpbase,bit_offset,byte); return;
            case 0x18: case 0x19: case 0x1A: case 0x1B:
              write8(lcd.control,bit_offset,byte); return;
            case 0x1C: case 0x1D: case 0x1E: case 0x1F:
              write8(lcd.imsc,bit_offset,byte); return;
          }
        default:
          switch( low_index ) {
            case 0x20: case 0x21: case 0x22: case 0x23:
              write8(lcd.ris,bit_offset,byte); return;
            case 0x24: case 0x25: case 0x26: case 0x27:
              write8(lcd.mis,bit_offset,byte); return;
            case 0x28: case 0x29: case 0x2A: case 0x2B:
            /*LcdIcr*/ return;
            case 0x2C: case 0x2D: case 0x2E: case 0x2F:
              write8(lcd.upcurr,bit_offset,byte); return;
            case 0x30: case 0x31: case 0x32: case 0x33:
              write8(lcd.lpcurr,bit_offset,byte); return;
            default:
              break;
          }
      }
    case 0xC:
      return;
    default:
      index -= 0x200;
      if(index < 0x200) {
          write8(lcd.palette[index],(index&1)<<3,byte); return;
      }
      return;
  }
}

void lcd_init(void) {
  lcd.control = 0x92D;
  lcd.upbase = 0xD40000;
  lcd.memory = &mem;
}

static const eZ80portrange_t device = { .read_in = lcd_read, .write_out = lcd_write };

eZ80portrange_t init_lcd(void) {
    return device;
}

#include "core/controlport.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "asic.h"

// Global CONTROL state
control_state_t control;

// Read from the 0x0000 range of ports
uint8_t control_read(const uint16_t pio) {
  uint8_t addr = pio & 0x7F;

  uint8_t read_byte;

  switch (addr) {
  case 0x01:
             return control.cpu_speed & 0b00010011;
             break;
  case 0x02:
             read_byte = control.ports[addr] | 1;
             break;
  case 0x03:
             read_byte = control.device_type;
             break;
  case 0x0B:
             if( (control.ports[0x0A] & 2) == 0 ) {
                    control.ports[addr] |= 2;
             }
             read_byte = control.ports[addr];
             break;
  case 0x0F:
             read_byte = control.ports[addr];
             if(control.unknown_g_Xb != 0x00) { addr |= 0x80; }
             if(control.unknown_g_Bd != 0x00) { addr |= 0x40; }
             break;
  default:
           read_byte = control.ports[addr];
           break;
  }
  return read_byte;
}

// Write to the 0x0000 range of ports
void control_write(const uint16_t pio, const uint8_t byte)
{
  uint8_t addr = pio & 0x7F;

  switch (addr) {
  case 0x00:
             control.ports[addr] = byte;

             switch (control.unknown_flag_0) {
                 case 2:
                    control.unknown_flag_0 = (byte == 0x83 ? 3 : 0);
                    break;
                 case 6:
                    control.unknown_flag_0 = (byte == 0x03 ? 7 : 0);
                    break;
                 case 8:
                    control.unknown_flag_0 = (byte == 0x83 ? 9 : 0);
                    break;
                 case 9:
                    control.unknown_flag_0 = (byte == 0x03 ? 10 : 0);
                    break;
             }

             /* switch (byte & 3) {
                 case 0:
                     this.oa = 1;
                     break;
                 case 1:
                     this.oa = 2;
                     break;
                 case 2:            // TI originally put 16 (and after that, 17)
                     this.oa = 3;
                     break;
                 case 3:
                     this.oa = 4;
             }

             this.Wa = 60 * this.pb * this.oa;
             this.Xa = 60 * this.pb * this.oa * 84 / 100 >> 0;
             this.Ya = 60 * this.pb * this.oa * 92 / 100 >> 0; // looks lime timings things to me

             if ((byte & 32) != 0)
             {
                 control.ports[0x5000] |= 0x80;
             }

             if ((control.ports[0x5004] & 0x80) != 0)
             {
                 control.ports[0x5014] |= 0x80);
             } */

             break;
  case 0x01:
             control.cpu_speed = byte & 0b00010011;
             switch(control.cpu_speed&3) {
                 case 0:
                         asic_set_clock_rate(6000000); // 6 MHz
                         break;
                 case 1:
                         asic_set_clock_rate(12000000); // 12 MHz
                         break;
                 case 2:
                         asic_set_clock_rate(24000000); // 24 MHz
                         break;
                 case 3:
                         asic_set_clock_rate(48000000); // 48 MHz
                         break;
                 default:
                         break;
             }
             break;
  case 0x02:
             control.ports[addr] = 0;
             break;
  case 0x04:
  case 0x05:
  case 0x08:
  case 0x1D:
  case 0x1E:
  case 0x1F:
             control.ports[addr] = byte;
             break;
  case 0x06:
             control.ports[addr] = byte & 7;
             break;
  case 0x07:
             if (control.unknown_flag_0 == 0)
             {
                 if ((byte & 0x90) == 0x90)
                 {
                    control.unknown_flag_0 = 1;
                 }
             } else {
                control.unknown_flag_0 = 0;
             }
             control.ports[addr] = byte;
  case 0x09:
            switch (control.unknown_flag_0)
            {
                case 1:
                    control.unknown_flag_0 = ((byte & 0x80) != 0x00) ? 2 : 0;
                    break;
                case 4:
                    control.unknown_flag_0 = ((byte & 0x90) == 0x90) ? 5 : 0;
                    break;
                case 5:
                    control.unknown_flag_0 = ((byte & 0x10) == 0x00) ? 6 : 0;
                    break;
                case 7:
                    control.unknown_flag_0 = ((byte & 0x80) == 0x00) ? 8 : 0;
                    break;
                default:
                    break;
            }
             break;
  case 0x0A:
             if( control.unknown_flag_0 == 3) {
                    control.unknown_flag_0 = (byte & 1) ? 4 : 0;
             }
             control.ports[addr] = byte;
             break;
  case 0x0B:
  case 0x0C:
             control.unknown_flag_0 = 0;
             control.ports[addr] = byte;
             break;
  case 0x0E:
             control.ports[addr] = byte;
             break;
  case 0x0D:
             control.ports[addr] = (byte & 0xF) << 4 | (byte & 0xF);
             break;
  case 0x0F:
             control.ports[addr] = byte & 3;
             break;
  case 0x20:
  case 0x21:
  case 0x22:
  case 0x23:
  case 0x24:
  case 0x25:
             control.ports[addr] = byte;
             break;
  case 0x28:
             control.ports[addr] = byte & 247;
             break;
  default:
           break;
  }
}

void free_control(void *_state) {
    control_state_t* state = (control_state_t*)_state;
    free(state);
}

eZ80portrange_t init_control() {
    int i;
    // Initialize device to default state
    for(i = 0; i<0x80; i++) {
        control.ports[i] = 0;
    }
    control.ports[0x00] = 0x03; // From WikiTI
    control.ports[0x01] = 0x03; // From WikiTI
    control.ports[0x02] = 0x01; // Probably right
    control.ports[0x05] = 0x76; // From WikiTI
    control.ports[0x06] = 0x03; // From WikiTI
    control.ports[0x07] = 0xB7; // From WikiTI
    control.ports[0x08] = 0xFD; // Does not compare to WikiTI's 0x7F
    control.ports[0x0B] = 0x08; // Does not compare to WikiTI's 0xFC
    control.ports[0x0C] = 0x00; // From WikiTI
    control.ports[0x0D] = 0xFF; // From WikiTI
    control.ports[0x0E] = 0x0A; // Good
    control.ports[0x0F] = 0x42; // Good
    control.ports[0x1C] = 0x80; // From WikiTI
    control.ports[0x1F] = 0x01; // Does not compare to WikiTI's 0x42
    control.ports[0x22] = 0xD0; // Probably right
    control.ports[0x23] = 0xFF; // Probably right
    control.ports[0x24] = 0xFF; // Probably right
    control.ports[0x25] = 0xD3; // Probably right
    control.ports[0x29] = 0x00; // From WikiTI
    control.ports[0x2A] = 0x70; // Good
    control.ports[0x2B] = 0xFE; // Good
    control.ports[0x2C] = 0xFF; // Probably right
    control.ports[0x30] = 0xFF; // Probably right
    control.ports[0x34] = 0x30; // Probably right
    control.ports[0x35] = 0x03; // Probably right
    control.ports[0x3A] = 0xFF; // Probably right
    control.ports[0x3B] = 0xFF; // Probably right
    control.ports[0x3C] = 0xDF; // Probably right

    eZ80portrange_t device = { control_read, control_write };
    return device;
}

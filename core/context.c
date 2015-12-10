#include "core/context.h"
#include "core/emu.h"
#include <stdio.h>

// Global CONTEXT state
execution_context_t context;

void context_init(void) {
    context.cpu = &cpu;
    gui_console_printf("Initialized execution context...\n");
}

uint8_t is_read_next_byte(void) {
  uint8_t b;

  if(context.cpu->ADL == 0) {
      context.cpu->registers.PC&=0xFFFF;
      context.cpu->registers.PC|=context.cpu->registers.MBASE<<16;
  }
  b = cpu_read_byte(context.cpu->registers.PC);
  context.cpu->registers.PC++;
  if(context.cpu->ADL == 0) {
      context.cpu->registers.PC&=0xFFFF;
      context.cpu->registers.PC|=context.cpu->registers.MBASE<<16;
  }
  return b;
}
int8_t is_read_next_signed_byte(void) {
  int8_t s = (int8_t)cpu_read_byte(context.cpu->registers.PC);

  context.cpu->registers.PC++;
  if(context.cpu->ADL == 0) {
      context.cpu->registers.PC&=0xFFFF;
      context.cpu->registers.PC|=context.cpu->registers.MBASE<<16;
  }
  return s;
}
uint32_t is_read_next_word(void) {
  uint32_t w = cpu_read_word(context.cpu->registers.PC);
  context.cpu->registers.PC += 2;
  return w;
}
uint8_t il_read_next_byte(void) {
  uint8_t b = cpu_read_byte(context.cpu->registers.PC);

  context.cpu->registers.PC++; context.cpu->registers.PC &= 0xFFFFFF;
  return b;
}
int8_t il_read_next_signed_byte(void) {
  int8_t s = (int8_t)cpu_read_byte(context.cpu->registers.PC);

  context.cpu->registers.PC++; context.cpu->registers.PC&=0xFFFFFF;
  return s;
}
uint32_t il_read_next_word(void) {
  uint32_t w = cpu_read_word(context.cpu->registers.PC);

  context.cpu->registers.PC += 3; context.cpu->registers.PC&=0xFFFFFF;
  return w;
}

void set_il_context(void) {
  context.nu = il_read_next_byte;
  context.ns = il_read_next_signed_byte;
  context.nw = il_read_next_word;
}
void set_is_context(void) {
  context.nu = is_read_next_byte;
  context.ns = is_read_next_signed_byte;
  context.nw = is_read_next_word;
}
void set_context(uint8_t mode) {
  mode ? set_il_context() : set_is_context();
}

uint8_t HorIHr(void) {
    if (context.cpu->prefix >> 8 == 0xDD) {
        return context.cpu->registers.IXH;
    } else if (context.cpu->prefix >> 8 == 0xFD) {
        return context.cpu->registers.IYH;
    } else {
        return context.cpu->registers.H;
    }
}
uint8_t HorIHw(const uint8_t value) {
    if (context.cpu->prefix >> 8 == 0xDD) {
        context.cpu->registers.IXH = value;
    } else if (context.cpu->prefix >> 8 == 0xFD) {
            context.cpu->registers.IYH = value;
    } else {
            context.cpu->registers.H = value;
    }
    return value;
}

uint8_t LorILr(void) {
    if (context.cpu->prefix >> 8 == 0xDD) {
            return context.cpu->registers.IXL;
    } else if (context.cpu->prefix >> 8 == 0xFD) {
            return context.cpu->registers.IYL;
    } else {
            return context.cpu->registers.L;
    }
}

uint8_t LorILw(const uint8_t value) {
    if (context.cpu->prefix >> 8 == 0xDD) {
            context.cpu->registers.IXL = value;
    } else if (context.cpu->prefix >> 8 == 0xFD) {
            context.cpu->registers.IYL = value;
    } else {
            context.cpu->registers.L = value;
    }
    return value;
}

uint32_t HLorIr(void) {
    if(context.cpu->S) {
        if (context.cpu->prefix >> 8 == 0xDD) {
            return context.cpu->registers.IX&0xFFFF;
        } else if (context.cpu->prefix >> 8 == 0xFD) {
            return context.cpu->registers.IY&0xFFFF;
        } else {
            return context.cpu->registers.HL&0xFFFF;
        }
    } else {
        if (context.cpu->prefix >> 8 == 0xDD) {
            return context.cpu->registers.IX&0xFFFFFF;
        } else if (context.cpu->prefix >> 8 == 0xFD) {
            return context.cpu->registers.IY&0xFFFFFF;
        } else {
            return context.cpu->registers.HL&0xFFFFFF;
        }
    }
}

uint32_t HLorIw(const uint32_t value) {
    if (context.cpu->prefix >> 8 == 0xDD) {
        context.cpu->registers.IX = value;
    } else if (context.cpu->prefix >> 8 == 0xFD) {
        context.cpu->registers.IY = value;
    } else {
        context.cpu->registers.HL = value;
    }
    return value;
}

uint8_t indHLorIr(void) {
    // This function erases the prefix early so that the next read (H or L) does not
    // use IXH or IXL
    if (context.cpu->prefix >> 8 == 0xDD) {
        context.cycles += 4;
        context.cpu->prefix = 0;
        context.cpu->registers.WZ = context.cpu->registers.IX + context.ns();
        return cpu_read_byte(context.cpu->registers.WZ);
    } else if (context.cpu->prefix >> 8 == 0xFD) {
        context.cycles += 4;
        context.cpu->prefix = 0;
        context.cpu->registers.WZ = context.cpu->registers.IY + context.ns();
        return cpu_read_byte(context.cpu->registers.WZ);
    } else {
        return cpu_read_byte(context.cpu->registers.HL);
    }
}

uint8_t indHLorIw(const uint8_t value) {
    if (context.cpu->prefix >> 8 == 0xDD) {
        context.cycles += 9;
        context.cpu->prefix = 0;
        context.cpu->registers.WZ = context.cpu->registers.IX + context.ns();
        cpu_write_byte(context.cpu->registers.WZ, value);
    } else if (context.cpu->prefix >> 8 == 0xFD) {
        context.cycles += 9;
        context.cpu->prefix = 0;
        context.cpu->registers.WZ = context.cpu->registers.IY + context.ns();
        cpu_write_byte(context.cpu->registers.WZ, value);
    } else {
        cpu_write_byte(context.cpu->registers.HL, value);
    }
    return value;
}

uint8_t read_reg(const int i) {
	switch (i) {
	case 0: return context.cpu->registers.B;
	case 1: return context.cpu->registers.C;
	case 2: return context.cpu->registers.D;
	case 3: return context.cpu->registers.E;
	case 4: return HorIHr();
	case 5: return LorILr();
	case 6:
		  context.cycles += 3;
		  if (context.cpu->prefix >> 8 == 0xDD) {
			  context.cycles += 8;
			  context.cpu->registers.WZ = context.cpu->registers.IX + context.ns();
			  return cpu_read_byte(context.cpu->registers.WZ);
		  } else if (context.cpu->prefix >> 8 == 0xFD) {
			  context.cycles += 8;
			  context.cpu->registers.WZ = context.cpu->registers.IY + context.ns();
			  return cpu_read_byte(context.cpu->registers.WZ);
		  } else {
			  return cpu_read_byte(context.cpu->registers.HL);
		  }
	  case 7: return context.cpu->registers.A;
	  }
	return 0; // This should never happen
}

uint8_t write_reg(const int i, const uint8_t value) {
	switch (i) {
	case 0: return context.cpu->registers.B = value;
	case 1: return context.cpu->registers.C = value;
	case 2: return context.cpu->registers.D = value;
	case 3: return context.cpu->registers.E = value;
	case 4: return HorIHw(value);
	case 5: return LorILw(value);
	case 6:
		context.cycles += 3;
		if (context.cpu->prefix >> 8 == 0xDD) {
			context.cycles += 4;
			context.cpu->registers.WZ = context.cpu->registers.IX + context.ns();
			cpu_write_byte(context.cpu->registers.WZ, value);
		} else if (context.cpu->prefix >> 8 == 0xFD) {
			context.cycles += 4;
			context.cpu->registers.WZ = context.cpu->registers.IY + context.ns();
			cpu_write_byte(context.cpu->registers.WZ, value);
		} else {
			cpu_write_byte(context.cpu->registers.HL, value);
		}
		return value;
	case 7: return context.cpu->registers.A = value;
	  }
	  return 0; // This should never happen
}
  
uint8_t read_write_reg(const int read, const int write) {
    if (write == 0x06 || read == 0x06) { // Reading from/writing to (IX/IY + n)
        uint8_t r;
        uint16_t old_prefix = context.cpu->prefix;
        if (write == 0x06) {
            context.cpu->prefix &= 0xFF;
        }
        r = read_reg(read);
        context.cpu->prefix = old_prefix;
        if (read == 0x06) {
            context.cpu->prefix &= 0xFF;
        }
        return write_reg(write, r);
    } else {
        return write_reg(write, read_reg(read));
    }
}

uint32_t read_rp(const int i) {
    if(context.cpu->S) {
        switch (i) {
        case 0: return context.cpu->registers.BC&0xFFFF;
        case 1: return context.cpu->registers.DE&0xFFFF;
        case 2: return HLorIr();
        case 3: return context.cpu->registers.SPS;
        }
        return 0; // This should never happen
    } else {
        switch (i) {
        case 0: return context.cpu->registers.BC&0xFFFFFF;
        case 1: return context.cpu->registers.DE&0xFFFFFF;
        case 2: return HLorIr();
        case 3: return context.cpu->registers.SPL&0xFFFFFF;
        }
        return 0; // This should never happen
    }
}

uint32_t write_rp(const int i, const uint32_t value) {
    if(context.cpu->S) {
        switch (i) {
        case 0: return context.cpu->registers.BC = value&0xFFFF;
        case 1: return context.cpu->registers.DE = value&0xFFFF;
        case 2: return HLorIw(value);
        case 3: return context.cpu->registers.SPS = value&0xFFFF;
        }
        return 0; // This should never happen
    } else {
        switch (i) {
        case 0: return context.cpu->registers.BC = value&0xFFFFFF;
        case 1: return context.cpu->registers.DE = value&0xFFFFFF;
        case 2: return HLorIw(value)&0xFFFFFF;
        case 3: return context.cpu->registers.SPL = value&0xFFFFFF;
        }
        return 0; // This should never happen
    }
}

uint32_t read_rp2(const int i) {
    if(context.cpu->S) {
        switch (i) {
        case 0: return context.cpu->registers.BC&0xFFFF;
        case 1: return context.cpu->registers.DE&0xFFFF;
        case 2: return HLorIr();
        case 3: return context.cpu->registers.AF&0xFFFF;
        }
        return 0; // This should never happen
    } else {
        switch (i) {
        case 0: return context.cpu->registers.BC&0xFFFFFF;
        case 1: return context.cpu->registers.DE&0xFFFFFF;
        case 2: return HLorIr()&0xFFFFFF;
        case 3: return context.cpu->registers.AF&0xFFFFFF;
        }
        return 0; // This should never happen
    }
}

uint32_t write_rp2(const int i, const uint32_t value) {
    if(context.cpu->S) {
        switch (i) {
        case 0: return context.cpu->registers.BC = value&0xFFFF;
        case 1: return context.cpu->registers.DE = value&0xFFFF;
        case 2: return HLorIw(value);
        case 3: return context.cpu->registers.AF = value&0xFFFF;
        }
        return 0; // This should never happen
    } else {
        switch (i) {
        case 0: return context.cpu->registers.BC = value&0xFFFFFF;
        case 1: return context.cpu->registers.DE = value&0xFFFFFF;
        case 2: return HLorIw(value)&0xFFFFFF;
        case 3: return context.cpu->registers.AF = value&0xFFFFFF;
        }
        return 0; // This should never happen
    }
}

uint8_t read_cc(const int i) {
    eZ80registers_t *r = &context.cpu->registers;
    switch (i) {
    case 0: return !r->flags.Z;
    case 1: return  r->flags.Z;
    case 2: return !r->flags.C;
    case 3: return  r->flags.C;
    case 4: return !r->flags.PV;
    case 5: return  r->flags.PV;
    case 6: return !r->flags.S;
    case 7: return  r->flags.S;
    }
    return 0; // This should never happen
}

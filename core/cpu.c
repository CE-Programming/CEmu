#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "core/emu.h"
#include "core/registers.h"
#include "core/cpu.h"
#include "core/context.h"

#define port_range(a) (((a)>>12)&0xF) // converts an address to a port range 0x0-0xF
#define addr_range(a) ((a)&0xFFF)     // converts an address to a port range value 0x0000-0xFFF
#define swap(a, b) do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while(0)

// Global CPU state
eZ80cpu_t cpu;

void cpu_init(void) {
    context_init();  // initilize execution context
    memset(&cpu, 0x00, sizeof(eZ80cpu_t));
    cpu.memory = &mem;
    gui_console_printf("Initialized CPU...\n");
}

uint8_t cpu_read_byte(const uint32_t address) {
    if(cpu.ADL) // address use MBASE
        return cpu.read_byte(address&0xFFFFFF);
    return cpu.read_byte((address&0xFFFF) | ((cpu.registers.MBASE)<<16));
}
uint32_t cpu_read_word(uint32_t address) {
    if(cpu.IS) // fetch 2 immediate bytes
        return cpu.read_byte(address) | cpu.read_byte(address+1)<<8;
    return cpu.read_byte(address) | cpu.read_byte(address+1)<<8 | cpu.read_byte(address+2)<<16;
}

void cpu_write_byte(uint32_t address, uint8_t value) {
    if(cpu.S) // address use MBASE
        cpu.write_byte( (address&0xFFFF) | ((cpu.registers.MBASE)<<16), value);
    cpu.write_byte( address&0xFFFFFF, value); // (cpu.L)
}
void cpu_write_word(uint32_t address, uint32_t value) {
    if(cpu.IS) { // write 2 bytes
        cpu.write_byte(address, (value) & 0xFF);
        cpu.write_byte(address + 1, (value >> 8) & 0xFF);
    } else { // write 3 bytes (cpu.IL)
        cpu.write_byte(address, (value) & 0xFF);
        cpu.write_byte(address + 1, (value >> 8) & 0xFF);
        cpu.write_byte(address + 2, (value >> 16) & 0xFF);
    }
}

void cpu_push(uint32_t value) {
    if(cpu.S) { // push 2 bytes
        cpu_write_word(cpu.registers.SPS - 2, value&0xFFFF);
        cpu.registers.SPS -= 2;
    } else { // push 3 bytes
        cpu_write_word(cpu.registers.SPL - 3, value&0xFFFFFF);
        cpu.registers.SPL -= 3; cpu.registers.SPL &= 0xFFFFFF;
    }
}
uint32_t cpu_pop(void) {
    uint32_t p;
    if(cpu.S) { // pop 2 bytes
        p = cpu_read_word(cpu.registers.SPS);
        cpu.registers.SPS += 2;
        return p;
    } else { // pop 3 bytes
        p = cpu_read_word(cpu.registers.SPL);
        cpu.registers.SPL += 3; cpu.registers.SPL &= 0xFFFFFF;
        return p;
    }
}

static void get_cntrl_data_blocks_format(void) {
    cpu.SUFFIX = 0;
    cpu.L = cpu.ADL;
    cpu.IL = cpu.ADL;
    cpu.S = !cpu.L;
    cpu.IS = !cpu.IL;
}

static void daa(void) {
	eZ80registers_t *r = &cpu.registers;
	uint8_t old = r->A;
	uint8_t v = 0;
	if ((r->A & 0xF) > 9 || r->flags.H) {
		v += 6;
	}
	if (((r->A + v) >> 4) > 9 || _flag_carry_b(r->A + v) || r->flags.C) {
		v += 0x60;
	}
	if (r->flags.N) {
		r->A -= v;
		r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			| _flag_undef(r->F) | _flag_parity(r->A)
			| _flag_subtract(r->flags.N) | __flag_c(v >= 0x60)
			| _flag_halfcarry_b_sub(old, v, 0);
	} else {
		r->A += v;
		r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			| _flag_undef(r->F) | _flag_parity(r->A)
			| _flag_subtract(r->flags.N) | __flag_c(v >= 0x60)
			| _flag_halfcarry_b_add(old, v, 0);
	}
}

static void execute_alu(int i, uint8_t v) {
	uint8_t old;
	eZ80registers_t *r = &cpu.registers;
	context.cycles += 4;
	switch (i) {
	  case 0: // ADD A, v
		  old = r->A;
		  r->A += v;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_overflow_b_add(old, v, r->A)
			  | _flag_subtract(0) | _flag_carry_b(old + v)
			  | _flag_halfcarry_b_add(old, v, 0);
		  break;
	  case 1: // ADC A, v
		  old = r->A;
		  r->A += v + r->flags.C;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_overflow_b_add(old, v, r->A)
			  | _flag_subtract(0) | _flag_carry_b(old + v + r->flags.C)
			  | _flag_halfcarry_b_add(old, v, r->flags.C);
		  break;
	  case 2: // SUB v
		  old = r->A;
		  r->A -= v;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_overflow_b_sub(old, v, r->A)
			  | _flag_subtract(1) | _flag_carry_b(old - v)
			  | _flag_halfcarry_b_sub(old, v, 0);
		  break;
	  case 3: // SBC v
		  old = r->A;
		  r->A -= v + r->flags.C;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_overflow_b_sub(old, v, r->A)
			  | _flag_subtract(1) | _flag_carry_b(old - v - r->flags.C)
			  | _flag_halfcarry_b_sub(old, v, r->flags.C);
		  break;
	  case 4: // AND v
		  old = r->A;
		  r->A &= v;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_parity(r->A)
			  | FLAG_H;
		  break;
	  case 5: // XOR v
		  old = r->A;
		  r->A ^= v;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_parity(r->A);
		  break;
	  case 6: // OR v
		  old = r->A;
		  r->A |= v;
		  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
			  | _flag_undef(r->F) | _flag_parity(r->A);
		  break;
	  case 7: // CP v
		  old = r->A - v;
		  r->F = _flag_sign_b(old) | _flag_zero(old)
			  | _flag_undef(r->F) | _flag_subtract(1)
			  | _flag_carry_b(r->A - v)
			  | _flag_overflow_b_sub(r->A, v, old)
			  | _flag_halfcarry_b_sub(r->A, v, 0);
		  break;
	}
}

static void execute_rot(int y, int z, int switch_opcode_data) {
	uint8_t r;
	uint8_t old_7;
	uint8_t old_0;
	uint8_t old_c;
	eZ80registers_t *reg;

	r = read_reg(z);
	if (z == 6 && switch_opcode_data) {
		// reset the PC back to the offset, so
		// the write reads it correctly
		context.cpu->registers.PC--;
		mask_mode(context.cpu->registers.PC, context.cpu->ADL);
	}

	//uint8_t old_r = r;
	old_7 = (r & 0x80) > 0;
	old_0 = (r & 0x01) > 0;
	old_c = context.cpu->registers.flags.C > 0;

	reg = &cpu.registers;
	switch (y) {
	case 0: // RLC r[z]
		r <<= 1; r |= old_7;
		write_reg(z, r);
		reg->F = __flag_c(old_7) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 1: // RRC r[z]
		r >>= 1; r |= old_0 << 7;
		write_reg(z, r);
		reg->F = __flag_c(old_0) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 2: // RL r[z]
		r <<= 1; r |= old_c;
		write_reg(z, r);
		reg->F = __flag_c(old_7) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 3: // RR r[z]
		r >>= 1; r |= old_c << 7;
		write_reg(z, r);
		reg->F = __flag_c(old_0) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 4: // SLA r[z]
		r <<= 1;
		write_reg(z, r);
		reg->F = __flag_c(old_7) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 5: // SRA r[z]
		r >>= 1;
		r |= old_7 << 7;
		write_reg(z, r);
		reg->F = __flag_c(old_0) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 6: // SLL r[z]
		r <<= 1; r |= 1;
		write_reg(z, r);
		reg->F = __flag_c(old_7) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	case 7: // SRL r[z]
		r >>= 1;
		write_reg(z, r);
		reg->F = __flag_c(old_0) | _flag_sign_b(r) | _flag_parity(r)
			| _flag_undef(reg->F) | _flag_zero(r);
		break;
	}
}

static void execute_rot_acc(int y)
{
 eZ80registers_t *r = &cpu.registers;
 uint8_t old;
 switch (y)
 {
    case 0: // RLCA
           context.cycles += 1;
           old = (r->A & 0x80) > 0;
           r->flags.C = old;
           r->A <<= 1;
           r->A |= old;
           r->flags.N = r->flags.H = 0;
           break;
    case 1: // RRCA
           context.cycles += 1;
           old = (r->A & 1) > 0;
           r->flags.C = old;
           r->A >>= 1;
           r->A |= old << 7;
           r->flags.N = r->flags.H = 0;
         break;
    case 2: // RLA
           context.cycles += 1;
           old = r->flags.C;
           r->flags.C = (r->A & 0x80) > 0;
           r->A <<= 1;
           r->A |= old;
           r->flags.N = r->flags.H = 0;
           break;
    case 3: // RRA
           context.cycles += 1;
           old = r->flags.C;
           r->flags.C = (r->A & 1) > 0;
           r->A >>= 1;
           r->A |= old << 7;
           r->flags.N = r->flags.H = 0;
           break;
    case 4: // DAA
           context.cycles += 1;
           old = r->A;
           daa();
           break;
    case 5: // CPL
           context.cycles += 1;
           r->A = ~r->A;
           r->flags.N = r->flags.H = 1;
           break;
    case 6: // SCF
           context.cycles += 1;
           r->flags.C = 1;
           r->flags.N = r->flags.H = 0;
           break;
    case 7: // CCF
           context.cycles += 1;
           r->flags.H = r->flags.C;
           r->flags.C = !r->flags.C;
           r->flags.N = 0;
           break;
  }
}

static void execute_bli(int y, int z) {
	eZ80registers_t *r = &cpu.registers;
	eZ80portrange_t portr;
	uint8_t old = 0, new = 0;

	switch (y) {
	case 0:
		switch (z) {
		case 2: // INIM
			context.cycles += 5;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.read_in != NULL) {
				old = portr.read_in(r->C);
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		case 3: // OTIM
			context.cycles += 5;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(r->C, old);  // c->byte in 0x0000 range
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);  // undefined bits are reset to 0
			break;
		case 4: // INI2 -- Same as INI, C incremented
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		}
		break;
	case 1:
		switch (z) {
		case 2: // INDM
			context.cycles += 5;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.read_in != NULL) {
				old = portr.read_in(r->C);
				cpu_write_byte(r->HL, old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->C--;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		case 3: // OTDM
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(r->C, old);  // c->byte in 0x0000 range
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->C--;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		case 4: // IND2 -- Same as IND, C decremented
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->BC-=0x0101; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		}
		break;
	case 2:
		switch (z) {
		case 2: // INIMR
			context.cycles += 2;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.read_in != NULL) {
				old = portr.read_in(r->C);
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 3: // OTIMR
			context.cycles += 2;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(r->C, old);  // c->byte in 0x0000 range
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 4: // INI2R
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->DE)];  // output to the 0x0000 port range
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->DE));
				cpu_write_byte(r->HL, old);
			}
			r->DE++; mask_mode(r->DE, cpu.L);
			r->HL++; mask_mode(r->HL, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->BC) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		}
		break;
	case 3:
		switch (z) {
		case 2: // INDMR -- C is decremented
			context.cycles += 2;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.read_in != NULL) {
				old = portr.read_in(r->C);
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C--;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 3: // OTDMR
			context.cycles += 2;
			portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(r->C, old);  // c->byte in 0x0000 range
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C--;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 4: // IND2R
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->BC-=0x0101; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->BC) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		}
		break;
	case 4:
		switch (z) {
		case 0: // LDI
			context.cycles += 5;
			old = cpu_read_byte(r->HL);
			r->HL++; mask_mode(r->HL, cpu.L);
			cpu_write_byte(r->DE, old);
			r->DE++; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A + old;
			r->flags.PV = r->BC != 0;
			r->flags.N = 0;
			break;
		case 1: // CPI
			context.cycles += 3;
			old = cpu_read_byte(r->HL);
			r->HL++; mask_mode(r->HL, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A - old;
			r->F = _flag_sign_b(new) | _flag_zero(new)
				| _flag_halfcarry_b_sub(r->A, old, 0) | __flag_pv(r->BC)
				| _flag_subtract(1) | __flag_c(r->flags.C)
				| _flag_undef(r->F);
			break;
		case 2: // INI
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		case 3: // OUTI
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		case 4: // OUTI2 -- Exactly like OUTI, just also increments C
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->C++;
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			break;
		}
		break;
	case 5:
		switch (z) {
		case 0: // LDD
			context.cycles += 12;
			old = cpu_read_byte(r->HL);
			r->HL--; mask_mode(r->HL, cpu.L);
			cpu_write_byte(r->DE, old);
			r->DE--; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A + old;
			r->flags.PV = r->BC != 0;
			r->flags.N = 0;
			break;
		case 1: // CPD
			context.cycles += 3;
			old = cpu_read_byte(r->HL);
			r->HL--; mask_mode(r->HL, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A - old;
			r->F = _flag_sign_b(new) | _flag_zero(new)
				| _flag_halfcarry_b_sub(r->A, old, 0)
				| __flag_pv(r->BC)
				| _flag_subtract(1) | __flag_c(r->flags.C)
				| _flag_undef(r->F);
			break;
		case 2: // IND
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old);
			break;
		case 3: // OUTD
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old);
			break;
		case 4: // OUTD2 -- Same as OUTD, C decrements
			context.cycles += 5;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->BC-=0x0101; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old);
			break;
		}
		break;
	case 6:
		switch (z) {
		case 0: // LDIR
			context.cycles += 2;
			old = cpu_read_byte(r->HL);
			cpu_write_byte(r->DE, old);
			r->HL++; mask_mode(r->HL, cpu.L);
			r->DE++; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A + old;
			r->flags.PV = r->BC != 0;
			r->flags.N = 0;
			if (r->BC) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 1: // CPIR
			context.cycles += 1;
			old = cpu_read_byte(r->HL);
			r->HL++; mask_mode(r->HL, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A - old;
			r->F = _flag_sign_b(new) | _flag_zero(new)
				| _flag_halfcarry_b_sub(r->A, old, 0) | __flag_pv(r->BC)
				| _flag_subtract(1) | __flag_c(r->flags.C)
				| _flag_undef(r->F);
			if (r->BC && !r->flags.Z) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 2: // INIR
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 3: // OTIR
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 4: // OTI2R
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->DE)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL++; mask_mode(r->HL, cpu.L);
			r->DE++; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->BC) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		}
		break;
	case 7:
		switch (z) {
		case 0: // LDDR
			context.cycles += 4;
			old = cpu_read_byte(r->HL);
			r->HL--; mask_mode(r->HL, cpu.L);
			cpu_write_byte(r->DE, old);
			r->DE--; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A + old;
			r->flags.PV = r->BC != 0;
			r->flags.N = 0;
			if (r->BC) {
				context.cycles += 5;
				r->PC -= 2;
			}
			break;
		case 1: // CPDR
			context.cycles += 1;
			old = cpu_read_byte(r->HL);
			r->HL--; mask_mode(r->HL, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			new = r->A - old;
			r->F = _flag_sign_b(new) | _flag_zero(new)
				| _flag_halfcarry_b_sub(r->A, old, 0) | __flag_pv(r->BC)
				| _flag_subtract(1) | __flag_c(r->flags.C)
				| _flag_undef(r->F);
			if (r->BC && !r->flags.Z) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 2: // INDR
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.read_in != NULL) {
				old = portr.read_in(addr_range(r->BC));
				cpu_write_byte(r->HL, old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 3: // OTDR
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->B--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->B) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->B) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		case 4: // OTD2R
			context.cycles += 2;
			portr = context.cpu->prange[port_range(r->BC)];
			if (portr.write_out != NULL) {
				old = cpu_read_byte(r->HL);
				portr.write_out(addr_range(r->BC), old);
			}
			r->HL--; mask_mode(r->HL, cpu.L);
			r->DE--; mask_mode(r->DE, cpu.L);
			r->BC--; mask_mode(r->BC, cpu.L);
			r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
			if (r->BC) {
				context.cycles += 3;
				r->PC -= 2;
			}
			break;
		}
		break;
	}
}

int cpu_execute(void) {
  eZ80portrange_t portr;

  // variable declaration
  int8_t s;
  uint8_t u;
  uint32_t w;

  uint8_t old = 0;
  uint32_t old_word;

  uint8_t new = 0;
  uint32_t new_word;

  uint32_t op_word;
  uint8_t old_r;

  int reset_prefix;

  eZ80registers_t *r = &cpu.registers;

  get_cntrl_data_blocks_format();

  while ((!exiting && cycle_count_delta < 0) || cpu.prefix != 0)
  {
    context.cycles = 0;

    if (cpu.IEF2 && !cpu.prefix)
    {
      if (cpu.IEF_wait)
      {
        cpu.IEF_wait = 0;
      } else {
        if (cpu.interrupt)
        {
          cpu.halted = 0;
          //handle_interrupt();
          goto exit_loop;
        }
      }
    }
    if (cpu.halted)  // has the CPU halted?
    {
      context.cycles += 1;
      goto exit_loop;
    }

    // first, initialize the execution context (ADL vs. Z80 mode)
    set_context(cpu.IL);

    // default = no prefix
    reset_prefix = 1;

    // fetch opcode

    context.opcode = context.nu();

    //gui_console_printf("Fetched Opcode: %02X\n", context.opcode);
    //system("pause");

    old_r = r->R;
    r->R++;
    r->R &= 0x7F;
    r->R |= old_r & 0x80;

    if ((cpu.prefix & 0xFF) == 0xCB) {
        int switch_opcode_data = cpu.prefix >> 8;
        if (switch_opcode_data) {
            context.opcode = cpu_read_byte(cpu.registers.PC--);
        }

        switch (context.x)
        {
        case 0: // rot[y] r[z]
                context.cycles += 1;
                execute_rot(context.y, context.z, switch_opcode_data);
                break;
        case 1: // BIT y, r[z]
                context.cycles += 2;
                old = read_reg(context.z);
                new = old & (1 << context.y);
                r->F = _flag_sign_b(new) | _flag_zero(new) | _flag_undef(r->F)
                       | _flag_parity(new) | __flag_c(r->flags.C)
                       | FLAG_H;
                break;
        case 2: // RES y, r[z]
               context.cycles += 2;
               old = read_reg(context.z);
               old &= ~(1 << context.y);
               if (context.z == 6 && switch_opcode_data) {
                  cpu.registers.PC--;
               }
               write_reg(context.z, old);
               break;
        case 3: // SET y, r[z]
               context.cycles += 2;
               old = read_reg(context.z);
               old |= 1 << context.y;
               if (context.z == 6 && switch_opcode_data) {
                   cpu.registers.PC--;
               }
               write_reg(context.z, old);
               break;
      }
      if (switch_opcode_data) {
          cpu.registers.PC++;
      }
    } else if (cpu.prefix >> 8 == 0xED) {
      switch (context.x)
      {
        case 0:
               switch (context.z)
               {
                 case 0:
                        if (context.y == 6) { // OPCODETRAP
                            context.cycles += 1;
                            cpu.IEF_wait = 1;
                            break;
                        }
                        // IN0 r[y], (n)
                        context.cycles += 4;
                        portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
                        if (portr.read_in != NULL) {
                            new = portr.read_in(context.nu());
                            write_reg(context.y, new);
                            r->F = _flag_sign_b(new) | _flag_zero(new)
                                | _flag_undef(r->F) | _flag_parity(new);
                        }
                        break;
                 case 1:
                        if (context.y == 6) { // LD IY, (HL)
                            r->IY = cpu_read_word(r->HL);
                            break;
                        }
                        // OUT0 (n), r[y]
                        context.cycles += 4;
                        portr = context.cpu->prange[0x00];  // output to the 0x0000 port range
                        if (portr.write_out != NULL) {
                            portr.write_out(context.nu(), read_reg(context.y));
                        }
                        break;
                 case 2: // LEA rp3[p], IX
                 case 3: // LEA rp3[p], IY
                        context.cycles += 3;
                        if (!context.q) {
                            write_rp3(context.p, (context.r ? r->IY : r->IX) + context.ns());
                            break;
                        }
                        context.cycles += 1; // OPCODETRAP
                        cpu.IEF_wait = 1;
                        break;
                 case 4: // TST A, r[y]
                        context.cycles += 2;
                        s = r->A & read_reg(context.y);
                        r->F = _flag_sign_b(s) | _flag_zero(s)
                                | _flag_undef(r->F) | _flag_parity(s)
                                | FLAG_H;
                        break;
                 case 5: // OPCODETRAP
                        context.cycles += 1;
                        cpu.IEF_wait = 1;
                        break;
                 case 6:
                        if (context.y == 7) { // LD (HL), IY
                            cpu_write_word(r->HL, r->IY);
                            break;
                        }
                        context.cycles += 1; // OPCODETRAP
                        cpu.IEF_wait = 1;
                        break;
                 case 7:
                        if (context.r) { // LD (HL), rp3[r]
                            cpu_write_word(r->HL, read_rp3(context.r));
                        } else { // LD rp3[r], (HL)
                            write_rp3(context.r, cpu_read_word(r->HL));
                        }
                        break;
               }
               break;
        case 1:
                switch (context.z)
                {
                 case 0:
                        if (context.y == 6) { // OPCODETRAP (ADL)
                            context.cycles += 1;
                            cpu.IEF_wait = 1;
                            break;
                        } else { // IN r[y], (BC)
                            context.cycles += 3;
                            portr = context.cpu->prange[port_range(r->BC)];  // output to the 0x0000 port range
                            if (portr.read_in != NULL) {
                                    new = portr.read_in(addr_range(r->BC));
                                    write_reg(context.y, new);
                                    r->F = _flag_sign_b(new) | _flag_zero(new)
                                            | _flag_undef(r->F) | _flag_parity(new);
                            }
                        }
                        break;
                 case 1:
                        if (context.y == 6) { // OPCODETRAP (ADL)
                          context.cycles += 1;
                          cpu.IEF_wait = 1;
                          break;
                        } else { // OUT (BC), r[y]
                          context.cycles += 3;
                          portr = context.cpu->prange[port_range(r->BC)];
                          if (portr.write_out != NULL) {
                                  portr.write_out(addr_range(r->BC), read_reg(context.y));
                          }
                        }
                        break;
                 case 2:
                        if (context.q == 0) { // SBC HL, rp[p]
                          context.cycles += 2;
                          old_word = r->HL;
                          op_word = read_rp(context.p);
                          r->HL -= op_word + r->flags.C;
                          mask_mode(r->HL, cpu.L);
                          r->F = _flag_sign_w(r->HL, cpu.L) | _flag_zero(r->HL)
                                 | _flag_undef(r->F) | _flag_overflow_w_sub(old_word, op_word, r->HL, cpu.L)
                                 | _flag_subtract(1) | _flag_carry_w(old_word - op_word - r->flags.C, cpu.L)
                                 | _flag_halfcarry_w_sub(old_word, op_word, r->flags.C);

                        } else { // ADC HL, rp[p]
                          context.cycles += 2;
                          old_word = r->HL;
                          op_word = read_rp(context.p);
                          r->HL += op_word + r->flags.C;
                          mask_mode(r->HL, cpu.L);
                          r->F = _flag_sign_w(r->HL, cpu.L) | _flag_zero(r->HL)
                                 | _flag_undef(r->F) | _flag_overflow_w_add(old_word, op_word, r->HL, cpu.L)
                                 | _flag_subtract(0) | _flag_carry_w(old_word + op_word + r->flags.C, cpu.L)
                                 | _flag_halfcarry_w_add(old_word, op_word, r->flags.C);
                        }
                        break;
                 case 3:
                        if (context.q == 0) { // LD (nn), rp[p]
                          context.cycles += 3;
                          r->WZ = context.nw();
                          cpu_write_word(r->WZ, read_rp(context.p));
                        } else { // LD rp[p], (nn)
                          context.cycles += 3;
                          r->WZ = context.nw();
                          write_rp(context.p, cpu_read_word(r->WZ));
                        }
                        break;
                 case 4:
                        if (context.q == 0)
                        {
                          switch (context.p)
                          {
                            case 0:  // NEG
                                   context.cycles += 2;
                                   old = r->A;
                                   r->A = -r->A;
                                   new = r->A;
                                   r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
                                          | _flag_undef(r->F) | __flag_pv(old == 0x80)
                                          | _flag_subtract(1) | __flag_c(old != 0)
                                          | _flag_halfcarry_b_sub(0, old, 0);
                                   break;
                            case 1:  // LEA IX, IY + d
                                   context.cycles += 3;
                                   s = context.ns();
                                   r->IX = r->IY + s;
                                   mask_mode(r->IX, cpu.L);
                                   break;
                            case 2:  // TST A,n
                                   context.cycles += 2;
                                   u = context.nu();
                                   old = r->A;
                                   new = r->A & u;
                                   old = r->A;
                                   r->F = _flag_sign_b(new) | _flag_zero(new)
                                          | _flag_undef(r->F) | _flag_parity(new)
                                          | FLAG_H;
                                   break;
                            case 3:  // TSTIO n
                                   // UNIMPLEMENTED
                                   break;
                          }
                        }
                        else // MLT rp[p]
                        {
                          context.cycles += 6;
                          old_word = read_rp(context.p);
                          new_word = (old_word&0xFF) * ((old_word>>8)&0xFF);
                          write_rp(context.p, new_word);
                          break;
                        }
                        break;
                 case 5:
                        switch (context.y)
                        {
                            case 0: // RETN
                                   // Note: Does not implement non-maskable interrupts
                                   context.cycles += 6;
                                   if (cpu.SUFFIX) {
                                       w = cpu_read_byte(r->SPL++) & 1;
                                       if (cpu.ADL) {
                                           r->PCL = cpu_read_byte(r->SPL++);
                                           r->PCH = cpu_read_byte(r->SPL++);
                                       } else {
                                           r->PCL = cpu_read_byte(r->SPS++);
                                           r->PCH = cpu_read_byte(r->SPS++);
                                       }
                                       if (w) {
                                           r->PCU = cpu_read_byte(r->SPL++);
                                           if (!cpu.L && !cpu.ADL) {
                                               r->PCU = 0;
                                           }
                                       }
                                       cpu.ADL = w;
                                   } else {
                                       r->PC = cpu_pop();
                                   }
                                   break;
                            case 1: // RETI
                                   // Note: Does not implement non-maskable interrupts
                                   context.cycles += 6;
                                   if (cpu.SUFFIX) {
                                       w = cpu_read_byte(r->SPL++) & 1;
                                       if (cpu.ADL) {
                                           r->PCL = cpu_read_byte(r->SPL++);
                                           r->PCH = cpu_read_byte(r->SPL++);
                                       } else {
                                           r->PCL = cpu_read_byte(r->SPS++);
                                           r->PCH = cpu_read_byte(r->SPS++);
                                       }
                                       if (w) {
                                           r->PCU = cpu_read_byte(r->SPL++);
                                           if (!cpu.L && !cpu.ADL) {
                                               r->PCU = 0;
                                           }
                                       }
                                       cpu.ADL = w;
                                   } else {
                                       r->PC = cpu_pop();
                                   }
                                   break;
                            case 2: // LEA IY, IX + d
                                   context.cycles += 3;
                                   s = context.ns();
                                   r->IY = r->IX + s;
                                   mask_mode(r->IY, cpu.L);
                                   break;
                            case 3:
                            case 6: // OPCODETRAP
                                   context.cycles += 1;
                                   cpu.IEF_wait = 1;
                                   break;
                            case 4: // PEA IX + d
                                   context.cycles += 6;
                                   s = context.ns();
                                   r->IX += s;
                                   mask_mode(r->IX, cpu.L);
                                   r->WZ = r->IX;
                                   cpu_push(r->WZ);
                                   break;
                            case 5: // LD MB,A
                                   context.cycles += 2;
                                   r->MBASE = r->A;
                                   break;
                            case 7: // STMIX
                                   context.cycles += 2;
                                   cpu.MADL = 1;
                                   break;
                        }
                        break;
                 case 6: // IM im[y]
                        switch (context.y)
                        {
                          case 0:
                          case 2:
                          case 3: // IM im[y]
                                 context.cycles += 2;
                                 //execute_im(context.y);
                                 break;
                          case 1: // OPCODETRAP
                                 context.cycles += 1;
                                 cpu.IEF_wait = 1;
                                 break;
                          case 4: // PEA IY + d
                                 context.cycles += 6;
                                 s = context.ns();
                                 r->IY += s;
                                 mask_mode(r->IY, cpu.L);
                                 r->WZ = r->IY;
                                 cpu_push(r->WZ);
                                 break;
                          case 5: // LD A,MB
                                 if(cpu.ADL) {
                                     context.cycles += 2;
                                     r->A = r->MBASE;
                                     break;
                                 }
                                 context.cycles += 1;
                                 break;
                          case 6: // SLP -- NOT IMPLEMENTED
                                 context.cycles += 2;
                                 break;
                          case 7: // RSMIX
                                 context.cycles += 2;
                                 cpu.MADL = 0;
                                 break;
                        }
                        break;
                 case 7:
                        switch (context.y)
                        {
                          case 0: // LD I, A
                                 context.cycles += 2;
                                 r->I = r->A;
                                 break;
                          case 1: // LD R, A
                                 context.cycles += 2;
                                 r->R = r->A;
                                 break;
                          case 2: // LD A, I
                                  context.cycles += 2;
                                  r->A = r->I;
                                  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
                                         | _flag_undef(r->F) | __flag_pv(cpu.IEF2)
                                         | _flag_subtract(0) | __flag_c(r->flags.C);
                                  break;
                           case 3: // LD A, R
                                  context.cycles += 5;
                                  r->A = r->R;
                                  r->F = _flag_sign_b(r->A) | _flag_zero(r->A)
                                         | _flag_undef(r->F) | __flag_pv(cpu.IEF2)
                                         | _flag_subtract(0) | __flag_c(r->flags.C);
                                  break;
                           case 4: // RRD
                                  context.cycles += 14;
                                  old = r->A;
                                  new = cpu_read_byte(r->HL);
                                  r->A &= 0xF0;
                                  r->A |= new & 0x0F;
                                  new >>= 4;
                                  new |= old << 4;
                                  cpu_write_byte(r->HL, new);
                                  r->F = __flag_c(r->flags.C) | _flag_sign_b(r->A) | _flag_zero(r->A)
                                          | _flag_parity(r->A) | _flag_undef(r->F);
                                  break;
                           case 5: // RLD
                                  context.cycles += 14;
                                  old = r->A;
                                  new = cpu_read_byte(r->HL);
                                  r->A &= 0xF0;
                                  r->A |= new >> 4;
                                  new <<= 4;
                                  new |= old & 0x0F;
                                  cpu_write_byte(r->HL, new);
                                  r->F = __flag_c(r->flags.C) | _flag_sign_b(r->A) | _flag_zero(r->A)
                                          | _flag_parity(r->A) | _flag_undef(r->F);
                                  break;
                          default: // OPCODETRAP
                                  context.cycles += 1;
                                  cpu.IEF_wait = 1;
                                  break;
                        }
                        break;
                }
                break;
         case 2:
                 if (context.y >= 0 && context.z <= 4) { // bli[y,z]
                   execute_bli(context.y, context.z);
                 } else { // OPCODETRAP
                   context.cycles += 1;
                   cpu.IEF_wait = 1;
                 }
                 break;
        case 3:  // There are only a few of these, so a simple switch for these shouldn't matter too much
                switch(context.opcode) {
                case 0xC2: // INIRX
                           context.cycles += 2;
                           portr = context.cpu->prange[port_range(r->DE)];
                           if (portr.read_in != NULL) {
                                   old = portr.read_in(addr_range(r->DE));
                                   cpu_write_byte(r->HL, old);
                           }
                           r->HL++; mask_mode(r->HL, cpu.L);
                           r->BC--; mask_mode(r->BC, cpu.L);
                           r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
                           if (r->BC) {
                                   context.cycles += 3;
                                   r->PC -= 2;
                           }
                           break;
                case 0xC3: // OTIRX
                           context.cycles += 2;
                           portr = context.cpu->prange[port_range(r->DE)];
                           if (portr.write_out != NULL) {
                                   old = cpu_read_byte(r->HL);
                                   portr.write_out(addr_range(r->DE), old);
                           }
                           r->HL++; mask_mode(r->HL, cpu.L);
                           r->BC--; mask_mode(r->BC, cpu.L);
                           r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
                           if (r->BC) {
                                   context.cycles += 3;
                                   r->PC -= 2;
                           }
                           break;
                case 0xC7: // LD I, HL
                           context.cycles += 2;
                           r->I = r->HL;
                           break;
                case 0xD7: // LD HL, I
                           context.cycles += 2;
                           r->HL = r->I | (r->MBASE << 16);
                           break;
                case 0xCA: // INDRX
                           context.cycles += 2;
                           portr = context.cpu->prange[port_range(r->DE)];
                           if (portr.read_in != NULL) {
                                   old = portr.read_in(addr_range(r->DE));
                                   cpu_write_byte(r->HL, old);
                           }
                           r->HL--; mask_mode(r->HL, cpu.L);
                           r->BC--; mask_mode(r->BC, cpu.L);
                           r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
                           if (r->BC) {
                                   context.cycles += 3;
                                   r->PC -= 2;
                           }
                           break;
                case 0xCB: // OTDRX
                           context.cycles += 2;
                           portr = context.cpu->prange[port_range(r->DE)];
                           if (portr.write_out != NULL) {
                                   old = cpu_read_byte(r->HL);
                                   portr.write_out(addr_range(r->DE), old);
                           }
                           r->HL--; mask_mode(r->HL, cpu.L);
                           r->BC--; mask_mode(r->BC, cpu.L);
                           r->F = _flag_zero(r->BC) | _flag_n_msb_set(old) | _flag_undef(r->F);
                           if (r->BC) {
                                   context.cycles += 3;
                                   r->PC -= 2;
                           }
                           break;
                default:   // OPCODETRAP
                           context.cycles += 1;
                           cpu.IEF_wait = 1;
                           break;
                }
                break;
        default: // OPCODETRAP
                 context.cycles += 1;
                 cpu.IEF_wait = 1;
                 break;
        }
    } else {
      switch (context.x)
      {
        case 0:
           switch (context.z)
           {
             case 0:
                switch (context.y)
                {
                  case 0:  // NOP
                          context.cycles += 1;
                          break;
                  case 1:  // EX af,af'
                          context.cycles += 1;
                          swap(r->AF, r->_AF);
                          break;
                  case 2: // DJNZ d
                          context.cycles += 2;
                          s = context.ns();
                          r->B--;                            // decrement B
                          if (r->B != 0) {                   // if B != 0
                              context.cycles += 2;
                              cpu.registers.PC += s;        // add rjump offset
                              mask_mode(cpu.registers.PC, cpu.ADL);
                          }
                          break;
                  case 3: // JR d
                          context.cycles += 3;
                          s = context.ns();
                          cpu.registers.PC += s;          // add rjump offset
                          mask_mode(cpu.registers.PC, cpu.ADL);
                          break;
                  case 4:
                  case 5:
                  case 6:
                  case 7: // JR cc[y-4], d
                          context.cycles += 2;
                          s = context.ns();
                          if (read_cc(context.y - 4)) {
                              context.cycles += 1;
                              cpu.registers.PC += s;          // add rjump offset
                              mask_mode(cpu.registers.PC, cpu.ADL);
                          }
                          break;
                }
                break;
             case 1:
                switch (context.q)
                {
                  case 0: // LD rr, Mmn
                          if (cpu.prefix >> 8 && context.p == 3) { // LD IY/IX, (IX/IY + d)
                              w = cpu_read_word(HLorIr() + context.ns());
                              cpu.prefix ^= 0x1000;
                              HLorIw(w);
                              break;
                          }
                          context.cycles += 3;
                          write_rp(context.p, context.nw());
                          break;
                  case 1: // ADD HL, rr
                          context.cycles += 1;
                          old_word = HLorIr();
                          op_word = read_rp(context.p);
                          new_word = HLorIw(old_word + op_word);
                          r->F = __flag_s(r->flags.S) | _flag_zero(!r->flags.Z)
                                  | _flag_undef(r->F) | __flag_pv(r->flags.PV)
                                  | _flag_subtract(0) | _flag_carry_w(old_word + op_word, cpu.L)
                                  | _flag_halfcarry_w_add(old_word, op_word, 0);
                          break;
                }
                break;
             case 2:
                switch (context.q)
                {
                  case 0:
                      switch (context.p)
                       {
                         case 0: // LD (BC), A
                                 context.cycles += 2;
                                 cpu_write_byte(r->BC, r->A);
                                 break;
                         case 1: // LD (DE), A
                                 context.cycles += 2;
                                 cpu_write_byte(r->DE, r->A);
                                 break;
                         case 2: // LD (Mmn), HL
                                 context.cycles += 7;
                                 cpu_write_word(context.nw(), HLorIr());
                                 break;
                         case 3: // LD (Mmn), A
                                 context.cycles += 5;
                                 cpu_write_byte(context.nw(), r->A);
                                 break;
                         }
                         break;
                      case 1:
                         switch (context.p) {
                           case 0: // LD A, (BC)
                                  context.cycles += 2;
                                  r->A = cpu_read_byte(r->BC);
                                  break;
                          case 1: // LD A, (DE)
                                  context.cycles += 2;
                                  r->A = cpu_read_byte(r->DE);
                                  break;
                          case 2: // LD HL, (Mmn)
                                  context.cycles += 7;
                                  r->WZ = context.nw();
                                  HLorIw(cpu_read_word(r->WZ));
                                  break;
                          case 3: // LD A, (Mmn)
                                  context.cycles += 5;
                                  r->WZ = context.nw();
                                  r->A = cpu_read_byte(r->WZ);
                                  break;
                        }
                        break;
                 }
                 break;
             case 3:
                 switch (context.q)
                 {
                   case 0: // INC rp[p]
                          context.cycles += 1;
                          write_rp(context.p, read_rp(context.p) + 1);
                          break;
                   case 1: // DEC rp[p]
                          context.cycles += 1;
                          write_rp(context.p, read_rp(context.p) - 1);
                          break;
                 }
                 break;
             case 4: // INC r[y]
                     context.cycles += 1;
                     old = read_reg(context.y);
                     if (context.y == 6 && cpu.prefix >> 8) {
                       cpu.registers.PC++;
                       mask_mode(cpu.registers.PC, cpu.ADL);
                     }
                     new = write_reg(context.y, old + 1);
                     r->F = __flag_c(r->flags.C) | _flag_sign_b(new) | _flag_zero(new)
                             | _flag_halfcarry_b_add(old, 0, 1) | __flag_pv(old == 0x7F)
                             | _flag_undef(r->F);
                     break;
             case 5: // DEC r[y]
                    context.cycles += 4;
                    old = read_reg(context.y);
                    if (context.y == 6 && cpu.prefix >> 8) {
                      cpu.registers.PC--;
                      mask_mode(cpu.registers.PC, cpu.ADL);
                    }
                     new = write_reg(context.y, old - 1);
                     r->F = __flag_c(r->flags.C) | _flag_sign_b(new) | _flag_zero(new)
                             | _flag_halfcarry_b_sub(old, 0, 1) | __flag_pv(old == 0x80)
                             | _flag_subtract(1) | _flag_undef(r->F);
                     break;
             case 6: // LD r[y], n
                     if (context.y == 7 && cpu.prefix >> 8) { // LD (IX/IY + d), IY/IX
                         cpu.prefix ^= 0x1000;
                         w = HLorIr();
                         cpu.prefix ^= 0x1000;
                         write_word_indHLorI(w);
                         break;
                     }
                     context.cycles += 2;
                     if (context.y == 6 && cpu.prefix >> 8) { // LD (IX/IY + d), n
                         cpu.registers.PC++;
                         mask_mode(cpu.registers.PC, cpu.ADL);
                     }
                     old = context.nu();
                     if (context.y == 6 && cpu.prefix >> 8) { // LD (IX/IY + d), n
                         cpu.registers.PC -= 2;
                         mask_mode(cpu.registers.PC, cpu.ADL);
                     }
                     write_reg(context.y, old);
                     if (context.y == 6 && cpu.prefix >> 8) { // LD (IX/IY + d), n
                         cpu.registers.PC++;
                         mask_mode(cpu.registers.PC, cpu.ADL);
                     }
                     break;
             case 7:
                     if (cpu.prefix >> 8) {
                         if (context.q) { // LD (IX/IY + d), rp3[p]
                             cpu_write_word(HLorIr() + context.ns(), read_rp3(context.p));
                         } else { // LD rp3[p], (IX/IY + d)
                             write_rp3(context.p, cpu_read_word(HLorIr() + context.ns()));
                         }
                     } else {
                         execute_rot_acc(context.y);
                     }
                     break;
           }
           break;
           case 1: // ignore prefixed prefixes
                  if (context.opcode == 0x40) // .SIS
                  {
                      cpu.SUFFIX = 1;
                      cpu.S = 1; cpu.IS = 1;
                      cpu.L = 0; cpu.IL = 0;
                      goto exit_loop;
                  }
                  else if (context.opcode == 0x49) // .SIL
                  {
                      cpu.SUFFIX = 1;
                      cpu.S = 1; cpu.IS = 0;
                      cpu.L = 0; cpu.IL = 1;
                      goto exit_loop;
                  }
                  else if (context.opcode == 0x52) // .LIS
                  {
                      cpu.SUFFIX = 1;
                      cpu.S = 0; cpu.IS = 1;
                      cpu.L = 1; cpu.IL = 0;
                      goto exit_loop;
                  }
                  else if (context.opcode == 0x5B) // .LIL
                  {
                      cpu.SUFFIX = 1;
                      cpu.S = 0; cpu.IS = 0;
                      cpu.L = 1; cpu.IL = 1;
                      goto exit_loop;
                  }
                  if (context.opcode == 0x76)  // HALT
                  {
                    context.cycles += 1;
                    cpu.halted = 1;
                  }
                  else // LD r[y], r[z]
                  {
                    context.cycles += 1;
                    read_write_reg(context.z, context.y);
                  }
                  break;
           case 2: // ALU[y] r[z]
                  execute_alu(context.y, read_reg(context.z));
                  break;
           case 3:
                  switch (context.z)
                  {
                    case 0: // RET cc[y]
                            context.cycles += 2;
                            if (read_cc(context.y)) {
                                context.cycles += 5;
                                if (cpu.SUFFIX) {
                                    w = cpu_read_byte(r->SPL++) & 1;
                                    if (cpu.ADL) {
                                        r->PCL = cpu_read_byte(r->SPL++);
                                        r->PCH = cpu_read_byte(r->SPL++);
                                    } else {
                                        r->PCL = cpu_read_byte(r->SPS++);
                                        r->PCH = cpu_read_byte(r->SPS++);
                                    }
                                    if (w) {
                                        r->PCU = cpu_read_byte(r->SPL++);
                                        if (!cpu.L && !cpu.ADL) {
                                            r->PCU = 0;
                                        }
                                    }
                                    cpu.ADL = w;
                                } else {
                                    r->PC = cpu_pop();
                                }
                            }
                            break;
                    case 1:
                           switch (context.q)
                           {
                             case 0: // POP rp2[p]
                                    context.cycles += 4;
                                    write_rp2(context.p, cpu_pop());
                                    break;
                             case 1:
                                    switch (context.p)
                                    {
                                      case 0: // RET
                                             context.cycles += 7;
                                             if (cpu.SUFFIX) {
                                                 w = cpu_read_byte(r->SPL++) & 1;
                                                 if (cpu.ADL) {
                                                     r->PCL = cpu_read_byte(r->SPL++);
                                                     r->PCH = cpu_read_byte(r->SPL++);
                                                 } else {
                                                     r->PCL = cpu_read_byte(r->SPS++);
                                                     r->PCH = cpu_read_byte(r->SPS++);
                                                 }
                                                 if (w) {
                                                     r->PCU = cpu_read_byte(r->SPL++);
                                                     if (!cpu.L && !cpu.ADL) {
                                                         r->PCU = 0;
                                                     }
                                                 }
                                                 cpu.ADL = w;
                                             } else {
                                                 r->PC = cpu_pop();
                                             }
                                             break;
                                      case 1: // EXX
                                             context.cycles += 1;
                                             exx(&cpu.registers);
                                             break;
                                      case 2: // JP (rr)
                                              context.cycles += 4;
                                              r->PC = HLorIr();
                                              cpu.ADL = cpu.L;
                                              break;
                                      case 3: // LD SP, HL
                                             context.cycles += 6;
                                             if(cpu.S) { r->SPS = HLorIr(); } else { r->SPL = HLorIr(); }
                                             break;
                                    }
                                    break;
                           }
                           break;
                    case 2: // JP cc[y], nn
                            context.cycles += 4;
                            if (cpu.ADL != cpu.IL) {
                                set_context(cpu.IL);
                            }
                            w = context.nw();
                            if (read_cc(context.y)) {
                                r->PC = w;
                                cpu.ADL = cpu.L;
                            }
                            break;
                    case 3:
                            switch (context.y)
                            {
                             case 0: // JP nn
                                     context.cycles += 5;
                                     if (cpu.ADL != cpu.IL) {
                                         set_context(cpu.IL);
                                     }
                                     r->PC = context.nw();
                                     cpu.ADL = cpu.L;
                                     break;
                             case 1: // 0xCB prefixed opcodes
                                     context.cycles += 1;
                                     cpu.prefix &= 0xFF00;
                                     cpu.prefix |= 0x00CB;
                                     reset_prefix = 0;
                                     break;
                             case 2: // OUT (n), A
                                    context.cycles += 3;
                                    r->WZ = (r->A<<8) | context.nu();
                                    portr = context.cpu->prange[port_range(r->WZ)];
                                    if (portr.write_out != NULL) {
                                            portr.write_out(addr_range(r->WZ), r->A);
                                    }
                                    break;
                             case 3: // IN A, (n)
                                    context.cycles += 3;
                                    r->WZ = (r->A<<8) | context.nu();
                                    portr = context.cpu->prange[port_range(r->WZ)];
                                    if (portr.read_in != NULL) {
                                            r->A = portr.read_in(addr_range(r->WZ));
                                    }
                                    break;
                             case 4: // EX (SP), HL/I
                                    context.cycles += 7;
                                    if(cpu.L) {
                                        r->WZ = cpu_read_word(r->SPL);
                                        cpu_write_word(r->SPL, HLorIr());
                                        HLorIw(r->WZ);
                                    } else {
                                        r->WZ = cpu_read_word(r->SPS);
                                        cpu_write_word(r->SPS, HLorIr());
                                        HLorIw(r->WZ);
                                    }
                                    break;
                             case 5: // EX DE, HL
                                    context.cycles += 1;
                                    swap(r->HL, r->DE);
                                    break;
                             case 6: // DI
                                    context.cycles += 1;
                                    cpu.IEF1 = 0;
                                    cpu.IEF2 = 0;
                                    break;
                             case 7: // EI
                                    context.cycles += 1;
                                    cpu.IEF1 = 1;
                                    cpu.IEF2 = 1;
                                    cpu.IEF_wait = 1;
                                    break;
                           }
                           break;
                    case 4: // CALL cc[y], nn
                           context.cycles += 4;
                           w = context.nw();
                           if (read_cc(context.y)) {
                               context.cycles += 3;
                               if (cpu.SUFFIX) {
                                   if (cpu.ADL) {
                                       cpu_write_byte(--r->SPL, r->PCU);
                                   }
                                   if (cpu.IL || (cpu.L && !cpu.ADL)) {
                                       cpu_write_byte(--r->SPL, r->PCH);
                                       cpu_write_byte(--r->SPL, r->PCL);
                                   } else {
                                       cpu_write_byte(--r->SPS, r->PCH);
                                       cpu_write_byte(--r->SPS, r->PCL);
                                   }
                                   cpu_write_byte(--r->SPL, (cpu.MADL << 1) | cpu.ADL);
                                   cpu.ADL = cpu.IL;
                               } else {
                                   cpu_push(r->PC);
                               }
                               r->PC = w;
                           }
                           break;
                    case 5:
                           switch (context.q)
                           {
                             case 0: // PUSH r2p[p]
                                    context.cycles += 4;
                                    cpu_push(read_rp2(context.p));
                                    break;
                             case 1:
                                    switch (context.p)
                                    {
                                      case 0: // CALL nn
                                             context.cycles += 7;
                                             w = context.nw();
                                             if (cpu.SUFFIX) {
                                                 if (cpu.ADL) {
                                                     cpu_write_byte(--r->SPL, r->PCU);
                                                 }
                                                 if (cpu.IL || (cpu.L && !cpu.ADL)) {
                                                     cpu_write_byte(--r->SPL, r->PCH);
                                                     cpu_write_byte(--r->SPL, r->PCL);
                                                 } else {
                                                     cpu_write_byte(--r->SPS, r->PCH);
                                                     cpu_write_byte(--r->SPS, r->PCL);
                                                 }
                                                 cpu_write_byte(--r->SPL, (cpu.MADL << 1) | cpu.ADL);
                                                 cpu.ADL = cpu.IL;
                                             } else {
                                                 cpu_push(r->PC);
                                             }
                                             r->PC = w;
                                             break;
                                      case 1: // 0xDD prefixed opcodes
                                             context.cycles += 1;
                                             cpu.prefix &= 0xFF;
                                             cpu.prefix |= 0xDD00;
                                             reset_prefix = 0;
                                             break;
                                      case 2: // 0xED prefixed opcodes
                                             context.cycles += 1;
                                             cpu.prefix &= 0xFF;
                                             cpu.prefix |= 0xED00;
                                             reset_prefix = 0;
                                             break;
                                      case 3: // 0xFD prefixed opcodes
                                             context.cycles += 1;
                                             cpu.prefix &= 0xFF;
                                             cpu.prefix |= 0xFD00;
                                             reset_prefix = 0;
                                             break;
                                    }
                                    break;
                             }
                             break;
                      case 6: // alu[y] n
                             execute_alu(context.y, context.nu());
                             break;
                      case 7: // RST y*8
                             context.cycles += 6;
                             if (cpu.SUFFIX) {
                                 if (cpu.ADL) {
                                     cpu_write_byte(--r->SPL, r->PCU);
                                 }
                                 if (cpu.IL || (cpu.L && !cpu.ADL)) {
                                     cpu_write_byte(--r->SPL, r->PCH);
                                     cpu_write_byte(--r->SPL, r->PCL);
                                 } else {
                                     cpu_write_byte(--r->SPS, r->PCH);
                                     cpu_write_byte(--r->SPS, r->PCL);
                                 }
                                 cpu_write_byte(--r->SPL, (cpu.MADL << 1) | cpu.ADL);
                                 cpu.ADL = cpu.IL;
                             } else {
                                 cpu_push(r->PC);
                             }
                             r->PC = context.y << 3;
                             break;
                    }
                    break;
            }
    }

    if (reset_prefix) {
            cpu.prefix = 0;
    }

    get_cntrl_data_blocks_format();

exit_loop:
    cycle_count_delta += context.cycles;
    if (context.cycles == 0) {
            //logprintf(LOG_CPU, "Error: Unrecognized instruction 0x%02X.", context.opcode);
            cycle_count_delta++;
    }
  }
  return cycle_count_delta;
}

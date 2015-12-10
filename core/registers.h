#ifndef REGISTERS_H
#define REGISTERS_H

#include <stdint.h>

#include <core/defines.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	union {
		uint32_t AF;
		struct {
			union {
				uint8_t F;
				struct {
					uint8_t C  : 1;
					uint8_t N  : 1;
					uint8_t PV : 1;
					uint8_t _3 : 1;
					uint8_t H  : 1;
					uint8_t _5 : 1;
					uint8_t Z  : 1;
					uint8_t S  : 1;
				} flags;
			};
			uint8_t A;
		};
	};
	union {
		uint32_t BC;
		struct {
			uint8_t C;
			uint8_t B;
			uint8_t BCU;
		};
	};
	union {
		uint32_t DE;
		struct {
			uint8_t E;
			uint8_t D;
			uint8_t DEU;
		};
	};
	union {
		uint32_t HL;
		struct {
			uint8_t L;
			uint8_t H;
			uint8_t HLU;
		};
	};
	uint32_t _AF, _BC, _DE, _HL;
	uint32_t SPL;
	uint16_t SPS;
	uint32_t PC;
	union {
		uint32_t IX;
		struct {
			uint8_t IXL;
			uint8_t IXH;
			uint8_t IXU;
		};
	};
	union {
		uint32_t IY;
		struct {
			uint8_t IYL;
			uint8_t IYH;
			uint8_t IYU;
		};
	};
	uint16_t I;
	uint8_t R, MBASE;  // interrupt, r, and z80 MBASE
	uint32_t WZ;
} eZ80registers_t;

typedef enum {
	FLAG_S =  1 << 7,
	FLAG_Z =  1 << 6,
	FLAG_5 =  1 << 5,
	FLAG_H =  1 << 4,
	FLAG_3 =  1 << 3,
	FLAG_PV = 1 << 2,
	FLAG_N  = 1 << 1,
	FLAG_C  = 1 << 0,
	FLAG_NONE = 0
} eZ80flags;

void exx(eZ80registers_t *r);
int parity(uint8_t x);

// S Z 5 H 3 PV N C
#define __flag_s(a)  ((a) ? FLAG_S  : 0)
#define __flag_5(a)  ((a) ? FLAG_5  : 0)
#define __flag_h(a)  ((a) ? FLAG_H  : 0)
#define __flag_3(a)  ((a) ? FLAG_3  : 0)
#define __flag_pv(a) ((a) ? FLAG_PV : 0)
#define __flag_c(a)  ((a) ? FLAG_C  : 0)

#define _flag_carry_w(a,mode) __flag_c((a) & (0x10000<<(mode<<3)))
#define _flag_carry_b(a) __flag_c((a) & 0x100)

#define _flag_sign_w(a,mode) __flag_s((a) & (0x8000<<(mode<<3)))
#define _flag_sign_b(a) __flag_s((a) & 0x80)

#define _flag_parity(a) __flag_pv(!parity(a))

#define _flag_undef(a) (a & (FLAG_3 | FLAG_5))

#define _flag_overflow_b_add(op1, op2, result) __flag_pv((op1 & 0x80) == (op2 & 0x80) && (op1 & 0x80) != (result & 0x80))
#define _flag_overflow_b_sub(op1, op2, result) __flag_pv((op1 & 0x80) != (op2 & 0x80) && (op1 & 0x80) != (result & 0x80))
#define _flag_overflow_w_add(op1, op2, result, mode) __flag_pv((op1 & (0x8000<<(mode<<3))) == (op2 & (0x8000<<(mode<<3))) && (op1 & (0x8000<<(mode<<3))) != (result & (0x8000<<(mode<<3))))
#define _flag_overflow_w_sub(op1, op2, result, mode) __flag_pv((op1 & (0x8000<<(mode<<3))) != (op2 & (0x8000<<(mode<<3))) && (op1 & (0x8000<<(mode<<3))) != (result & (0x8000<<(mode<<3))))

#define _flag_halfcarry_b_add(op1, op2, carry) __flag_h(((op1 & 0xf) + (op2 & 0xf) + carry) & 0x10)
#define _flag_halfcarry_b_sub(op1, op2, carry) __flag_h(((op1 & 0xf) - (op2 & 0xf) - carry) & 0x10)
#define _flag_halfcarry_w_add(op1, op2, carry, mode) __flag_h(( (op1 & (0xfffff>>(mode<<3))) + (op2 & (0xfffff>>(mode<<3))) + carry) & (0x1000<<(mode<<3)))
#define _flag_halfcarry_w_sub(op1, op2, carry, mode) __flag_h(( (op1 & (0xfffff>>(mode<<3))) - (op2 & (0xfffff>>(mode<<3))) - carry) & (0x1000<<(mode<<3)))

#define _flag_subtract(a)   ((a) ? FLAG_N : 0)
#define _flag_zero(a)       ((a) ? 0 : FLAG_Z)

#define _flag_n_msb_set(a)  ((a>>7) ? FLAG_N : 0)
#define mask_mode(a, mode)  (a &= (0xFFFF<<(mode<<3))|0xFF)

#ifdef __cplusplus
}
#endif

#endif

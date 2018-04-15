#include "zdis.h"

#include <assert.h>

#define MASK(start, length) (((1u << (length)) - 1u) << start)

#define MNE_MAX_LEN 2
#define MNE_MASK MASK(0, 5)
enum mne {
#define MNE(x) MNE_##x,
#define MNE2(x,y) MNE(x)
#include "mne.def"
  MNE_LAST
};
static_assert(MNE_LAST <= MASK(5, 1), "Too many mnemonics");
static const char mnes[][MNE_MAX_LEN] = {
#define MNE(x) #x,
#define MNE2(x,y) y,
#include "mne.def"
};

enum sep {
  SEP_,
  SEP__,
  SEP_B = SEP__,
  SEP_C = SEP__,
  SEP_D = SEP__,
  SEP_E = SEP__,
  SEP_H = SEP__,
  SEP_L = SEP__,
  SEP_F_ = SEP_,
  SEP_A = SEP__,
  SEP_NZ = SEP__,
  SEP_Z = SEP__,
  SEP_NC = SEP__,
/*SEP_C = SEP__,*/
  SEP_PO = SEP__,
  SEP_PE = SEP__,
  SEP_P = SEP__,
  SEP_M = SEP__,
};

#define ARG_MAX_LEN 3
#define ARG_MASK MASK(0, 6)
enum arg {
#define ARG(x) ARG_##x,
#define ARG2(x,y) ARG(x)
#include "arg.def"
#define IMM(x) ARG_##x,
#include "imm.def"
  ARG_A_,
  ARG_F_,
  ARG_IXYH,
  ARG_IXYL,
  ARG_IXY,
  ARG_IYX,
  ARG__IXY,
  ARG__IXYO,
  ARG_IXO,
  ARG_IYO,
  ARG_LAST,
  ARG_FIRST_IMM = ARG_BYTE
};
static_assert(ARG_LAST <= MASK(6, 1), "Too many arguments");
static const char args[][ARG_MAX_LEN] = {
#define ARG(x) #x,
#define ARG2(x,y) y,
#include "arg.def"
};
#define ARG2PUT(x) ((enum zdis_put)((x) - ARG_FIRST_IMM + ZDIS_PUT_FIRST_IMM))

#define I(m1,m2,s1,a1,s2,a2) {MNE_##m1|(MNE_##m2<<5&MASK(5, 3)),MNE_##m2>>3|ARG_##a1<<2,SEP_##s1|ARG_##a2<<1|SEP_##s2<<7},
#define P(p) {p,0,0},
#define A1(a1) 
#define ADD(a1,a2) I(AD,D,_,a1,_,a2)
#define ADC(a1,a2) I(AD,C,_,a1,_,a2)
#define AND(a2) I(AN,D,_,A_,,a2)
#define CALL(a1) I(CA,LL,_,a1,a1,ABS)
#define CP(a2) I(C,P,_,A_,,a2)
#define DEC(a1) I(D,E,,C,_,a1)
#define EX(a1,a2) I(E,X,_,a1,_,a2)
#define IM(a1) I(I,,,M,_,a1)
#define IN(m2,a1,a2) I(IN,m2,_,a1,a1,a2)
#define INC(a1) I(IN,C,,,_,a1)
#define JP(a1,a2) I(J,P,_,a1,a1,a2)
#define JR(a1) I(J,R,_,a1,a1,REL)
#define LD(a1,a2) I(L,D,_,a1,_,a2)
#define LEA(a1,a2) I(L,EA,_,a1,_,a2##O)
#define MLT(a1) I(,,,MLT,_,a1)
#define OR(a2) I(OR,,_,A_,,a2)
#define OUT(m2,a1,a2) I(OU,T##m2,_,a1,_,a2)
#define PEA(a1) I(P,EA,,,_,a1##O)
#define POP(a1) I(P,,,OP,_,a1)
#define PUSH(a1) I(PU,S,,H,_,a1)
#define RET(a1) I(R,E,,T,a1,a1)
#define RST() I(R,S,,T,_,RST)
#define SBC(a1,a2) I(S,BC,_,a1,_,a2)
#define SUB(a2) I(S,UB,_,A_,,a2)
#define TRAP() I(T,R,,A,,P)
#define TST(a2) I(T,ST,_,A_,,a2)
#define XOR(a2) I(X,OR,_,A_,,a2)
static const uint8_t main_insts[][1 << 6][3] = {
  {
    I(N,,,,,OP)      LD(BC,WORD) LD(_BC,A)   INC(BC) INC(B)   DEC(B)   LD(B,BYTE)   I(R,L,,C,,A)
    EX(AF,AF_)       ADD(HL,BC)  LD(A,_BC)   DEC(BC) INC(C)   DEC(C)   LD(C,BYTE)   I(R,R,,C,,A)
    I(D,J,,NZ,_,REL) LD(DE,WORD) LD(_DE,A)   INC(DE) INC(D)   DEC(D)   LD(D,BYTE)   I(R,L,,,,A)
    JR()             ADD(HL,DE)  LD(A,_DE)   DEC(DE) INC(E)   DEC(E)   LD(E,BYTE)   I(R,R,,,,A)
    JR(NZ)           LD(HL,WORD) LD(ADDR,HL) INC(HL) INC(H)   DEC(H)   LD(H,BYTE)   I(D,,,A,,A)
    JR(Z)            ADD(HL,HL)  LD(HL,ADDR) DEC(HL) INC(L)   DEC(L)   LD(L,BYTE)   I(C,,,P,,L)
    JR(NC)           LD(SP,WORD) LD(ADDR,A)  INC(SP) INC(_HL) DEC(_HL) LD(_HL,BYTE) I(S,,,C,,F)
    JR(C)            ADD(HL,SP)  LD(A,ADDR)  DEC(SP) INC(A)   DEC(A)   LD(A,BYTE)   I(C,,,C,,F)
  }, {
    RET(NZ)           POP(BC)     JP(NZ,ABS)  JP(,ABS)     CALL(NZ) PUSH(BC) ADD(A,BYTE) RST()
    RET(Z)            RET()       JP(Z,ABS)   P(0)         CALL(Z)  CALL()   ADC(A,BYTE) RST()
    RET(NC)           POP(DE)     JP(NC,ABS)  OUT(,PORT,A) CALL(NC) PUSH(DE) SUB(BYTE)   RST()
    RET(C)            I(E,X,,X,,) JP(C,ABS)   IN(,A,PORT)  CALL(C)  P(2)     SBC(A,BYTE) RST()
    RET(PO)           POP(HL)     JP(PO,ABS)  EX(_SP,HL)   CALL(PO) PUSH(HL) AND(BYTE)   RST()
    RET(PE)           JP(,_HL)    JP(PE,ABS)  EX(DE,HL)    CALL(PE) P(1)     XOR(BYTE)   RST()
    RET(P)            POP(AF)     JP(P,ABS)   I(D,I,,,,)   CALL(P)  PUSH(AF) OR(BYTE)    RST()
    RET(M)            LD(SP,HL)   JP(M,ABS)   I(E,I,,,,)   CALL(M)  P(3)     CP(BYTE)    RST()
  }
};
static const uint8_t cb_insts[1 << 5][3] = {
  I(R,L,,C,_,B)  I(R,R,,C,_,B)  I(R,L,,,_,B)   I(R,R,,,_,B)   I(S,L,,A,_,B)  I(S,R,,A,_,B)  TRAP()         I(S,R,,L,_,B)
  I(BI,T,_,0,_,B) I(BI,T,_,1,_,B) I(BI,T,_,2,_,B) I(BI,T,_,3,_,B) I(BI,T,_,4,_,B) I(BI,T,_,5,_,B) I(BI,T,_,6,_,B) I(BI,T,_,7,_,B)
  I(RE,S,_,0,_,B) I(RE,S,_,1,_,B) I(RE,S,_,2,_,B) I(RE,S,_,3,_,B) I(RE,S,_,4,_,B) I(RE,S,_,5,_,B) I(RE,S,_,6,_,B) I(RE,S,_,7,_,B)
  I(SE,T,_,0,_,B) I(SE,T,_,1,_,B) I(SE,T,_,2,_,B) I(SE,T,_,3,_,B) I(SE,T,_,4,_,B) I(SE,T,_,5,_,B) I(SE,T,_,6,_,B) I(SE,T,_,7,_,B)
};
static const uint8_t xd_insts[1 << 8][3] = {
  TRAP() TRAP()        TRAP()       TRAP()    TRAP()     TRAP()     TRAP()         LD(BC,_IXYO)
  TRAP() ADD(IXY,BC)   TRAP()       TRAP()    TRAP()     TRAP()     TRAP()         LD(_IXYO,BC)
  TRAP() TRAP()        TRAP()       TRAP()    TRAP()     TRAP()     TRAP()         LD(DE,_IXYO)
  TRAP() ADD(IXY,DE)   TRAP()       TRAP()    TRAP()     TRAP()     TRAP()         LD(_IXYO,DE)
  TRAP() LD(IXY,WORD)  LD(ADDR,IXY) INC(IXY)  INC(IXYH)  DEC(IXYH)  LD(IXYH,BYTE)  LD(HL,_IXYO)
  TRAP() ADD(IXY,IXY)  LD(IXY,ADDR) DEC(IXY)  INC(IXYL)  DEC(IXYL)  LD(IXYL,BYTE)  LD(_IXYO,HL)
  TRAP() LD(IYX,_IXYO) TRAP()       TRAP()    INC(_IXYO) DEC(_IXYO) LD(_IXYO,BYTE) LD(IXY,_IXYO)
  TRAP() ADD(IXY,SP)   TRAP()       TRAP()    TRAP()     TRAP()     LD(_IXYO,IYX)  LD(_IXYO,IXY)

  TRAP()      TRAP()      TRAP()      TRAP()      LD(B,IXYH)    LD(B,IXYL)    LD(B,_IXYO) TRAP()
  TRAP()      TRAP()      TRAP()      TRAP()      LD(C,IXYH)    LD(C,IXYL)    LD(C,_IXYO) TRAP()
  TRAP()      TRAP()      TRAP()      TRAP()      LD(D,IXYH)    LD(D,IXYL)    LD(D,_IXYO) TRAP()
  TRAP()      TRAP()      TRAP()      TRAP()      LD(E,IXYH)    LD(E,IXYL)    LD(E,_IXYO) TRAP()
  LD(IXYH,B)  LD(IXYH,C)  LD(IXYH,D)  LD(IXYH,E)  LD(IXYH,IXYH) LD(IXYH,IXYL) LD(H,_IXYO) LD(IXYH,A)
  LD(IXYL,B)  LD(IXYL,C)  LD(IXYL,D)  LD(IXYL,E)  LD(IXYL,IXYH) LD(IXYL,IXYL) LD(L,_IXYO) LD(IXYL,A)
  LD(_IXYO,B) LD(_IXYO,C) LD(_IXYO,D) LD(_IXYO,E) LD(_IXYO,H)   LD(_IXYO,L)   TRAP()      LD(_IXYO,A)
  TRAP()      TRAP()      TRAP()      TRAP()      LD(A,IXYH)    LD(A,IXYL)    LD(A,_IXYO) TRAP()

  TRAP() TRAP() TRAP() TRAP() ADD(A,IXYH) ADD(A,IXYL) ADD(A,_IXYO) TRAP()
  TRAP() TRAP() TRAP() TRAP() ADC(A,IXYH) ADC(A,IXYL) ADC(A,_IXYO) TRAP()
  TRAP() TRAP() TRAP() TRAP() SUB(IXYH)   SUB(IXYL)   SUB(_IXYO)   TRAP()
  TRAP() TRAP() TRAP() TRAP() SBC(A,IXYH) SBC(A,IXYL) SBC(A,_IXYO) TRAP()
  TRAP() TRAP() TRAP() TRAP() AND(IXYH)   AND(IXYL)   AND(_IXYO)   TRAP()
  TRAP() TRAP() TRAP() TRAP() XOR(IXYH)   XOR(IXYL)   XOR(_IXYO)   TRAP()
  TRAP() TRAP() TRAP() TRAP() OR(IXYH)    OR(IXYL)    OR(_IXYO)    TRAP()
  TRAP() TRAP() TRAP() TRAP() CP(IXYH)    CP(IXYL)    CP(_IXYO)    TRAP()

  TRAP() TRAP()     TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
  TRAP() TRAP()     TRAP() P(0)        TRAP() TRAP()    TRAP() TRAP()
  TRAP() TRAP()     TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
  TRAP() TRAP()     TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
  TRAP() POP(IXY)   TRAP() EX(_SP,IXY) TRAP() PUSH(IXY) TRAP() TRAP()
  TRAP() JP(,_IXY)  TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
  TRAP() TRAP()     TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
  TRAP() LD(SP,IXY) TRAP() TRAP()      TRAP() TRAP()    TRAP() TRAP()
};
static const uint8_t ed_insts[1 << 8][3] = {
  IN(0,B,PORT)  OUT(0,PORT,B) LEA(BC,IX)  LEA(BC,IY)  TST(B)   TRAP() TRAP()      LD(BC,_HL)
  IN(0,C,PORT)  OUT(0,PORT,C) TRAP()      TRAP()      TST(C)   TRAP() TRAP()      LD(_HL,BC)
  IN(0,D,PORT)  OUT(0,PORT,D) LEA(DE,IX)  LEA(DE,IY)  TST(D)   TRAP() TRAP()      LD(DE,_HL)
  IN(0,E,PORT)  OUT(0,PORT,E) TRAP()      TRAP()      TST(E)   TRAP() TRAP()      LD(_HL,DE)
  IN(0,H,PORT)  OUT(0,PORT,H) LEA(HL,IX)  LEA(HL,IY)  TST(H)   TRAP() TRAP()      LD(HL,_HL)
  IN(0,L,PORT)  OUT(0,PORT,L) TRAP()      TRAP()      TST(L)   TRAP() TRAP()      LD(_HL,HL)
  IN(0,F_,PORT) LD(IYX,_HL)   LEA(IXY,IX) LEA(IYX,IY) TST(_HL) TRAP() TRAP()      LD(IXY,_HL)
  IN(0,A,PORT)  OUT(0,PORT,A) TRAP()      TRAP()      TST(A)   TRAP() LD(_HL,IYX) LD(_HL,IXY)

  IN(,B,_BC)  OUT(,_BC,B) SBC(HL,BC) LD(ADDR,BC) I(N,E,,G,,)        I(R,E,,T,,N)   IM(0)          LD(I,A)
  IN(,C,_BC)  OUT(,_BC,C) ADC(HL,BC) LD(BC,ADDR) MLT(BC)            I(R,E,,T,,I)   TRAP()         LD(R,A)
  IN(,D,_BC)  OUT(,_BC,D) SBC(HL,DE) LD(ADDR,DE) LEA(IXY,IY)        LEA(IYX,IX)    IM(1)          LD(A,I)
  IN(,E,_BC)  OUT(,_BC,E) ADC(HL,DE) LD(DE,ADDR) MLT(DE)            TRAP()         IM(2)          LD(A,R)
  IN(,H,_BC)  OUT(,_BC,H) SBC(HL,HL) LD(ADDR,HL) TST(BYTE)          PEA(IX)        PEA(IY)        I(R,R,,D,,)
  IN(,L,_BC)  OUT(,_BC,L) ADC(HL,HL) LD(HL,ADDR) MLT(HL)            LD(MB,A)       LD(A,MB)       I(R,L,,D,,)
  IN(,F_,_BC) TRAP()      SBC(HL,SP) LD(ADDR,SP) I(T,ST,,IO,_,BYTE) TRAP()         I(S,L,,P,,)    TRAP()
  IN(,A,_BC)  OUT(,_BC,A) ADC(HL,SP) LD(SP,ADDR) MLT(SP)            I(S,T,,M,,IXY) I(R,S,,M,,IXY) TRAP()

  TRAP()       TRAP()       I(IN,I,,M,,)  I(OT,I,,M,,)  I(IN,I,,2,,)  TRAP() TRAP() TRAP()
  TRAP()       TRAP()       I(IN,D,,M,,)  I(OT,D,,M,,)  I(IN,D,,2,,)  TRAP() TRAP() TRAP()
  TRAP()       TRAP()       I(IN,I,,M,,R) I(OT,I,,M,,R) I(IN,I,,2,,R) TRAP() TRAP() TRAP()
  TRAP()       TRAP()       I(IN,D,,M,,R) I(OT,D,,M,,R) I(IN,D,,2,,R) TRAP() TRAP() TRAP()
  I(L,D,,I,,)  I(C,P,,I,,)  I(IN,I,,,,)   I(OU,T,,I,,)  I(OU,T,,I,,2) TRAP() TRAP() TRAP()
  I(L,D,,D,,)  I(C,P,,D,,)  I(IN,D,,,,)   I(OU,T,,D,,)  I(OU,T,,D,,2) TRAP() TRAP() TRAP()
  I(L,D,,I,,R) I(C,P,,I,,R) I(IN,I,,,,R)  I(O,T,,I,,R)  I(OT,I,,2,,R) TRAP() TRAP() TRAP()
  I(L,D,,D,,R) I(C,P,,D,,R) I(IN,D,,,,R)  I(O,T,,D,,R)  I(OT,D,,2,,R) TRAP() TRAP() TRAP()

  TRAP() TRAP() I(IN,I,,R,,X) I(OT,I,,R,,X) TRAP() TRAP() TRAP() LD(I,HL)
  TRAP() TRAP() I(IN,D,,R,,X) I(OT,D,,R,,X) TRAP() TRAP() TRAP() TRAP()
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() LD(HL,I)
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() TRAP()
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() TRAP()
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() TRAP()
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() TRAP()
  TRAP() TRAP() TRAP()        TRAP()        TRAP() TRAP() TRAP() TRAP()
};
#undef I
#define I(m1,m2,s1,a1,s2,a2) (MNE_##m1|MNE_##m2<<5|ARG_##a1<<10|SEP_##s1<<16|ARG_##a2<<17|SEP_##s2<<23)

static bool next(uint8_t *result, struct zdis_ctx *ctx) {
  int byte = ctx->zdis_read(ctx, ctx->zdis_end_addr++);
  return (*result = byte) == byte;
}

static bool put(struct zdis_ctx *ctx, char c) {
  return ctx->zdis_put(ctx, ZDIS_PUT_CHAR, c);
}
static bool letter(struct zdis_ctx *ctx, char c) {
  if (!c) return true;
  if (ctx->zdis_lowercase) {
    c |= 1 << 5;
  }
  return put(ctx, c);
}
static bool letters(struct zdis_ctx *ctx, const char *str, size_t len) {
  while (len-- && *str)
    if (!letter(ctx, *str++))
      return false;
  return true;
}
static bool str(struct zdis_ctx *ctx, const char *str) {
  while (*str)
    if (!ctx->zdis_put(ctx, ZDIS_PUT_CHAR, *str++))
      return false;
  return true;
}
static bool mne(struct zdis_ctx *ctx, enum mne mne) {
  return letters(ctx, mnes[mne], MNE_MAX_LEN);
}
static bool suffix(struct zdis_ctx *ctx, bool suffix) {
  return letter(ctx, suffix ? 'L' : 'S');
}
static bool index(struct zdis_ctx *ctx, bool index) {
  return letter(ctx, 'I') && letter(ctx, 'X' + index);
}
static bool sep(struct zdis_ctx *ctx, bool sep, bool last, uint8_t extra, uint8_t *which) {
  ctx->zdis_end_addr += last && extra & MASK(7, 1);
  return !sep || ((*which || !(extra & MASK(2, 1)) ||
		   (put(ctx, '.') && suffix(ctx, extra & MASK(0, 1)) &&
		    letter(ctx, 'i') && suffix(ctx, extra & MASK(1, 1)))) &&
		  (last || str(ctx, ctx->zdis_separators[(*which)++])));
}
static bool arg(struct zdis_ctx *ctx, enum arg arg, uint8_t extra, uint8_t *which) {
  uint8_t a, b, c = 0;
  switch (arg) {
    default:
      return letters(ctx, args[arg], ARG_MAX_LEN) && (args[arg][0] != '(' || put(ctx, ')'));
    case ARG_BYTE:
    case ARG_PORT:
    case ARG_OFF:
    case ARG_REL:
      return next(&a, ctx) &&
	(arg != ARG_PORT || put(ctx, '(')) &&
	ctx->zdis_put(ctx, ARG2PUT(arg), arg < ARG_OFF ? a : (int8_t)a) &&
	(arg != ARG_PORT || put(ctx, ')'));
    case ARG_WORD:
    case ARG_ADDR:
    case ARG_ABS:
      return next(&a, ctx) && next(&b, ctx) &&
	(!(extra & MASK(2, 1) ? extra & MASK(1, 1) : ctx->zdis_adl) || next(&c, ctx)) &&
	(arg != ARG_ADDR || put(ctx, '(')) &&
	ctx->zdis_put(ctx, ARG2PUT(arg), a | b << 8 | c << 16) &&
	(arg != ARG_ADDR || put(ctx, ')'));
    case ARG_RST:
      return ctx->zdis_put(ctx, ARG2PUT(arg), extra & MASK(3, 3));
    case ARG_A_:
    case ARG_F_:
      return !ctx->zdis_implicit ||
	(letter(ctx, arg == ARG_A_ ? 'A' : 'F') &&
	 sep(ctx, true, false, extra, which));
    case ARG_IXYH:
    case ARG_IXYL:
    case ARG_IXY:
    case ARG_IYX:
    case ARG__IXY:
    case ARG__IXYO:
      return (arg < ARG__IXYO || next(&a, ctx)) &&
	(arg < ARG__IXY || put(ctx, '(')) &&
	index(ctx, (extra >> 6 ^ (arg == ARG_IYX)) & MASK(0, 1)) &&
	(arg < ARG__IXYO || ctx->zdis_put(ctx, ZDIS_PUT_OFF, (int8_t)a)) &&
	(arg >= ARG_IXY || letter(ctx, arg == ARG_IXYH ? 'H' : 'L')) &&
	(arg < ARG__IXY || put(ctx, ')'));
    case ARG_IXO:
    case ARG_IYO:
      return next(&a, ctx) && index(ctx, (arg ^ ARG_IXO) & MASK(0, 1)) &&
	ctx->zdis_put(ctx, ZDIS_PUT_OFF, (int8_t)a);
  }
}

static uint32_t lookup(const uint8_t *entry) {
  return entry[0] | entry[1] << 8 | entry[2] << 16;
}

static uint32_t zdis_decode(struct zdis_ctx *ctx) {
  uint8_t opc, x, y, z, extra = 0;
  uint32_t inst;
  ctx->zdis_start_addr = ctx->zdis_end_addr;
  while (true) {
    if (!next(&opc, ctx)) return 0;
    x = opc >> 3 & MASK(0, 3);
    y = opc >> 0 & MASK(0, 3);
    z = opc >> 6 & MASK(0, 2);
    extra = (extra & ~MASK(3, 3)) | (opc & MASK(3, 3));
    if (z != 1) break;
    if (x == y) {
      if (x <= MASK(0, 2)) {
	if (!(extra & MASK(2, 1))) { extra |= MASK(2, 1) | x; continue; }
	ctx->zdis_end_addr--;
	return I(N,O,,N,,I);
      }
      if (x == 6) return I(,,,H,,ALT) | extra << 24;
    }
    return (I(L,D,_,B,_,B) + (x << 10) + (y << 17)) | extra << 24;
  }
  if (z == 2) return (lookup(main_insts[1][x << 3 | 6]) & ~MASK(17, 6)) | (ARG_B + y) << 17 | extra << 24;
  if ((inst = lookup(main_insts[z >> 1][opc & MASK(0, 6)])) > MASK(0, 5)) return inst | extra << 24;
  if (!next(&opc, ctx)) return 0;
  x = opc >> 3 & MASK(0, 3);
  y = opc >> 0 & MASK(0, 3);
  z = opc >> 6 & MASK(0, 2);
  extra = (extra & ~MASK(3, 4)) | (opc & MASK(3, 3)) | (inst << 6 & MASK(6, 1));
  switch (inst) {
    case 0:
      return (lookup(cb_insts[opc >> 3]) + (opc >> 3 != 6 ? y << 17 : 0)) | extra << 24;
    case 1:
      return lookup(ed_insts[opc]) | extra << 24;
    default:
      inst = lookup(xd_insts[opc]);
      if ((inst = lookup(xd_insts[opc]))) return inst | extra << 24;
      extra |= 0x80;
      if ((inst = ctx->zdis_read(ctx, ctx->zdis_end_addr + 1)) > MASK(0, 8)) return 0;
      opc = inst;
      if (opc >> 3 == 6 || (opc & MASK(0, 3)) != 6) return I(T,R,,A,,P) | (uint32_t)extra << 24;
      return (lookup(cb_insts[opc >> 3]) + ((ARG__IXYO - ARG_B) << 17)) | (uint32_t)extra << 24;
  }
}

bool zdis_put_inst(struct zdis_ctx *ctx) {
  uint32_t inst = zdis_decode(ctx);
  uint8_t separator = 0;
  return inst &&
    mne(ctx, inst >> 0 & MNE_MASK) && mne(ctx, inst >> 5 & MNE_MASK) &&
    sep(ctx, inst & MASK(16, 1), false, inst >> 24, &separator) &&
    arg(ctx, inst >> 10 & ARG_MASK, inst >> 24, &separator) &&
    sep(ctx, inst & MASK(23, 1), false, inst >> 24, &separator) &&
    arg(ctx, inst >> 17 & ARG_MASK, inst >> 24, &separator) &&
    sep(ctx, true, true, inst >> 24, &separator);
}

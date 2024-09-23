#include "zdis.h"

#include <stdio.h>
#include <string.h>

enum Options {
    SUFFIXED_IMM = 1 << 0,
    DECIMAL_IMM  = 1 << 1,
    MNE_SPACE    = 1 << 2,
    ARG_SPACE    = 1 << 3,
    COMPUTE_REL  = 1 << 4,
    COMPUTE_ABS  = 1 << 5,
};

static int read(struct zdis_ctx *ctx, uint32_t addr) {
    FILE *file = (void *)ctx->zdis_user_ptr;
    if (fseek(file, addr, SEEK_SET)) {
        return EOF;
    }
    // return 0-255 or EOF on error just like fgetc
    return fgetc(file);
}

static bool put(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il) {
    char pattern[8], *p = pattern;
    switch (kind) {
        case ZDIS_PUT_REL: // JR/DJNZ targets
            val += ctx->zdis_end_addr;
            if (ctx->zdis_user_size & COMPUTE_REL) {
                return put(ctx, ZDIS_PUT_WORD, val, il);
            }
            if (putchar('$') == EOF) {
                return false;
            }
            val -= ctx->zdis_start_addr;
            // fallthrough
        case ZDIS_PUT_OFF: // immediate offsets from index registers
            if (val > 0) {
                *p++ = '+';
            } else if (val < 0) {
                *p++ = '-';
                val = -val;
            } else {
                return true;
            }
            // fallthrough
        case ZDIS_PUT_BYTE: // byte immediates
        case ZDIS_PUT_PORT: // immediate ports
        case ZDIS_PUT_RST: // RST targets
            if (ctx->zdis_user_size & DECIMAL_IMM) {
                *p++ = '%';
                *p++ = 'u';
                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    *p++ = 'd';
                }
            } else {
                *p++ = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
                *p++ = '%';
                *p++ = '0';
                *p++ = '2';
                *p++ = ctx->zdis_lowercase ? 'x' : 'X';
                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    *p++ = 'h';
                }
            }
            *p = '\0';
            return printf(pattern, val) >= 0;
        case ZDIS_PUT_ABS: // JP/CALL immediate targets
            if (ctx->zdis_user_size & COMPUTE_ABS) {
                int32_t extend = il ? 8 : 16;
                return putchar('$') != EOF && put(ctx, ZDIS_PUT_OFF, (int32_t)(val - ctx->zdis_start_addr) << extend >> extend, il);
            }
            // fallthrough
        case ZDIS_PUT_WORD: // word immediates (il ? 24 : 16) bits wide
        case ZDIS_PUT_ADDR: // load/store immediate addresses
            if (ctx->zdis_user_size & DECIMAL_IMM) {
                *p++ = '%';
                *p++ = 'u';
                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    *p++ = 'd';
                }
            } else {
                *p++ = ctx->zdis_user_size & SUFFIXED_IMM ? '0' : '$';
                *p++ = '%';
                *p++ = '0';
                *p++ = il ? '6' : '4';
                *p++ = ctx->zdis_lowercase ? 'x' : 'X';
                if (ctx->zdis_user_size & SUFFIXED_IMM) {
                    *p++ = 'h';
                }
            }
            *p = '\0';
            return printf(pattern, val) >= 0;
        case ZDIS_PUT_CHAR: // one character of mnemonic, register, or parentheses
            return putchar(val) != EOF;
        case ZDIS_PUT_MNE_SEP: // between mnemonic and arguments
            return putchar(ctx->zdis_user_size & MNE_SPACE ? ' ' : '\t') != EOF;
        case ZDIS_PUT_ARG_SEP: // between two arguments
            return fputs(ctx->zdis_user_size & ARG_SPACE ? ", " : ",", stdout) != EOF;
        case ZDIS_PUT_END: // at end of instruction
            return putchar('\n') != EOF;
    }
    // return false for error
    return false;
}

int main(int argc, char **argv) {
    struct zdis_ctx ctx;
    uint32_t end = UINT32_C(0xFFFFFF);;

    if (argc < 2) {
        fprintf(stderr, "usage: %s [args] <file>\n", argv[0]);
        return 1;
    }

    ctx.zdis_read = read; // callback for getting bytes to disassemble
    ctx.zdis_put = put; // callback for processing disassebly output
    ctx.zdis_start_addr = 0; // starting address of the current instruction
    ctx.zdis_end_addr = 0; // ending address of the current instruction
    ctx.zdis_lowercase = true; // automatically convert ZDIS_PUT_CHAR characters to lowercase
    ctx.zdis_implicit = true; // omit certain destination arguments as per z80 style assembly
    ctx.zdis_adl = false; // default word width when not overridden by suffix
    ctx.zdis_user_ptr = NULL; // arbitrary use
    ctx.zdis_user_size = 0; // arbitrary use

    while (*++argv) {
        if (!strcmp(*argv, "--start")) { // address at which to start disassembly
            ctx.zdis_end_addr = atoi(*++argv);
        } else if (!strcmp(*argv, "--end")) { // address at which to end disassembly
            end = atoi(*++argv);
        } else if (!strcmp(*argv, "--lowercase")) { // lowercase alpha
            ctx.zdis_lowercase = true;
        } else if (!strcmp(*argv, "--uppercase")) { // uppercase alpha
            ctx.zdis_lowercase = false;
        } else if (!strcmp(*argv, "--implicit-dest")) { // z80-style omitted dest (or a)
            ctx.zdis_implicit = true;
        } else if (!strcmp(*argv, "--explicit-dest")) { // ez80-style dest (or a,a)
            ctx.zdis_implicit = false;
        } else if (!strcmp(*argv, "--ez80")) { // assume adl = 1
            ctx.zdis_adl = true;
        } else if (!strcmp(*argv, "--z80")) { // assume adl = 0
            ctx.zdis_adl = false;
        } else if (!strcmp(*argv, "--suffix")) { // suffix immediates with base indicator (0FFh)
            ctx.zdis_user_size |= SUFFIXED_IMM;
        } else if (!strcmp(*argv, "--prefix")) { // prefix immediates with base indicator ($FF)
            ctx.zdis_user_size &= ~SUFFIXED_IMM;
        } else if (!strcmp(*argv, "--decimal")) { // base-10 immediates (255)
            ctx.zdis_user_size |= DECIMAL_IMM;
        } else if (!strcmp(*argv, "--hex")) { // base-16 immediates ($FF)
            ctx.zdis_user_size &= ~DECIMAL_IMM;
        } else if (!strcmp(*argv, "--mnemonic-space")) { // space after mnemonic
            ctx.zdis_user_size |= MNE_SPACE;
        } else if (!strcmp(*argv, "--mnemonic-tab")) { // tab after mnemonic
            ctx.zdis_user_size &= ~MNE_SPACE;
        } else if (!strcmp(*argv, "--argument-space")) { // space after argument comma (ld a, b)
            ctx.zdis_user_size |= ARG_SPACE;
        } else if (!strcmp(*argv, "--no-argument-space")) { // no space after argument comma (ld a,b)
            ctx.zdis_user_size &= ~ARG_SPACE;
        } else if (!strcmp(*argv, "--compute-relative")) { // compute PC relative addresses (jr $1234)
            ctx.zdis_user_size |= COMPUTE_REL;
        } else if (!strcmp(*argv, "--literal-relative")) { // literal PC relative addresses (jr $+3)
            ctx.zdis_user_size &= ~COMPUTE_REL;
        } else if (!strcmp(*argv, "--compute-absolute")) { // compute PC relative addresses (jp $+3)
            ctx.zdis_user_size |= COMPUTE_ABS;
        } else if (!strcmp(*argv, "--literal-absolute")) { // literal PC relative addresses (jp $1234)
            ctx.zdis_user_size &= ~COMPUTE_ABS;
        } else {
            if (ctx.zdis_user_ptr) {
                fputs("error: multiple files specified\n", stderr);
                return 1;
            }
            if (!(ctx.zdis_user_ptr = (void *)fopen(*argv, "r"))) {
                fprintf(stderr, "error: file not found: %s\n", *argv);
                return 1;
            }
        }
    }

    while (ctx.zdis_end_addr <= end && putchar('\t') != EOF && zdis_put_inst(&ctx)) {
    }
    return 0;
}

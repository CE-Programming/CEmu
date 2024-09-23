#ifdef DEBUG_SUPPORT

#ifndef ZDIS_H
#define ZDIS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

enum zdis_put {
#define IMM(x) ZDIS_PUT_##x,
#include "imm.def"
  ZDIS_PUT_CHAR,
  ZDIS_PUT_MNE_SEP,
  ZDIS_PUT_ARG_SEP,
  ZDIS_PUT_END,
};

struct zdis_ctx {
  /// Callback invoked to read the raw bytes to be disassembled.
  /// Return the value of the byte at addr or EOF on error.
  int (*zdis_read)(struct zdis_ctx *ctx, uint32_t addr);
  /// Callback invoked to report the parts of disassembled instructions.
  /// Possible values of kind:
  ///   ZDIS_PUT_BYTE:
  ///     An immediate byte val in the range [0, 2^8-1].
  ///     Represents a normal constant byte.
  ///   ZDIS_PUT_PORT:
  ///     An immediate byte val in the range [0, 2^8-1].
  ///     Represents a port address.
  ///     Surrounding parentheses are reported by ZDIS_PUT_CHAR.
  ///   ZDIS_PUT_OFF:
  ///     An immediate byte val in the range [-2^7, 2^7-1].
  ///     Represents an index register offset.
  ///     The register itself is reported by ZDIS_PUT_CHAR.
  ///   ZDIS_PUT_REL:
  ///     An immediate byte val in the range [-2^7, 2^7-1],
  ///     Represents a JR or DJNZ target address which is relative to ctx->zdis_end_addr.
  ///   ZDIS_PUT_WORD:
  ///     An immediate word val in the range [0, 2^16-1] if il is false or [0, 2^24-1] if il is true.
  ///     Represents a normal constant word.
  ///   ZDIS_PUT_ADDR:
  ///     An immediate word val in the range [0, 2^16-1] if il is false or [0, 2^24-1] if il is true.
  ///     Represents a memory address.
  ///     Surrounding parentheses are reported by ZDIS_PUT_CHAR.
  ///   ZDIS_PUT_ABS:
  ///     An immediate word val in the range [0, 2^16-1] if il is false or [0, 2^24-1] if il is true.
  ///     Represents an absolute JP or CALL target address.
  ///   ZDIS_PUT_RST:
  ///     An encoded immediate val which is one of {0, 8, 16, 24, 32, 40, 48, 56}.
  ///     Represents an absolute RST target address.
  ///   ZDIS_PUT_CHAR:
  ///     A single ascii character val.
  ///     This is either part
  ///   ZDIS_PUT_MNE_SEP:
  ///     Represents the separator between the mnemonic and the arguments.
  ///   ZDIS_PUT_ARG_SEP:
  ///     Represents the separator between multiple arguments.
  ///   ZDIS_PUT_END:
  ///     Represents the end of an instruction.
  /// Return true on success or false on error.
  bool (*zdis_put)(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val, bool il);
  /// The starting and ending addresses of the current instruction.
  /// When disassembling a new instruction, zdis_end_addr is used as the
  /// new zdis_start_addr to simplify disassembling consecutive instructions.
  uint32_t zdis_start_addr, zdis_end_addr;
  /// Whether to automatically convert ZDIS_PUT_CHAR characters to lowercase.
  bool zdis_lowercase : 1;
  /// Whether to omit implicit arguments that were omitted in z80-style assembly.
  bool zdis_implicit : 1;
  /// What to assume as the current operating mode of the cpu.
  /// This corresponds with .assume adl = 0/1 in assembly.
  bool zdis_adl : 1;
  /// The following two fields are never used and can store arbitrary user data.
  uint8_t *zdis_user_ptr;
  size_t zdis_user_size;
};

/// Return the size of the instruction starting at ctx->zdis_end_addr.
/// Sets ctx->zdis_start_addr to the address of the first byte of this
/// instruction and sets ctx->zdis_end_addr to one byte past the end.
/// Uses ctx->zdis_read to fetch bytes, and does not report any
/// disassembly information to ctx->zdis_put.
int8_t zdis_inst_size(struct zdis_ctx *ctx);

/// Disassemble the instruction starting at ctx->zdis_end_addr.
/// Sets ctx->zdis_start_addr to the address of the first byte of this
/// instruction and sets ctx->zdis_end_addr to one byte past the end.
/// Uses ctx->zdis_read to fetch bytes and ctx->zdis_put to report
/// different parts of the disassembled instruction according to
/// other settings in ctx.
bool zdis_put_inst(struct zdis_ctx *ctx);

#ifdef __cplusplus
}
#endif

#endif

#endif

#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>

enum zdis_put {
#define IMM(x) ZDIS_PUT_##x,
#include "imm.def"
  ZDIS_PUT_CHAR,
  ZDIS_PUT_FIRST_IMM = ZDIS_PUT_BYTE,
};

struct zdis_ctx {
  int (*zdis_read)(struct zdis_ctx *ctx, uint32_t addr);
  bool (*zdis_put)(struct zdis_ctx *ctx, enum zdis_put kind, int32_t val);
  uint32_t zdis_start_addr, zdis_end_addr;
  char *zdis_separators[3];
  bool zdis_lowercase : 1;
  bool zdis_implicit : 1;
  bool zdis_adl : 1;
  uint8_t *zdis_user_ptr;
  size_t zdis_user_size;
};

bool zdis_put_inst(struct zdis_ctx *ctx);

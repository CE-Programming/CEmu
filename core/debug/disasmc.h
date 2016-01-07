#ifndef DISASMC_H
#define DISASMC_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

typedef struct {
  bool hit_pc;
  bool hit_read_breakpoint;
  bool hit_write_breakpoint;
  bool hit_exec_breakpoint;
} disasm_highlights_state_t;

extern disasm_highlights_state_t disasmHighlight;

#ifdef __cplusplus
}
#endif

#endif

#ifndef JIT_H
#define JIT_H

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void jitFlush(void);
void jitReportWrite(uint32_t address, uint8_t value);
bool jitTryExecute(void);

#ifdef __cplusplus
}
#endif

#endif

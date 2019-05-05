#if defined(JIT_SUPPORT) && defined(JIT_BACKEND_ARM)

#include "jit.h"

void jitFlush(void) {}
void jitReportWrite(uint32_t address, uint8_t value) {}
bool jitTryExecute(void) { return false; }

#endif

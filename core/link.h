#ifndef LINK_H
#define LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vat.h"

enum { LINK_RAM=0, LINK_ARCH, LINK_FILE };
enum { LINK_GOOD=0, LINK_WARN, LINK_ERR };

int emu_send_variable(const char *file, int location);
int emu_send_variables(const char **files, int num, int location);
int emu_receive_variable(const char *file, const calc_var_t *vars, int count);

#ifdef __cplusplus
}
#endif

#endif

#ifndef LINK_H
#define LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vat.h"

#include <stdbool.h>

extern volatile bool emu_is_sending;
extern volatile bool emu_is_recieving;

void enterVariableLink(void);
bool listVariablesLink(void);
bool sendVariableLink(const char *var_name);
bool receiveVariableLink(int count, const calc_var_t *vars, const char *file_name);


#ifdef __cplusplus
}
#endif

#endif

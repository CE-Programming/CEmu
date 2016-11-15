#ifndef LINK_H
#define LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

#include "defines.h"
#include "vat.h"

enum dest_location { LINK_RAM=0, LINK_ARCH, LINK_FILE };

extern volatile bool isSending;
extern volatile bool isReceiving;

void enterVariableLink(void);
bool listVariablesLink(void);
bool sendVariableLink(const char *var_name, unsigned location);
bool receiveVariableLink(int count, const calc_var_t *vars, const char *file_name);

#ifdef __cplusplus
}
#endif

#endif

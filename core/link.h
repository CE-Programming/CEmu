#ifndef LINK_H
#define LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

extern volatile bool emu_is_sending;

void enterVariableLink(void);
bool sendVariableLink(const char *var_name);

#ifdef __cplusplus
}
#endif

#endif

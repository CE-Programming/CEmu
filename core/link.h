#ifndef LINK_H
#define LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "vat.h"
#include "usb/device.h"

enum { LINK_RAM=0, LINK_ARCH, LINK_FILE };
enum { LINK_GOOD=0, LINK_WARN, LINK_ERR };

int emu_send_variable(const char *file, int location);
int emu_send_variables(const char *const *files, int num, int location,
                       usb_progress_handler_t *progress_handler, void *progress_context);
int emu_receive_variable(const char *file, const calc_var_t *vars, int count);
int emu_abort_variable_transfer(void);

#ifdef __cplusplus
}
#endif

#endif

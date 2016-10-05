#ifndef STEPPING_H
#define STEPPING_H

#ifdef __cplusplus
extern "C" {
#endif

void debug_set_step_in(void);
void debug_set_step_out(void);
void debug_set_step_over(void);
void debug_set_step_next(void);

#ifdef __cplusplus
}
#endif

#endif

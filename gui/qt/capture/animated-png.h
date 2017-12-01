#ifndef APNG_H
#define APNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>

typedef struct {
    FILE *tmp;
    unsigned int frameskip;
    unsigned int n;
    bool recording;
    unsigned int time;
} apng_t;

bool apng_start(const char *tmp_name, unsigned int frameskip);
void apng_add_frame(void);
bool apng_stop(void);

#ifdef __cplusplus
}
#endif

#endif

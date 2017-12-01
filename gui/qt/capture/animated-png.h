#ifndef APNG_H
#define APNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../../../core/lcd.h"
#include "libpng-apng/png.h"

typedef struct {
    FILE *tmp;
    png_uint_16 num;
    png_uint_16 den;
    unsigned int n;
    bool recording;
    unsigned int time;
    uint8_t frame[LCD_FRAME_SIZE];
    uint8_t prev[LCD_FRAME_SIZE];
    char *name;
} apng_t;

bool apng_start(const char *tmp_name, int, int);
void apng_add_frame(void);
bool apng_stop(void);
bool apng_save(const char *filename);

#ifdef __cplusplus
}
#endif

#endif

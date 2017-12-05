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

#define TABLE_SIZE 521 // should be a prime >= 256*2

typedef struct {
    FILE *tmp;
    png_uint_16 num, den, delay;
    uint32_t n;
    bool recording;
    unsigned int frameskip;
    unsigned int skipped;
    uint32_t table[TABLE_SIZE];
    png_color frame[LCD_HEIGHT][LCD_WIDTH], prev[LCD_HEIGHT][LCD_WIDTH];
    png_bytep row_ptrs[LCD_HEIGHT];
} apng_t;

bool apng_start(const char *tmp_name, int, int);
void apng_add_frame(void);
bool apng_stop(void);
bool apng_save(const char *filename, bool optimize);

#ifdef __cplusplus
}
#endif

#endif

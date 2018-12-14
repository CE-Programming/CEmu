#ifndef APNG_H
#define APNG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdint.h>

#include "../../../core/lcd.h"

#ifdef PNG_SUPPORT
#include "png.h"
#endif

#ifdef PNG_WRITE_APNG_SUPPORTED

#define TABLE_SIZE 521 /* should be a prime >= 256*2 */

typedef struct {
    FILE *tmp;
    uint32_t n;
    bool recording;
    unsigned int frameskip;
    unsigned int skipped;
    uint32_t table[TABLE_SIZE], frame[LCD_HEIGHT][LCD_WIDTH], prev[LCD_HEIGHT][LCD_WIDTH];
    uint64_t prev_time;
    png_bytep row_ptrs[LCD_HEIGHT];
} apng_t;

bool apng_start(const char *tmp_name, int frameskip);
void apng_add_frame(const void *frame);
bool apng_stop(void);
bool apng_save(const char *filename, bool optimize);

#endif

#ifdef __cplusplus
}
#endif

#endif

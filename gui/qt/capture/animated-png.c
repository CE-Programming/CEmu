#include "animated-png.h"
#include "libpng-apng/png.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#define PNG_BPP 16
#define PNG_WIDTH 320
#define PNG_HEIGHT 240

apng_t apng;

bool apng_start(const char *tmp_name, int fps, int frameskip) {

    // copy name for later
    apng.name = strdup(tmp_name);

    // temp file used for saving rgb888 data rather than storing everything in ram
    if (!(apng.tmp = fopen(apng.name, "wb"))) {
        return false;
    }

    // set delay rate
    apng.num = frameskip + 1;
    apng.den = fps;
    apng.frameskip = frameskip;

    // init recording items
    apng.recording = true;
    apng.skipped = 0;
    apng.n = 0;

    return true;
}

void apng_add_frame(void) {
    if (!apng.recording) {
        return;
    }

    if (!apng.skipped--) {
        apng.skipped = apng.frameskip;

        // write frame to temp file
        fwrite(lcd.frame, 1, LCD_FRAME_SIZE, apng.tmp);

        apng.n++;
    }
}

bool apng_stop(void) {
    apng.recording = false;
    return true;
}

bool apng_save(const char *filename) {
    uint8_t op = PNG_DISPOSE_OP_NONE;
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep *row_ptrs;
    FILE *f;

    unsigned int k, a;

    fclose(apng.tmp);
    if (!(apng.tmp = fopen(apng.name, "rb"))) {
        return false;
    }

    if (!(f = fopen(filename, "wb"))) {
        return false;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    setjmp(png_jmpbuf(png_ptr));
    png_init_io(png_ptr, f);

    png_set_IHDR(png_ptr, info_ptr, LCD_WIDTH, LCD_HEIGHT, 8, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);

    png_set_acTL(png_ptr, info_ptr, apng.n, 0);
    png_write_info(png_ptr, info_ptr);

    row_ptrs = (png_bytepp)png_malloc(png_ptr, sizeof(png_bytep) * LCD_HEIGHT);

    for (k = 0; k < LCD_HEIGHT; k++) {
        row_ptrs[k] = apng.frame + (k * LCD_WIDTH * LCD_RGB_SIZE);
    }

    for (a = 0; a < apng.n; a++) {
        if (fread(apng.frame, 1, LCD_FRAME_SIZE, apng.tmp) != LCD_FRAME_SIZE) { goto err; };

        op = PNG_DISPOSE_OP_NONE;

        if (a && !memcmp(apng.frame, apng.prev, LCD_FRAME_SIZE)) {
            op = PNG_DISPOSE_OP_PREVIOUS;
        }

        png_write_frame_head(png_ptr, info_ptr, row_ptrs, LCD_WIDTH, LCD_HEIGHT, 0, 0, apng.num, apng.den, op, PNG_BLEND_OP_SOURCE);
        png_write_image(png_ptr, row_ptrs);
        png_write_frame_tail(png_ptr, info_ptr);

        memcpy(apng.prev, apng.frame, LCD_FRAME_SIZE);
    }

    png_write_end(png_ptr, info_ptr);
    png_destroy_write_struct(&png_ptr, &info_ptr);

err:
    free(row_ptrs);
    fclose(f);

    fclose(apng.tmp);
    apng.tmp = NULL;
    free(apng.name);
    apng.name = NULL;

    return true;
}

#include "animated-png.h"

#ifdef PNG_WRITE_APNG_SUPPORTED

#include <assert.h>
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

    // temp file used for saving rgb888 data rather than storing everything in ram
    if (!(apng.tmp = fopen(tmp_name, "w+b"))) {
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
        if (!apng.n || 0xFFFFu - apng.delay < apng.num || memcmp(lcd.cntrl.frame, apng.frame, LCD_FRAME_SIZE)) {
            if (apng.n) {
                fwrite(&apng.delay, sizeof(apng.delay), 1, apng.tmp);
            }
            apng.delay = 0;
            memcpy(apng.frame, lcd.cntrl.frame, LCD_FRAME_SIZE);
            fwrite(apng.frame, 1, LCD_FRAME_SIZE, apng.tmp);
            apng.n++;
        }

        apng.delay += apng.num;
    }
}

bool apng_stop(void) {
    fwrite(&apng.delay, sizeof(apng.delay), 1, apng.tmp);
    apng.recording = false;
    return true;
}

bool apng_save(const char *filename, bool optimize) {
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep dst;
    png_colorp prev, cur, palette;
    png_color_16 trans;
    FILE *f;

    bool have_trans = true;
    uint32_t count = ~0, pixel = 0, i, j, probe, hash;
    int x, y;
    struct { int x[2], y[2]; } frame;

    if (optimize) {
        rewind(apng.tmp);
        memset(apng.table, 0, sizeof(apng.table));
        count = 1;
        for (i = 0; i != apng.n; i++) {
            for (j = 0; j != LCD_SIZE; j++) {
                size_t size = fread(&pixel, 3, 1, apng.tmp);
                if (size != 1) {
                    fclose(apng.tmp);
                    apng.tmp = NULL;
                    return false;
                }
                if (pixel == 0xFEFEFE) {
                    have_trans = false;
                }
                if (count > 1 << 8) {
                    if (have_trans) {
                        continue;
                    } else {
                        break;
                    }
                }
                hash = pixel % TABLE_SIZE;
                probe = 1;
                while (true) {
                    if (!apng.table[hash]) { // empty
                        apng.table[hash] = count++ << 24 | (pixel & UINT32_C(0xFFFFFF));
                        break;
                    }
                    if ((apng.table[hash] & UINT32_C(0xFFFFFF)) == pixel) {
                        break;
                    }
                    if (!probe) {
                        assert(count & ~0xFF);
                        break;
                    }
                    hash += probe;
                    if (hash >= TABLE_SIZE) {
                        hash -= TABLE_SIZE;
                    }
                    probe += 2;
                    if (probe >= TABLE_SIZE) {
                        probe -= TABLE_SIZE;
                    }
                }
            }
            fseek(apng.tmp, sizeof(apng.delay), SEEK_CUR);
        }
    }
    rewind(apng.tmp);

    if (!(f = fopen(filename, "wb"))) {
        fclose(apng.tmp);
        apng.tmp = NULL;
        return false;
    }

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    info_ptr = png_create_info_struct(png_ptr);
    if (setjmp(png_jmpbuf(png_ptr))) { goto err; }
    png_init_io(png_ptr, f);

    png_set_IHDR(png_ptr, info_ptr, LCD_WIDTH, LCD_HEIGHT, count <= 1 << 1 ? 1 : count <= 1 << 2 ? 2 : count <= 1 << 4 ? 4 : 8,
                 count <= 1 << 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    if (count <= 1 << 8) {
        palette = &apng.prev[0][0];
        for (i = 0; i != TABLE_SIZE; i++) {
            if (apng.table[i]) {
                j = apng.table[i] >> 24;
                palette[j].red = apng.table[i];
                palette[j].green = apng.table[i] >> 8;
                palette[j].blue = apng.table[i] >> 16;
            }
        }
        palette[0].red = palette[0].green = palette[0].blue = 0; // transparent
        png_set_PLTE(png_ptr, info_ptr, palette, count);
        png_set_tRNS(png_ptr, info_ptr, &palette[0].red, 1, NULL);
    } else if (have_trans) {
        trans.red = trans.green = trans.blue = 0xFE; // transparent
        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans);
    }

    png_set_acTL(png_ptr, info_ptr, apng.n, 0);
    png_write_info(png_ptr, info_ptr);
    png_set_packing(png_ptr);

    for (i = 0; i != apng.n; i++) {
        if (fread(apng.frame, 1, LCD_FRAME_SIZE, apng.tmp) != LCD_FRAME_SIZE ||
            fread(&apng.delay, sizeof(apng.delay), 1, apng.tmp) != 1) { goto err; }

        frame.x[0] = LCD_WIDTH - 1;
        frame.y[0] = LCD_HEIGHT - 1;
        frame.x[1] = frame.y[1] = 0;
        prev = &apng.prev[0][0];
        cur = &apng.frame[0][0];
	if (count <= 1 << 8) {
            dst = &cur->red;
            for (y = 0; y != LCD_HEIGHT; y++) {
                apng.row_ptrs[y] = dst;
                for (x = 0; x != LCD_WIDTH; x++) {
                    if (i && cur->red == prev->red && cur->green == prev->green && cur->blue == prev->blue) {
                        *dst++ = 0;
                    } else {
                        if (frame.x[0] > x) {
                            frame.x[0] = x;
                        }
                        if (frame.x[1] < x) {
                            frame.x[1] = x;
                        }
                        if (frame.y[0] > y) {
                            frame.y[0] = y;
                        }
                        if (frame.y[1] < y) {
                            frame.y[1] = y;
                        }
                        pixel = cur->red | cur->green << 8 | cur->blue << 16;
                        hash = pixel % TABLE_SIZE;
                        probe = 1;
                        while (true) {
                            assert(apng.table[hash]);
                            if ((apng.table[hash] & UINT32_C(0xFFFFFF)) == pixel) {
                                *dst++ = apng.table[hash] >> 24;
                                break;
                            }
                            hash += probe;
                            if (hash >= TABLE_SIZE) {
                                hash -= TABLE_SIZE;
                            }
                            probe += 2;
                            if (probe >= TABLE_SIZE) {
                                probe -= TABLE_SIZE;
                            }
                        }
                        *prev = *cur;
                    }
                    prev++;
                    cur++;
                }
            }
            if (frame.x[0] > frame.x[1] || frame.y[0] > frame.y[1]) {
                frame.x[0] = frame.y[0] = 0;
                frame.x[1] = frame.y[1] = -1;
            }
            for (y = frame.y[0]; y <= frame.y[1]; y++) {
                apng.row_ptrs[y] += frame.x[0];
            }
        } else {
            for (y = 0; y != LCD_HEIGHT; y++) {
                apng.row_ptrs[y] = &cur->red;
                for (x = 0; x != LCD_WIDTH; x++) {
                    if (i && cur->red == prev->red && cur->green == prev->green && cur->blue == prev->blue) {
                        if (have_trans) {
                            cur->red = trans.red;
                            cur->green = trans.green;
                            cur->blue = trans.blue;
                        }
                    } else {
                        if (frame.x[0] > x) {
                            frame.x[0] = x;
                        }
                        if (frame.x[1] < x) {
                            frame.x[1] = x;
                        }
                        if (frame.y[0] > y) {
                            frame.y[0] = y;
                        }
                        if (frame.y[1] < y) {
                            frame.y[1] = y;
                        }
                        *prev = *cur;
                    }
                    prev++;
                    cur++;
                }
            }
            if (frame.x[0] > frame.x[1] || frame.y[0] > frame.y[1]) {
                printf("zero on frame %u/%u\n", i, apng.n);
                frame.x[0] = frame.y[0] = 0;
                frame.x[1] = frame.y[1] = -1;
            }
            for (y = frame.y[0]; y <= frame.y[1]; y++) {
                apng.row_ptrs[y] += frame.x[0] * LCD_RGB_SIZE;
            }
        }

        png_write_frame_head(png_ptr, info_ptr, &apng.row_ptrs[frame.y[0]], frame.x[1] - frame.x[0] + 1, frame.y[1] - frame.y[0] + 1,
                             frame.x[0], frame.y[0], apng.delay, apng.den, PNG_DISPOSE_OP_NONE, PNG_BLEND_OP_OVER);
        png_write_image(png_ptr, &apng.row_ptrs[frame.y[0]]);
        png_write_frame_tail(png_ptr, info_ptr);
    }
    png_write_end(png_ptr, info_ptr);

err:
    png_destroy_write_struct(&png_ptr, &info_ptr);
    fclose(f);

    fclose(apng.tmp);
    apng.tmp = NULL;

    return true;
}

#endif

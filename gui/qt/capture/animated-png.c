#include "animated-png.h"

#ifdef PNG_WRITE_APNG_SUPPORTED

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>
#include <stdbool.h>

#include "../../core/schedule.h"

#ifdef _MSC_VER
static inline int clzll(unsigned long long input_num) {
    unsigned long index;
#ifdef _WIN64
    _BitScanReverse64(&index, input_num);
#else  // if we must support 32-bit Windows
    if (input_num > 0xFFFFFFF) {
        _BitScanReverse(&index, (uint32_t)(input_num >> 32));
    } else {
        _BitScanReverse(&index, (uint32_t)(input_num));
        index += 32;
    }
#endif
    return 63 - index;
}
#else
#define clzll(x) __builtin_clzll(x)
#endif

static apng_t apng;

bool apng_start(const char *tmp_name, int frameskip) {
    /* temp file used for saving rgb888 data rather than storing everything in ram */
    if (!(apng.tmp = fopen(tmp_name, "w+b"))) {
        return false;
    }

    /* set frameskip rate */
    apng.frameskip = (unsigned int)frameskip + 1;

    /* init recording items */
    apng.recording = true;
    apng.skipped = 0;
    apng.n = 0;

    return true;
}

static void apng_write_delay(void) {
    uint64_t time = sched_total_time(CLOCK_48M) - apng.prev_time;
    int logo = 48 - clzll(time);
    uint64_t shift = logo > 10 ? (uint64_t)logo : 10;
    png_uint_16 num = time >> shift, den = sched.clockRates[CLOCK_48M] >> shift;
    fwrite(&num, sizeof(num), 1, apng.tmp);
    fwrite(&den, sizeof(den), 1, apng.tmp);
    apng.prev_time += num << shift;
}

void apng_add_frame(const void *frame) {
    if (!apng.recording) {
        return;
    }

    if (!apng.skipped--) {
        apng.skipped = apng.frameskip;

        if (!apng.n || memcmp(frame, apng.frame, sizeof(apng.frame))) {
            if (!apng.n) {
                apng.prev_time = sched_total_time(CLOCK_48M);
            } else {
                apng_write_delay();
            }
            memcpy(apng.frame, frame, sizeof(apng.frame));
            fwrite(apng.frame, 1, sizeof(apng.frame), apng.tmp);
            apng.n++;
        }
    }
}

bool apng_stop(void) {
    apng_write_delay();
    apng.recording = false;
    return true;
}

bool apng_save(const char *filename, bool optimize) {
    png_structp png_ptr;
    png_infop info_ptr;
    png_bytep dst;
    png_uint_16 num, den;
    uint32_t *prev, *cur;
    png_colorp palette;
    png_color_16 trans;
    FILE *f;
    uint32_t count = ~0, pixel = 0;
    bool have_trans = false;
    uint32_t i, j, probe, hash;
    int x, y;
    struct { int x[2], y[2]; } frame;

    png_ptr = png_create_write_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (setjmp(png_jmpbuf(png_ptr))) { goto err; }

    if (optimize) {
        rewind(apng.tmp);
        memset(apng.table, 0, sizeof(apng.table));
        have_trans = true;
        count = 1;
        for (i = 0; i != apng.n; i++) {
            for (j = 0; j != LCD_SIZE; j++) {
                size_t size = fread(&pixel, sizeof(pixel), 1, apng.tmp);
                if (size != 1) {
                    fclose(apng.tmp);
                    apng.tmp = NULL;
                    return false;
                }
                if (pixel == UINT32_C(0xFFFEFEFE)) {
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
                    if (!apng.table[hash]) { /* empty */
                        apng.table[hash] = count++ << 24 | (pixel & UINT32_C(0xFFFFFF));
                        break;
                    }
                    if (!((apng.table[hash] ^ pixel) & UINT32_C(0xFFFFFF))) {
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
            fseek(apng.tmp, sizeof(num) + sizeof(den), SEEK_CUR);
        }
    }
    rewind(apng.tmp);

    if (!(f = fopen(filename, "wb"))) {
        fclose(apng.tmp);
        apng.tmp = NULL;
        return false;
    }

    info_ptr = png_create_info_struct(png_ptr);
    png_init_io(png_ptr, f);

    png_set_IHDR(png_ptr, info_ptr, LCD_WIDTH, LCD_HEIGHT, count <= 1 << 1 ? 1 : count <= 1 << 2 ? 2 : count <= 1 << 4 ? 4 : 8,
                 count <= 1 << 8 ? PNG_COLOR_TYPE_PALETTE : PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT);
    if (count <= 1 << 8) {
        palette = (png_colorp)apng.prev;
        for (i = 0; i != TABLE_SIZE; i++) {
            if (apng.table[i]) {
                j = apng.table[i] >> 24;
                palette[j].red = apng.table[i];
                palette[j].green = apng.table[i] >> 8;
                palette[j].blue = apng.table[i] >> 16;
            }
        }
        palette[0].red = palette[0].green = palette[0].blue = 0; /* transparent */
        png_set_PLTE(png_ptr, info_ptr, palette, count);
        png_set_tRNS(png_ptr, info_ptr, &palette[0].red, 1, NULL);
    } else if (have_trans) {
        trans.red = trans.green = trans.blue = 0xFE; /* transparent */
        png_set_tRNS(png_ptr, info_ptr, NULL, 0, &trans);
    }

    png_set_acTL(png_ptr, info_ptr, apng.n, 0);
    png_write_info(png_ptr, info_ptr);
    if (count <= 1 << 8) {
        png_set_packing(png_ptr);
    } else {
        png_set_filler(png_ptr, 0, PNG_FILLER_AFTER);
    }

    for (i = 0; i != apng.n; i++) {
        if (fread(apng.frame, 1, sizeof(apng.frame), apng.tmp) != sizeof(apng.frame) ||
            fread(&num, sizeof(num), 1, apng.tmp) != 1 ||
            fread(&den, sizeof(den), 1, apng.tmp) != 1) { goto err; }

        frame.x[0] = LCD_WIDTH - 1;
        frame.y[0] = LCD_HEIGHT - 1;
        frame.x[1] = frame.y[1] = 0;
        prev = &apng.prev[0][0];
        cur = &apng.frame[0][0];
	if (count <= 1 << 8) {
            dst = (png_bytep)cur;
            for (y = 0; y != LCD_HEIGHT; y++) {
                apng.row_ptrs[y] = dst;
                for (x = 0; x != LCD_WIDTH; x++) {
                    if (i && !((*cur ^ *prev) & UINT32_C(0xFFFFFF))) {
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
                        hash = *cur % TABLE_SIZE;
                        probe = 1;
                        while (true) {
                            assert(apng.table[hash]);
                            if (!((apng.table[hash] ^ *cur) & UINT32_C(0xFFFFFF))) {
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

            /* Hack around libpng-apng bug that doesn't like 1 row frames */
            if (frame.y[0] == frame.y[1]) {
                if (frame.y[0]) {
                    --frame.y[0];
                } else {
                    ++frame.y[1];
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
                apng.row_ptrs[y] = (png_bytep)cur;
                for (x = 0; x != LCD_WIDTH; x++) {
                    if (i && !((*cur ^ *prev) & UINT32_C(0xFFFFFF))) {
                        if (have_trans) {
                            *cur = trans.red << 16 | trans.green << 8 | trans.blue;
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

            /* Hack around libpng-apng bug that doesn't like 1 row frames */
            if (frame.y[0] == frame.y[1]) {
                if (frame.y[0]) {
                    --frame.y[0];
                } else {
                    ++frame.y[1];
                }
            }

            if (frame.x[0] > frame.x[1] || frame.y[0] > frame.y[1]) {
                frame.x[0] = frame.y[0] = 0;
                frame.x[1] = frame.y[1] = -1;
            }
            for (y = frame.y[0]; y <= frame.y[1]; y++) {
                apng.row_ptrs[y] += frame.x[0] * sizeof(uint32_t);
            }
        }

        png_write_frame_head(png_ptr, info_ptr, &apng.row_ptrs[frame.y[0]], frame.x[1] - frame.x[0] + 1, frame.y[1] - frame.y[0] + 1,
                             frame.x[0], frame.y[0], num, den, PNG_DISPOSE_OP_NONE, PNG_BLEND_OP_OVER);
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

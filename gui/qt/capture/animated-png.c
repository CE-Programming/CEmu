#include "animated-png.h"
#include "libpng-apng/png.h"
#include "../../../core/lcd.h"

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

// start
bool apng_start(const char *tmp_name, unsigned int frameskip) {
    // temp file used for saving rgb888 data rather than storing everything in ram
    if (!(apng.tmp = fopen(tmp_name, "wb"))) {
        return false;
    }

    apng.frameskip = frameskip;
    apng.recording = true;
    apng.time = 0;
    apng.n = 0;

    return true;
}

void apng_add_frame(void) {
    if (!apng.recording || (++apng.n % (apng.frameskip + 1))) {
        return;
    }

    // write frame to temp file
    fwrite(lcd.frame, 1, 320*240*3, apng.tmp);

    // increment number of frames
    apng.n++;
}

bool apng_stop(void) {
    apng.recording = false;
    apng.time = 0;
    apng.n = 0;

    if (apng.tmp) {
        fclose(apng.tmp);
        apng.tmp = NULL;
    }

    return true;
}

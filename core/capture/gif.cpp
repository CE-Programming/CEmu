#ifndef EMBEDED_DEVICE

#include <array>
#include <mutex>
#include <vector>

#include "../emu.h"
#include "../lcd.h"
#include "gif.h"
#include "giflib.h"

struct RGB24 {
    uint8_t r, g, b, a;
};

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static unsigned int framenr = 0, framenrskip = 0, framedelay = 0;
static RGB24 buffer[320*240];

bool gif_single_frame(const char *filename) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    uint16_t *ptr16 = lcd.framebuffer;
    RGB24 *ptr24 = buffer;
    for(unsigned int i = 0; i < 320*240; ++i) {
        ptr24->r = (*ptr16 & 0b1111100000000000) >> 8;
        ptr24->g = (*ptr16 & 0b0000011111100000) >> 3;
        ptr24->b = (*ptr16 & 0b0000000000011111) << 3;
        ++ptr24;
        ++ptr16;
    }

    if (!GifBegin(&writer, filename, 320, 240, 0)) {
        return false;
    }
    if (!GifWriteFrame(&writer, reinterpret_cast<const uint8_t*>(buffer), 320, 240, 0))  {
        return false;
    }
    if (!GifEnd(&writer)) {
        return false;
    }

    return true;
}

bool gif_start_recording(const char *filename, unsigned int frameskip) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    framenr = framenrskip = frameskip;
    framedelay = 100 / (60/(frameskip+1));

    if(GifBegin(&writer, filename, 320, 240, framedelay)) {
        recording = true;
        gui_console_printf("Started recording GIF image.\n");
    }

    return recording;
}

void gif_new_frame() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    if(!recording || --framenr) {
        return;
    }

    framenr = framenrskip;

    static uint32_t bitfields[] = { 0x01F, 0x000, 0x000};
    lcd_drawframe(lcd.framebuffer, bitfields);

    uint16_t *ptr16 = lcd.framebuffer;
    RGB24 *ptr24 = buffer;
    for(unsigned int i = 0; i < 320*240; ++i) {
        ptr24->r = (*ptr16 & 0b1111100000000000) >> 8;
        ptr24->g = (*ptr16 & 0b0000011111100000) >> 3;
        ptr24->b = (*ptr16 & 0b0000000000011111) << 3;
        ++ptr24;
        ++ptr16;
    }

    if(!GifWriteFrame(&writer, reinterpret_cast<const uint8_t*>(buffer), 320, 240, framedelay)) {
        recording = false;
    }
}

bool gif_stop_recording() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool ret = recording;

    recording = false;

    GifEnd(&writer);

    gui_console_printf("Done recording GIF image.\n");
    return ret;
}

#endif

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
static std::vector<RGB24> buffer;
static unsigned int framenr = 0, framenrskip = 0, framedelay = 0;

bool gif_start_recording(const char *filename, unsigned int frameskip)
{
    std::lock_guard<std::mutex> lock(gif_mutex);

    framenr = framenrskip = frameskip;
    framedelay = 100 / (60/(frameskip+1));

    if(GifBegin(&writer, filename, 320, 240, framedelay))
        recording = true;

    buffer.resize(320*240);

    gui_console_printf("Started recording GIF image.\n");
    return recording;
}

void gif_new_frame()
{
    if(!recording) {
        return;
    }

    std::lock_guard<std::mutex> gif_lock(gif_mutex);

    if(!recording || --framenr) {
        return;
    }

    framenr = framenrskip;

    static std::array<uint16_t, 320 * 240> framebuffer;

    uint32_t bitfields[] = { 0x01F, 0x000, 0x000};

    lcd_drawframe(framebuffer.data(), bitfields);

    uint16_t *ptr16 = framebuffer.data();
    RGB24 *ptr24 = buffer.data();
    for(unsigned int i = 0; i < 320*240; ++i)
    {
        ptr24->r = (*ptr16 & 0xF800) >> 8;
        ptr24->g = (*ptr16 & 0x7E0) >> 3;
        ptr24->b = (*ptr16 & 0x1F) << 3;
        ++ptr24;
        ++ptr16;
    }

    if(!GifWriteFrame(&writer, reinterpret_cast<const uint8_t*>(buffer.data()), 320, 240, framedelay))
        recording = false;
}

bool gif_stop_recording()
{
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool ret = recording;

    recording = false;

    buffer.clear();
    GifEnd(&writer);

    gui_console_printf("Done recording GIF image.\n");
    return ret;
}

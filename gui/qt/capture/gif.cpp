#include <mutex>
#include <QtGui/QImage>

#include "gif.h"
#include "giflib.h"
#include "qtframebuffer.h"

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static unsigned int framenr = 0, framenrskip = 0, framedelay = 0;

static bool gif_write_frame(GifWriter *frameWriter, unsigned int delay) {
    return GifWriteFrame(frameWriter, renderFramebuffer().convertToFormat(QImage::Format_RGBA8888).bits(), 320, 240, delay);
}

bool gif_single_frame(const char *filename) {
    GifWriter frameWriter;
    return GifBegin(&frameWriter, filename, 320, 240, 0) && gif_write_frame(&frameWriter, 0) && GifEnd(&frameWriter);
}

bool gif_start_recording(const char *filename, unsigned int frameskip) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    framenr = framenrskip = frameskip;
    framedelay = 100 / (60/(frameskip+1));

    if(GifBegin(&writer, filename, 320, 240, framedelay)) {
        recording = true;
    }

    return recording;
}

void gif_new_frame() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    if(!recording || --framenr) {
        return;
    }

    framenr = framenrskip;

    if(!gif_write_frame(&writer, framedelay)) {
        recording = false;
    }
}

bool gif_stop_recording() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool ret = recording;

    recording = false;

    GifEnd(&writer);

    return ret;
}

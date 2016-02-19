#include <mutex>
#include <QtGui/QImage>

#include "gif.h"
#include "giflib.h"
#include "qtframebuffer.h"

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static unsigned int frame, frameskip, gifTime;

static bool gif_write_frame(GifWriter *frameWriter, unsigned int delay) {
    return GifWriteFrame(frameWriter, renderFramebuffer(&lcd).convertToFormat(QImage::Format_RGBA8888).bits(), 320, 240, delay);
}

bool gif_single_frame(const char *filename) {
    GifWriter frameWriter;

    return GifBegin(&frameWriter, filename, 320, 240, 0) && gif_write_frame(&frameWriter, 0) && GifEnd(&frameWriter);
}

bool gif_start_recording(const char *filename, unsigned int frameskip_) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    if (GifBegin(&writer, filename, 320, 240, 1)) {
        recording = true;
        frame = 0;
        frameskip = frameskip_;
        gifTime = 0;
    }

    return recording;
}

void gif_new_frame() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    if (!recording || (++frame % (frameskip + 1))) {
        return;
    }

    int lastGifTime = gifTime;
    gifTime = (frame * 100 + 15) / 30;

    if (!gif_write_frame(&writer, gifTime - lastGifTime)) {
        recording = false;
        GifEnd(&writer);
    }
}

bool gif_stop_recording() {
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool wasRecording = recording;
    recording = false;
    return wasRecording && GifEnd(&writer);
}

#include <mutex>
#include <QtGui/QImage>
#include <QtCore/QString>

#include "gif.h"
#include "giflib.h"
#include "../qtframebuffer.h"

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static unsigned int frame, frameskip, gifTime;

Gt_OutputData active_output_data;
Gif_CompressInfo gif_write_info;

static bool gif_write_frame(GifWriter *frameWriter, unsigned int delay) {
    return GifWriteFrame(frameWriter, reinterpret_cast<const uint8_t*>(lcd_framebuffer), LCD_WIDTH, LCD_HEIGHT, delay);
}

bool gif_single_frame(const char *filename) {
    GifWriter frameWriter;

    return GifBegin(&frameWriter, filename, LCD_WIDTH, LCD_HEIGHT, 0) && gif_write_frame(&frameWriter, 0) && GifEnd(&frameWriter);
}

bool gif_start_recording(const char *filename, unsigned int frameskip_) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    if (GifBegin(&writer, filename, LCD_WIDTH, LCD_HEIGHT, 1)) {
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

    unsigned int lastGifTime = gifTime;
    gifTime = (frame * 100 + 32) / 64;

    if (!gif_write_frame(&writer, gifTime - lastGifTime)) {
        recording = false;
        GifEnd(&writer);
    }
}

bool gif_stop_recording(void) {
    std::lock_guard<std::mutex> lock(gif_mutex);

    bool wasRecording = recording;
    recording = false;
    return wasRecording && GifEnd(&writer);
}

bool gif_optimize(const QString &in_name, const QString &out_name) {
    bool ret = false;
    FILE *in = NULL;
    FILE *out = NULL;
    Gif_Stream *gfs = NULL;

    if (!(in = fopen(in_name.toStdString().c_str(), "rb")))   goto err;
    if (!(out = fopen(out_name.toStdString().c_str(), "wb"))) goto err;

    gfs = Gif_ReadFile(in);

    if (!gfs || !Gif_ImageCount(gfs))                         goto err;

    Gif_Optimize(gfs);
    Gif_WriteFile(gfs, out);

    ret = true;

err:
    Gif_DeleteStream(gfs);
    fclose(in);
    fclose(out);
    return ret;
}

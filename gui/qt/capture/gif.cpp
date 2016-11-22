#include <mutex>
#include <QtGui/QImage>

#include "gif.h"
#include "giflib.h"
#include "../qtframebuffer.h"

static std::mutex gif_mutex;
static bool recording = false;
static GifWriter writer;
static unsigned int frame, frameskip, gifTime;

Gt_OutputData active_output_data;
Gif_CompressInfo gif_write_info;
Gt_Frame def_frame;

int nested_mode = 0;
int verbosing = 0;

static bool gif_write_frame(GifWriter *frameWriter, unsigned int delay) {
    return GifWriteFrame(frameWriter, renderFramebuffer(&lcd).bits(), 320, 240, delay);
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

bool gif_optimize(const char *in_name, const char *out_name) {
    bool ret = true;
    FILE *in;
    FILE *out;
    Gif_Stream *gfs;

    if (!(in = fopen(in_name, "rb"))){
        return false;
    }

    if (!(out = fopen(out_name, "wb"))){
        fclose(in);
        return false;
    }

    gfs = Gif_FullReadFile(in, GIF_READ_COMPRESSED, 0, 0);
    Gif_InitCompressInfo(&gif_write_info);

    if (!gfs || (Gif_ImageCount(gfs) == 0 && gfs->errors > 0)) {
      fprintf(stdout,"open error");
      Gif_DeleteStream(gfs);
      ret = false;
      goto err;
    }

    optimize_fragments(gfs, GT_OPT_MASK, 0);
    Gif_FullWriteFile(gfs, &gif_write_info, out);
    Gif_DeleteStream(gfs);

err:
    fclose(in);
    fclose(out);
    return ret;
}

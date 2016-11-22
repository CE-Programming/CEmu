#ifndef GIF_H
#define GIF_H

#include "gifsicle.h"

bool gif_single_frame(const char *filename);
bool gif_start_recording(const char *filename, unsigned int frameskip);
void gif_new_frame();
bool gif_stop_recording();
bool gif_optimize(const char *in_name, const char *out_name);

#endif

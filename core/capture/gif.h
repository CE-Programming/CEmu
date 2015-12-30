#ifndef GIF_H
#define GIF_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>

bool gif_start_recording(const char *filename, unsigned int frameskip);
void gif_new_frame();
bool gif_stop_recording();

#ifdef __cplusplus
}
#endif

#endif

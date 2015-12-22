#ifndef _H_GIF
#define _H_GIF

#ifdef __cplusplus
extern "C" {
#endif

bool gif_start_recording(const char *filename, unsigned int frameskip);
void gif_new_frame();
bool gif_stop_recording();

#ifdef __cplusplus
}
#endif

#endif

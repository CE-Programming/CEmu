#ifndef GIF_H
#define GIF_H

#include <QtCore/QString>
#include "gifsicle.h"

#define Gif_Optimize(a) optimize_fragments(a, GT_OPT_MASK, 0);

bool gif_single_frame(const char *filename);
bool gif_start_recording(const char *filename, unsigned int frameskip);
void gif_new_frame();
bool gif_stop_recording();
bool gif_optimize(const QString &in_name, const QString &out_name);

#endif

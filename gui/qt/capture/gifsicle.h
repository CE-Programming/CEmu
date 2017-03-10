/* gifsicle.h - Function declarations for gifsicle.
   Copyright (C) 1997-2014 Eddie Kohler, ekohler@gmail.com
   This file is part of gifsicle.
   Gifsicle is free software. It is distributed under the GNU Public License,
   version 2; you can copy, distribute, or alter it at will, as long
   as this notice is kept intact and this source code is made available. There
   is no warranty, express or implied. */

#ifndef GIFSICLE_H
#define GIFSICLE_H
#include "lcdfgif/gif.h"
#ifdef __GNUC__
#define NORETURN __attribute__ ((noreturn))
#define USED_ATTR __attribute__ ((used))
#else
#define NORETURN
#define USED_ATTR
#endif

#ifdef __cplusplus
extern "C" {
#endif

typedef struct Gt_Frameset Gt_Frameset;
typedef struct Gt_Crop Gt_Crop;
typedef struct Gt_ColorTransform Gt_ColorTransform;

typedef struct Gt_Frame {

  Gif_Stream *stream;
  Gif_Image *image;
  int use;

  const char *name;
  int no_name;
  Gif_Comment *comment;
  int no_comments;

  Gif_Color transparent;        /* also background */
  int interlacing;
  int left;
  int top;

  Gt_Crop *crop;
  int left_offset;
  int top_offset;

  int delay;
  int disposal;

  Gt_Frameset *nest;
  int explode_by_name;

  int no_extensions;
  int no_app_extensions;
  Gif_Extension *extensions;

  unsigned flip_horizontal: 1;
  unsigned flip_vertical: 1;
  unsigned info_flags: 3;
  unsigned position_is_offset: 1;
  unsigned total_crop: 1;
  unsigned rotation;

  const char *input_filename;

} Gt_Frame;


struct Gt_Frameset {
  int count;
  int cap;
  Gt_Frame *f;
};


struct Gt_Crop {
  int ready;
  int transparent_edges;
  int spec_x;
  int spec_y;
  int spec_w;
  int spec_h;
  int x;
  int y;
  int w;
  int h;
  int left_offset;
  int top_offset;
};


typedef void (*colormap_transform_func)(Gif_Colormap *, void *);

struct Gt_ColorTransform {
  Gt_ColorTransform *prev;
  Gt_ColorTransform *next;
  colormap_transform_func func;
  void *data;
};


typedef struct {

  const char *output_name;
  const char *active_output_name;

  int screen_width;
  int screen_height;

  Gif_Color background;
  int loopcount;

  int colormap_size;
  Gif_Colormap *colormap_fixed;
  int colormap_algorithm;
  int colormap_needs_transparency;
  int dither_type;
  const uint8_t* dither_data;
  const char* dither_name;
  int colormap_gamma_type;
  double colormap_gamma;

  int optimizing;

  int scaling;
  int resize_width;
  int resize_height;
  double scale_x;
  double scale_y;
  int scale_method;
  int scale_colors;

  int conserve_memory;

} Gt_OutputData;

extern Gt_OutputData active_output_data;

#define GT_OPT_MASK             0xFFFF
#define GT_OPT_KEEPEMPTY        0x10000

/*****
 * helper
 **/

static inline int
constrain(int low, int x, int high)
{
  return x < low ? low : (x < high ? x : high);
}


/*****
 * error & verbose
 **/
extern Gif_CompressInfo gif_write_info;

/*****
 * merging images
 **/
void    unmark_colors(Gif_Colormap *);

/*****
 * image/colormap transformations
 **/
void optimize_fragments(Gif_Stream *, int optimizeness, int huge_stream);

/*****
 * quantization
 **/
#define KC_GAMMA_SRGB                   0
#define KC_GAMMA_NUMERIC                1
void    kc_set_gamma(int type, double gamma);

#define COLORMAP_DIVERSITY              0
#define COLORMAP_BLEND_DIVERSITY        1
#define COLORMAP_MEDIAN_CUT             2

enum {
    dither_none = 0, dither_default, dither_floyd_steinberg,
    dither_ordered, dither_ordered_new
};
int     set_dither_type(Gt_OutputData* od, const char* name);
void    colormap_stream(Gif_Stream*, Gif_Colormap*, Gt_OutputData*);

#ifdef __cplusplus
}
#endif

#endif

/* xform.c - Image transformation functions for gifsicle.
   Copyright (C) 1997-2015 Eddie Kohler, ekohler@gmail.com
   This file is part of gifsicle.

   Gifsicle is free software. It is distributed under the GNU Public License,
   version 2; you can copy, distribute, or alter it at will, as long
   as this notice is kept intact and this source code is made available. There
   is no warranty, express or implied. */

#include "gifsicle.h"
#include <math.h>

#if HAVE_UNISTD_H
# include <unistd.h>
#endif
#if HAVE_SYS_TYPES_H && HAVE_SYS_STAT_H
# include <sys/types.h>
# include <sys/stat.h>
#endif
#ifndef M_PI
/* -std=c89 does not define M_PI */
# define M_PI           3.14159265358979323846
#endif


/******
 * color transforms
 **/

Gt_ColorTransform *
append_color_transform(Gt_ColorTransform *list,
                       color_transform_func func, void *data)
{
  Gt_ColorTransform *trav;
  Gt_ColorTransform *xform = Gif_New(Gt_ColorTransform);
  xform->next = 0;
  xform->func = func;
  xform->data = data;

  for (trav = list; trav && trav->next; trav = trav->next)
    ;
  if (trav) {
    trav->next = xform;
    return list;
  } else
    return xform;
}

Gt_ColorTransform *
delete_color_transforms(Gt_ColorTransform *list, color_transform_func func)
{
  Gt_ColorTransform *prev = 0, *trav = list;
  while (trav) {
    Gt_ColorTransform *next = trav->next;
    if (trav->func == func) {
      if (prev) prev->next = next;
      else list = next;
      Gif_Delete(trav);
    } else
      prev = trav;
    trav = next;
  }
  return list;
}

void
apply_color_transforms(Gt_ColorTransform *list, Gif_Stream *gfs)
{
  int i;
  Gt_ColorTransform *xform;
  for (xform = list; xform; xform = xform->next) {
    if (gfs->global)
      xform->func(gfs->global, xform->data);
    for (i = 0; i < gfs->nimages; i++)
      if (gfs->images[i]->local)
        xform->func(gfs->images[i]->local, xform->data);
  }
}

/*****
 * crop image; returns true if the image exists
 **/

void
combine_crop(Gt_Crop* dstcrop, const Gt_Crop* srccrop, const Gif_Image* gfi)
{
    int cl = srccrop->x - gfi->left, cr = cl + srccrop->w,
        ct = srccrop->y - gfi->top, cb = ct + srccrop->h,
        dl = cl > 0 ? cl : 0, dr = cr < gfi->width ? cr : gfi->width,
        dt = ct > 0 ? ct : 0, db = cb < gfi->height ? cb : gfi->height;

    if (dl < dr) {
        dstcrop->x = dl;
        dstcrop->w = dr - dl;
    } else {
        dstcrop->x = (cl <= 0 ? 0 : srccrop->w - 1) + (srccrop->left_offset - gfi->left);
        dstcrop->w = 0;
    }
    if (dt < db) {
        dstcrop->y = dt;
        dstcrop->h = db - dt;
    } else {
        dstcrop->y = (ct <= 0 ? 0 : srccrop->h - 1) + (srccrop->top_offset - gfi->top);
        dstcrop->h = 0;
    }
}

int
crop_image(Gif_Image* gfi, Gt_Frame* fr, int preserve_total_crop)
{
    Gt_Crop c;
    int j;

    combine_crop(&c, fr->crop, gfi);

    fr->left_offset = fr->crop->left_offset;
    fr->top_offset = fr->crop->top_offset;

    if (c.w > 0 && c.h > 0) {
        uint8_t** old_img = gfi->img;
        gfi->img = Gif_NewArray(uint8_t *, c.h + 1);
        for (j = 0; j < c.h; j++)
            gfi->img[j] = old_img[c.y + j] + c.x;
        gfi->img[c.h] = 0;
        Gif_DeleteArray(old_img);
        gfi->width = c.w;
        gfi->height = c.h;
    } else if (preserve_total_crop)
        Gif_MakeImageEmpty(gfi);
    else {
        Gif_DeleteArray(gfi->img);
        gfi->img = 0;
        gfi->width = gfi->height = 0;
    }

    gfi->left += c.x - fr->left_offset;
    gfi->top += c.y - fr->top_offset;
    return gfi->img != 0;
}

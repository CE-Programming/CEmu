/* giffunc.c - General functions for the GIF library.
   Copyright (C) 1997-2014 Eddie Kohler, ekohler@gmail.com
   This file is part of the LCDF GIF library.

   The LCDF GIF library is free software. It is distributed under the GNU
   General Public License, version 2; you can copy, distribute, or alter it at
   will, as long as this notice is kept intact and this source code is made
   available. There is no warranty, express or implied. */

#include "lcdfgif/gif.h"
#include "gifsicle.h"
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

Gif_Stream *
Gif_NewStream(void)
{
  Gif_Stream *gfs = Gif_New(Gif_Stream);
  if (!gfs)
    return 0;
  gfs->images = 0;
  gfs->nimages = gfs->imagescap = 0;
  gfs->global = 0;
  gfs->background = 256;
  gfs->screen_width = gfs->screen_height = 0;
  gfs->loopcount = -1;
  gfs->end_comment = 0;
  gfs->end_extension_list = 0;
  gfs->errors = 0;
  gfs->userflags = 0;
  gfs->refcount = 0;
  gfs->landmark = 0;
  return gfs;
}


Gif_Image *
Gif_NewImage(void)
{
  Gif_Image *gfi = Gif_New(Gif_Image);
  if (!gfi)
    return 0;
  gfi->width = gfi->height = 0;
  gfi->img = 0;
  gfi->image_data = 0;
  gfi->left = gfi->top = 0;
  gfi->delay = 0;
  gfi->disposal = GIF_DISPOSAL_NONE;
  gfi->interlace = 0;
  gfi->local = 0;
  gfi->transparent = -1;
  gfi->user_flags = 0;
  gfi->identifier = 0;
  gfi->comment = 0;
  gfi->extension_list = 0;
  gfi->free_image_data = Gif_Free;
  gfi->compressed_len = 0;
  gfi->compressed = 0;
  gfi->free_compressed = 0;
  gfi->user_data = 0;
  gfi->free_user_data = 0;
  gfi->refcount = 0;
  return gfi;
}


Gif_Colormap *
Gif_NewColormap(void)
{
  Gif_Colormap *gfcm = Gif_New(Gif_Colormap);
  if (!gfcm)
    return 0;
  gfcm->ncol = 0;
  gfcm->capacity = 0;
  gfcm->col = 0;
  gfcm->refcount = 0;
  gfcm->userflags = 0;
  return gfcm;
}


Gif_Colormap *
Gif_NewFullColormap(int count, int capacity)
{
  Gif_Colormap *gfcm = Gif_New(Gif_Colormap);
  if (!gfcm || capacity <= 0 || count < 0) {
    Gif_Delete(gfcm);
    return 0;
  }
  if (count > capacity)
    capacity = count;
  gfcm->ncol = count;
  gfcm->capacity = capacity;
  gfcm->col = Gif_NewArray(Gif_Color, capacity);
  gfcm->refcount = 0;
  gfcm->userflags = 0;
  if (!gfcm->col) {
    Gif_Delete(gfcm);
    return 0;
  } else
    return gfcm;
}


Gif_Comment *
Gif_NewComment(void)
{
  Gif_Comment *gfcom = Gif_New(Gif_Comment);
  if (!gfcom)
    return 0;
  gfcom->str = 0;
  gfcom->len = 0;
  gfcom->count = gfcom->cap = 0;
  return gfcom;
}


Gif_Extension *
Gif_NewExtension(int kind, const char* appname, int applength)
{
    Gif_Extension *gfex = Gif_New(Gif_Extension);
    if (!gfex)
        return 0;
    gfex->kind = kind;
    if (appname) {
        gfex->appname = (char*) Gif_NewArray(char, applength + 1);
        if (!gfex->appname) {
            Gif_Delete(gfex);
            return 0;
        }
        memcpy(gfex->appname, appname, applength);
        gfex->appname[applength] = 0;
        gfex->applength = applength;
    } else {
        gfex->appname = 0;
        gfex->applength = 0;
    }
    gfex->data = 0;
    gfex->stream = 0;
    gfex->image = 0;
    gfex->next = 0;
    gfex->free_data = 0;
    gfex->packetized = 0;
    return gfex;
}

Gif_Extension*
Gif_CopyExtension(Gif_Extension* src)
{
    Gif_Extension* dst = Gif_NewExtension(src->kind, src->appname, src->applength);
    if (!dst)
        return NULL;
    if (!src->data || !src->free_data) {
        dst->data = src->data;
        dst->length = src->length;
    } else {
        dst->data = Gif_NewArray(uint8_t, src->length);
        if (!dst->data) {
            Gif_DeleteExtension(dst);
            return NULL;
        }
        memcpy(dst->data, src->data, src->length);
        dst->length = src->length;
        dst->free_data = Gif_Free;
    }
    dst->packetized = src->packetized;
    return dst;
}


char *
Gif_CopyString(const char *s)
{
  int l;
  char *copy;
  if (!s)
    return 0;
  l = strlen(s);
  copy = Gif_NewArray(char, l + 1);
  if (!copy)
    return 0;
  memcpy(copy, s, l + 1);
  return copy;
}


int
Gif_AddImage(Gif_Stream *gfs, Gif_Image *gfi)
{
  if (gfs->nimages >= gfs->imagescap) {
    if (gfs->imagescap)
      gfs->imagescap *= 2;
    else
      gfs->imagescap = 2;
    Gif_ReArray(gfs->images, Gif_Image *, gfs->imagescap);
    if (!gfs->images)
      return 0;
  }
  gfs->images[gfs->nimages] = gfi;
  gfs->nimages++;
  gfi->refcount++;
  return 1;
}

void
Gif_RemoveImage(Gif_Stream *gfs, int inum)
{
  int j;
  if (inum < 0 || inum >= gfs->nimages)
    return;
  Gif_DeleteImage(gfs->images[inum]);
  for (j = inum; j < gfs->nimages - 1; j++)
    gfs->images[j] = gfs->images[j+1];
  gfs->nimages--;
}

int
Gif_AddCommentTake(Gif_Comment *gfcom, char *x, int xlen)
{
  if (gfcom->count >= gfcom->cap) {
    if (gfcom->cap)
      gfcom->cap *= 2;
    else
      gfcom->cap = 2;
    Gif_ReArray(gfcom->str, char *, gfcom->cap);
    Gif_ReArray(gfcom->len, int, gfcom->cap);
    if (!gfcom->str || !gfcom->len)
      return 0;
  }
  if (xlen < 0)
    xlen = strlen(x);
  gfcom->str[ gfcom->count ] = x;
  gfcom->len[ gfcom->count ] = xlen;
  gfcom->count++;
  return 1;
}


int
Gif_AddComment(Gif_Comment *gfcom, const char *x, int xlen)
{
  char *new_x;
  if (xlen < 0)
    xlen = strlen(x);
  new_x = Gif_NewArray(char, xlen);
  if (!new_x)
    return 0;
  memcpy(new_x, x, xlen);
  if (Gif_AddCommentTake(gfcom, new_x, xlen) == 0) {
    Gif_DeleteArray(new_x);
    return 0;
  } else
    return 1;
}


int
Gif_AddExtension(Gif_Stream* gfs, Gif_Image* gfi, Gif_Extension* gfex)
{
    Gif_Extension **pprev;
    if (gfex->stream || gfex->image)
        return 0;
    pprev = gfi ? &gfi->extension_list : &gfs->end_extension_list;
    while (*pprev)
        pprev = &(*pprev)->next;
    *pprev = gfex;
    gfex->stream = gfs;
    gfex->image = gfi;
    gfex->next = 0;
    return 1;
}


void
Gif_CalculateScreenSize(Gif_Stream *gfs, int force)
{
  int i;
  int screen_width = 0;
  int screen_height = 0;

  for (i = 0; i < gfs->nimages; i++) {
    Gif_Image *gfi = gfs->images[i];
    /* 17.Dec.1999 - I find this old behavior annoying. */
    /* if (gfi->left != 0 || gfi->top != 0) continue; */
    if (screen_width < gfi->left + gfi->width)
      screen_width = gfi->left + gfi->width;
    if (screen_height < gfi->top + gfi->height)
      screen_height = gfi->top + gfi->height;
  }

  /* Only use the default 640x480 screen size if we are being forced to create
     a new screen size or there's no screen size currently. */
  if (screen_width == 0 && (gfs->screen_width == 0 || force))
    screen_width = 640;
  if (screen_height == 0 && (gfs->screen_height == 0 || force))
    screen_height = 480;

  if (gfs->screen_width < screen_width || force)
    gfs->screen_width = screen_width;
  if (gfs->screen_height < screen_height || force)
    gfs->screen_height = screen_height;
}


Gif_Colormap *
Gif_CopyColormap(Gif_Colormap *src)
{
  Gif_Colormap *dest;
  if (!src)
    return 0;

  dest = Gif_NewFullColormap(src->ncol, src->capacity);
  if (!dest)
    return 0;

  memcpy(dest->col, src->col, sizeof(src->col[0]) * src->ncol);
  return dest;
}

void Gif_MakeImageEmpty(Gif_Image* gfi) {
    Gif_ReleaseUncompressedImage(gfi);
    Gif_ReleaseCompressedImage(gfi);
    gfi->width = gfi->height = 1;
    gfi->transparent = 0;
    Gif_CreateUncompressedImage(gfi, 0);
    gfi->img[0][0] = 0;
}


/** DELETION **/

void
Gif_DeleteStream(Gif_Stream *gfs)
{
  int i;
  if (!gfs || --gfs->refcount > 0)
    return;

  for (i = 0; i < gfs->nimages; i++)
    Gif_DeleteImage(gfs->images[i]);
  Gif_DeleteArray(gfs->images);

  Gif_DeleteColormap(gfs->global);

  Gif_DeleteComment(gfs->end_comment);
  while (gfs->end_extension_list)
      Gif_DeleteExtension(gfs->end_extension_list);

  Gif_Delete(gfs);
}


void
Gif_DeleteImage(Gif_Image *gfi)
{
  if (!gfi || --gfi->refcount > 0)
    return;

  Gif_DeleteArray(gfi->identifier);
  Gif_DeleteComment(gfi->comment);
  while (gfi->extension_list)
      Gif_DeleteExtension(gfi->extension_list);
  Gif_DeleteColormap(gfi->local);
  if (gfi->image_data && gfi->free_image_data)
    (*gfi->free_image_data)((void *)gfi->image_data);
  Gif_DeleteArray(gfi->img);
  if (gfi->compressed && gfi->free_compressed)
    (*gfi->free_compressed)((void *)gfi->compressed);
  if (gfi->user_data && gfi->free_user_data)
    (*gfi->free_user_data)(gfi->user_data);
  Gif_Delete(gfi);
}


void
Gif_DeleteColormap(Gif_Colormap *gfcm)
{
  if (!gfcm || --gfcm->refcount > 0)
    return;

  Gif_DeleteArray(gfcm->col);
  Gif_Delete(gfcm);
}


void
Gif_DeleteComment(Gif_Comment *gfcom)
{
  int i;
  if (!gfcom)
    return;
  for (i = 0; i < gfcom->count; i++)
    Gif_DeleteArray(gfcom->str[i]);
  Gif_DeleteArray(gfcom->str);
  Gif_DeleteArray(gfcom->len);
  Gif_Delete(gfcom);
}


void
Gif_DeleteExtension(Gif_Extension *gfex)
{
  if (!gfex)
    return;
  if (gfex->data && gfex->free_data)
    (*gfex->free_data)(gfex->data);
  Gif_DeleteArray(gfex->appname);
  if (gfex->stream || gfex->image) {
      Gif_Extension** pprev;
      if (gfex->image)
          pprev = &gfex->image->extension_list;
      else
          pprev = &gfex->stream->end_extension_list;
      while (*pprev && *pprev != gfex)
          pprev = &(*pprev)->next;
      if (*pprev)
          *pprev = gfex->next;
  }
  Gif_Delete(gfex);
}

int
Gif_ColorEq(Gif_Color *c1, Gif_Color *c2)
{
  return GIF_COLOREQ(c1, c2);
}


int
Gif_FindColor(Gif_Colormap *gfcm, Gif_Color *c)
{
  int i;
  for (i = 0; i < gfcm->ncol; i++)
    if (GIF_COLOREQ(&gfcm->col[i], c))
      return i;
  return -1;
}


int
Gif_AddColor(Gif_Colormap *gfcm, Gif_Color *c, int look_from)
{
  int i;
  if (look_from >= 0)
    for (i = look_from; i < gfcm->ncol; i++)
      if (GIF_COLOREQ(&gfcm->col[i], c))
        return i;
  if (gfcm->ncol >= gfcm->capacity) {
    gfcm->capacity *= 2;
    Gif_ReArray(gfcm->col, Gif_Color, gfcm->capacity);
    if (gfcm->col == 0)
      return -1;
  }
  i = gfcm->ncol;
  gfcm->ncol++;
  gfcm->col[i] = *c;
  return i;
}


Gif_Image *
Gif_GetImage(Gif_Stream *gfs, int imagenumber)
{
  if (imagenumber >= 0 && imagenumber < gfs->nimages)
    return gfs->images[imagenumber];
  else
    return 0;
}


Gif_Image *
Gif_GetNamedImage(Gif_Stream *gfs, const char *name)
{
  int i;

  if (!name)
    return gfs->nimages ? gfs->images[0] : 0;

  for (i = 0; i < gfs->nimages; i++)
    if (gfs->images[i]->identifier &&
        strcmp(gfs->images[i]->identifier, name) == 0)
      return gfs->images[i];

  return 0;
}


void
Gif_ReleaseCompressedImage(Gif_Image *gfi)
{
  if (gfi->compressed && gfi->free_compressed)
    (*gfi->free_compressed)(gfi->compressed);
  gfi->compressed = 0;
  gfi->compressed_len = 0;
  gfi->free_compressed = 0;
}

void
Gif_ReleaseUncompressedImage(Gif_Image *gfi)
{
  Gif_DeleteArray(gfi->img);
  if (gfi->image_data && gfi->free_image_data)
    (*gfi->free_image_data)(gfi->image_data);
  gfi->img = 0;
  gfi->image_data = 0;
  gfi->free_image_data = 0;
}


int
Gif_ClipImage(Gif_Image *gfi, int left, int top, int width, int height)
{
  int new_width = gfi->width, new_height = gfi->height;
  int y;

  if (!gfi->img)
    return 0;

  if (gfi->left < left) {
    int shift = left - gfi->left;
    for (y = 0; y < gfi->height; y++)
      gfi->img[y] += shift;
    gfi->left += shift;
    new_width -= shift;
  }

  if (gfi->top < top) {
    int shift = top - gfi->top;
    for (y = gfi->height - 1; y >= shift; y++)
      gfi->img[y - shift] = gfi->img[y];
    gfi->top += shift;
    new_height -= shift;
  }

  if (gfi->left + new_width >= width)
    new_width = width - gfi->left;

  if (gfi->top + new_height >= height)
    new_height = height - gfi->top;

  if (new_width < 0)
    new_width = 0;
  if (new_height < 0)
    new_height = 0;
  gfi->width = new_width;
  gfi->height = new_height;
  return 1;
}


int
Gif_InterlaceLine(int line, int height)
{
  height--;
  if (line > height / 2)
    return line * 2 - ( height       | 1);
  else if (line > height / 4)
    return line * 4 - ((height & ~1) | 2);
  else if (line > height / 8)
    return line * 8 - ((height & ~3) | 4);
  else
    return line * 8;
}


int
Gif_SetUncompressedImage(Gif_Image *gfi, uint8_t *image_data,
                         void (*free_data)(void *), int data_interlaced)
{
  /* NB does not affect compressed image (and must not) */
  unsigned i;
  unsigned width = gfi->width;
  unsigned height = gfi->height;
  uint8_t **img;

  Gif_ReleaseUncompressedImage(gfi);
  if (!image_data)
    return 0;

  img = Gif_NewArray(uint8_t *, height + 1);
  if (!img)
    return 0;

  if (data_interlaced)
    for (i = 0; i < height; i++)
      img[ Gif_InterlaceLine(i, height) ] = image_data + width * i;
  else
    for (i = 0; i < height; i++)
      img[i] = image_data + width * i;
  img[height] = 0;

  gfi->img = img;
  gfi->image_data = image_data;
  gfi->free_image_data = free_data;
  return 1;
}

int
Gif_CreateUncompressedImage(Gif_Image *gfi, int data_interlaced)
{
    size_t sz = (size_t) gfi->width * (size_t) gfi->height;
    uint8_t *data = Gif_NewArray(uint8_t, sz ? sz : 1);
    return Gif_SetUncompressedImage(gfi, data, Gif_Free, data_interlaced);
}

#if !GIF_ALLOCATOR_DEFINED
void* Gif_Realloc(void* p, size_t s, size_t n, const char* file, int line) {
    (void) file, (void) line;
    if (s == 0 || n == 0)
        Gif_Free(p);
    else if (s == 1 || n == 1 || s <= ((size_t) -1) / n)
        return realloc(p, s * n);
    return (void*) 0;
}

#undef Gif_Free
void Gif_Free(void* p) {
    free(p);
}
#endif

#ifdef __cplusplus
}
#endif

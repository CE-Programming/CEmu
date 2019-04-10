#ifndef DEFINES_H
#define DEFINES_H

#ifdef __EMSCRIPTEN__
 #include <emscripten.h>
#else
 #define EMSCRIPTEN_KEEPALIVE
#endif

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

/* MSVC doesn't support __builtin_expect or __attribute__, stub them out */
#ifndef _MSC_VER
 #define likely(x) __builtin_expect(!!(x), 1)
#else
 #define likely(x) (x)
 #define __attribute__(x)
#endif

#define unlikely(x) !likely(!(x))

#endif

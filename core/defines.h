#ifndef DEFINES_H
#define DEFINES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

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

/* Cross-compiler packed wrapper */
#ifdef _MSC_VER
#  define PACK(...) __pragma(pack(push, 1)) __VA_ARGS__ __pragma(pack(pop))
#elif defined(__GNUC__)
#  define PACK(...) __VA_ARGS__ __attribute__((packed))
#endif

/* Cross-compiler alignment */
#if defined(_MSC_VER)
#  define ALIGNED_(x) __declspec(align(x))
#elif defined(__GNUC__)
#  define ALIGNED_(x) __attribute__ ((aligned(x)))
#endif

#endif

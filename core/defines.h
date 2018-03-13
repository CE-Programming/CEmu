#ifndef DEFINES_H
#define DEFINES_H

#ifdef __EMSCRIPTEN__
 #include <emscripten.h>
#else
 #define EMSCRIPTEN_KEEPALIVE
#endif

/* hacky atomics */
#if defined (MULTITHREAD)
#ifndef __cplusplus
#if !defined(__STDC_NO_ATOMICS__)
 #include <stdatomic.h>
#else
 #define _Atomic(X) volatile X /* doesn't do anything, but makes me feel better... although if you are trying to do multithreading glhf */
#endif
#else
 #include <atomic>
 #define _Atomic(X) std::atomic<X>
#endif
#else
 #define _Atomic(X) X
#endif

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

/* MSVC doesn't support __builtin_expect, stub it out */
#ifndef _MSC_VER
 #define   likely(x) __builtin_expect(!!(x), 1)
#else
 #define   likely(x) (x)
#endif

#define unlikely(x) !likely(!(x))

#endif

#ifndef DEFINES_H
#define DEFINES_H

#ifndef __has_attribute
# define __has_attribute(x) 0
#endif
#ifndef __has_builtin
# define __has_builtin(x) 0
#endif

#ifndef __cplusplus
# if __has_attribute(fallthrough)
#  define fallthrough __attribute__((fallthrough))
# else
#  define fallthrough (void)0
# endif
#endif

#if __has_builtin(__builtin_expect)
# define unlikely(x) __builtin_expect(x, 0)
#else
# define unlikely(x) (x)
#endif
#define likely(x) !unlikely(!(x))

#if __has_builtin(__builtin_unreachable)
# define unreachable __builtin_unreachable
#else
# define unreachable abort
#endif

#ifdef _MSC_VER
# define strcasecmp _stricmp
#endif

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#else
# define EMSCRIPTEN_KEEPALIVE
#endif

#define PASTE(x, y) x ## y
#define CONCAT(x, y) PASTE(x, y)

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

#endif

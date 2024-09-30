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

#if __has_builtin(__builtin_expect) || (__GNUC__ >= 3)
# define unlikely(x) __builtin_expect(x, 0)
#else
# define unlikely(x) (x)
#endif
#define likely(x) !unlikely(!(x))

#include <stddef.h>
#ifndef unreachable
# if __has_builtin(__builtin_unreachable) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 5)
#  define unreachable() __builtin_unreachable()
# elif defined(_MSC_VER)
#  define unreachable() __assume(0)
# else
#  include <stdlib.h>
#  define unreachable() abort()
# endif
#endif

#ifdef _MSC_VER
# define strcasecmp _stricmp
#endif

#ifdef _MSC_VER
# define CEMU_TYPEOF __typeof__
#else
# define CEMU_TYPEOF typeof
#endif

#ifdef __EMSCRIPTEN__
# include <emscripten.h>
#else
# define EMSCRIPTEN_KEEPALIVE
#endif

#define CEMU_LITTLE_ENDIAN 1
#define CEMU_BIG_ENDIAN 2
#ifndef CEMU_BYTE_ORDER
# if defined(__BYTE_ORDER__)
#  if __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
#   define CEMU_BYTE_ORDER CEMU_LITTLE_ENDIAN
#  elif __BYTE_ORDER__ == __ORDER_BIG_ENDIAN__
#   define CEMU_BYTE_ORDER CEMU_BIG_ENDIAN
#  endif
# elif defined(_MSC_VER)
#  define CEMU_BYTE_ORDER CEMU_LITTLE_ENDIAN
# endif
#endif
#if CEMU_BYTE_ORDER != CEMU_LITTLE_ENDIAN && CEMU_BYTE_ORDER != CEMU_BIG_ENDIAN
# error Endianness not automatically determined, please define CEMU_BYTE_ORDER as CEMU_LITTLE_ENDIAN or CEMU_BIG_ENDIAN.
#endif
#ifndef CEMU_BITFIELD_ORDER
# define CEMU_BITFIELD_ORDER CEMU_BYTE_ORDER
#endif

#if CEMU_BYTE_ORDER == CEMU_LITTLE_ENDIAN
# define from_le16(w) (w)
# define from_le32(w) (w)
#else
# if __has_builtin(__builtin_bswap16) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 8)
#  define from_le16 __builtin_bswap16
# elif defined(_MSVC)
#  define from_le16 _byteswap_ushort
# else
static inline uint16_t from_le16(uint16_t w) {
    return ((w & UINT16_C(0xFF00) >> 8) | ((w & UINT16_C(0x00FF) << 8);
}
# endif
# if __has_builtin(__builtin_bswap32) || (__GNUC__ > 4) || (__GNUC__ == 4 && __GNUC_MINOR__ >= 3)
#  define from_le32 __builtin_bswap32
# elif defined(_MSVC)
#  define from_le32 _byteswap_ulong
# else
static inline uint32_t from_le32(uint32_t w) {
    return ((w & UINT32_C(0xFF000000) >> 24) | ((w & UINT32_C(0x00FF0000) >> 8) |
           ((w & UINT32_C(0x0000FF00) << 8) | ((w & UINT32_C(0x000000FF) << 24);
}
# endif
#endif
#define to_le16 from_le16
#define to_le32 from_le32

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

#endif

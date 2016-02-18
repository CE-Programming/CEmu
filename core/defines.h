#ifndef DEFINES_H
#define DEFINES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

#define mmio_range(addr) ((((addr<0xF00000) ? ((addr-0xDF0000)>>16) : ((addr-0xEB0000)>>16)))&0xF)
#define port_range(a) (((a)>>12)&0xF) /* converts an address to a port range 0x0-0xF */
#define addr_range(a) ((a)&0xFFF)     /* converts an address to a port range value 0x000-0xFFF */

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

#ifndef DEFINES_H
#define DEFINES_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>

#if (defined(_MSC_VER) && _MSC_VER < 1900)
    #define CEMU_CONSTEXPR
#else
    #define CEMU_CONSTEXPR constexpr
#endif

#ifdef _MSC_VER
    #define CEMU_INLINE __inline
#else
    #define CEMU_INLINE inline
#endif

#define GETMASK(index, size) (((1U << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((uint32_t)(value) << (index)))

#define write8(data, index, value) WRITE(data, index, 8, value)
#define read8(data, index) READFROM(data, index, 8)

#define mmio_range(addr) (((addr<0xF00000) ? ((addr-0xDF0000)>>16) : ((addr-0xEB0000)>>16))&0xF)
#define port_range(a) (((a)>>12)&0xF) // converts an address to a port range 0x0-0xF
#define addr_range(a) ((a)&0xFFF)     // converts an address to a port range value 0x000-0xFFF
#define rswap(a, b) do { (a) ^= (b); (b) ^= (a); (a) ^= (b); } while(0)

#endif // DEFINES

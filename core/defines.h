#ifndef DEFINES
#define DEFINES

#define GETMASK(index, size) (((1 << (size)) - 1) << (index))
#define READFROM(data, index, size) (((data) & GETMASK((index), (size))) >> (index))
#define WRITE(data, index, size, value) ((data) = ((data) & (~GETMASK((index), (size)))) | ((value) << (index)))

#define write8(data, index, value) ((data) = ((data) & (~GETMASK((index), (8)))) | ((value) << (index)))
#define read8(data, index) (((data) & GETMASK((index), (8))) >> (index))

#define upperNibble8(data) ((data >> 4) & 0xF)
#define upperNibble16(data) ((data >> 12) & 0xF)
#define upperNibble24(data) ((data >> 20) & 0xF)
#define upperNibble32(data) ((data >> 28) & 0xF)

#define upper16(data) ((data >> 8) & 0xFF)
#define upper24(data) ((data >> 16) & 0xFF)
#define upper32(data) ((data >> 24) & 0xFF)

#define mask8(data)  ((data & 0xFF))
#define mask16(data) ((data & 0xFFFF))
#define mask24(data) ((data & 0xFFFFFF))

#define inc24(data)  ((data+1)&0xFFFFFF)
#define dec24(data)  ((data-1)&0xFFFFFF)
#endif // DEFINES


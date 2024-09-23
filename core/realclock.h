#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef enum rtc_mode {
    RTC_TICK,
    RTC_LATCH,
    RTC_LOAD_LATCH
} rtc_mode_t;

typedef union rtc_datetime {
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint64_t day : 16, hour : 8, min : 8, sec : 8, pad : 24;
#else
        uint64_t pad : 24, sec : 8, min : 8, hour : 8, day : 16;
#endif
    };
    uint64_t value;
} rtc_datetime_t;

typedef union rtc_time {
    struct {
#if CEMU_BITFIELD_ORDER == CEMU_LITTLE_ENDIAN
        uint32_t hour : 8, min : 8, sec : 8, pad : 8;
#else
        uint32_t pad : 8, sec : 8, min : 8, hour : 8;
#endif
    };
    uint32_t value;
} rtc_time_t;

typedef struct rtc_state {
    rtc_mode_t mode;
    uint8_t control, interrupt, loadTicksProcessed;

    rtc_datetime_t counter;
    rtc_datetime_t latched;
    rtc_datetime_t load;
    rtc_time_t alarm;
} rtc_state_t;

extern rtc_state_t rtc;

eZ80portrange_t init_rtc(void);
void rtc_reset(void);
bool rtc_restore(FILE *image);
bool rtc_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif

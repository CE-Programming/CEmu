#ifndef RTC_H
#define RTC_H

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

typedef struct rtc_state {
    uint8_t control, interrupt;
    uint8_t readSec,
            readMin,
            readHour;
    uint8_t alarmSec,
            alarmMin,
            alarmHour;
    uint8_t writeSec,
            writeMin,
            writeHour;
    uint8_t holdSec,
            holdMin,
            holdHour;

    uint16_t readDay,
             writeDay,
             holdDay;

    uint32_t revision;
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

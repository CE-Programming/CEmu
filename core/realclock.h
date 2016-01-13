#ifndef H_RTC
#define H_RTC

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "apb.h"

typedef struct rtc_state {
    /* Previos second counter */
    time_t prevsec;

    /* Registers */
    uint8_t read_sec;
    uint8_t read_min;
    uint8_t read_hour;
    uint16_t read_day;
    uint8_t alarm_sec;
    uint8_t alarm_min;
    uint8_t alarm_hour;
    uint8_t control;
    uint8_t write_sec;
    uint8_t write_min;
    uint8_t write_hour;
    uint16_t write_day;
    uint8_t interrupt;
    uint8_t hold_sec;
    uint8_t hold_min;
    uint8_t hold_hour;
    uint8_t hold_day;
    uint32_t revision;
} rtc_state_t;

/* Global GPT state */
extern rtc_state_t rtc;

/* Available Functions */
eZ80portrange_t init_rtc(void);
void rtc_reset(void);

#ifdef __cplusplus
}
#endif

#endif

#ifndef _H_RTC
#define _H_RTC

#include <core/cpu.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtc_state {
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
};

/* Type Definitions */
typedef struct rtc_state rtc_state_t;

/* Global GPT state */
extern rtc_state_t rtc;

/* Available Functions */
eZ80portrange_t init_rtc(void);
void rtc_reset(void);

#ifdef __cplusplus
}
#endif

#endif

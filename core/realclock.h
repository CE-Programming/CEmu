#ifndef _H_RTC
#define _H_RTC

#include <core/cpu.h>
#include <time.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtc_state {
    time_t rawtime;
    struct tm *timeinfo;
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

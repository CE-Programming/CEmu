#ifndef _H_RTC
#define _H_RTC

#include <core/cpu.h>

#ifdef __cplusplus
extern "C" {
#endif

struct rtc_state {
    uint32_t dummy;     // Remove when implemented
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

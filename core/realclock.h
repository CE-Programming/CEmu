#ifndef H_RTC
#define H_RTC

#ifdef __cplusplus
extern "C" {
#endif

#include "port.h"

#include <time.h>

PACK(typedef struct rtc_state {
    /* Registers */
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
}) rtc_state_t;

/* Global GPT state */
extern rtc_state_t rtc;

/* Available Functions */
eZ80portrange_t init_rtc(void);
void rtc_reset(void);

/* Save/Restore */
typedef struct emu_image emu_image;
bool rtc_restore(const emu_image*);
bool rtc_save(emu_image*);

#ifdef __cplusplus
}
#endif

#endif

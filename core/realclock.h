#ifndef H_RTC
#define H_RTC

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include "apb.h"

typedef struct rtc_state {
    /* Previos second counter */
    time_t prevSec;

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
} __attribute__((packed)) rtc_state_t;

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

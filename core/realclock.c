#include "realclock.h"
#include "emu.h"
#include "schedule.h"
#include "interrupt.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>

/* Global GPT state */
rtc_state_t rtc;

#define RTC_TIME_BITS (8 * 3)
#define RTC_DATETIME_BITS (RTC_TIME_BITS + 16)
#define RTC_DATETIME_MASK ((UINT64_C(1) << RTC_DATETIME_BITS) - 1)

#define TICKS_PER_SECOND 32768
#define LATCH_TICK_OFFSET 16429
#define LOAD_LATCH_TICK_OFFSET (LATCH_TICK_OFFSET + 7)

/* Load status gets set 1 tick after each load completes */
#define LOAD_SEC_FINISHED (1 + 8)
#define LOAD_MIN_FINISHED (LOAD_SEC_FINISHED + 8)
#define LOAD_HOUR_FINISHED (LOAD_MIN_FINISHED + 8)
#define LOAD_DAY_FINISHED (LOAD_HOUR_FINISHED + 16)
#define LOAD_TOTAL_TICKS (LOAD_DAY_FINISHED + 10)
#define LOAD_PENDING UINT8_MAX

static void rtc_process_load(uint8_t endTick) {
    assert(endTick <= LOAD_TOTAL_TICKS);
    uint8_t startTick = rtc.loadTicksProcessed;
    assert(startTick != LOAD_PENDING);
    if (endTick <= startTick) {
        assert(endTick == startTick);
        return;
    }
    rtc.loadTicksProcessed = endTick;
    if (startTick >= RTC_DATETIME_BITS) {
        return;
    }
    if (endTick >= RTC_DATETIME_BITS) {
        rtc.control &= ~64;  /* Load bit is cleared after all bits are loaded */
        endTick = RTC_DATETIME_BITS;
    }
    /* Load is processed 1 bit at a time in each register from most to least significant */
    uint64_t writeMask = (RTC_DATETIME_MASK >> startTick) & ~(RTC_DATETIME_MASK >> endTick);
    rtc.counter.value = (rtc.counter.value & ~writeMask) | (rtc.load.value & writeMask);
}

static void rtc_update_load(void) {
    if (likely(rtc.loadTicksProcessed >= LOAD_TOTAL_TICKS)) {
        return;
    }
    /* Get the number of ticks passed since the latch event */
    uint16_t ticks = sched_ticks_remaining(SCHED_RTC);
    if (rtc.mode == RTC_TICK) {
        ticks = (TICKS_PER_SECOND - LATCH_TICK_OFFSET) - ticks;
    } else {
        assert(rtc.mode == RTC_LOAD_LATCH);
        ticks = (LOAD_LATCH_TICK_OFFSET - LATCH_TICK_OFFSET) - ticks;
    }
    /* Add 1 because the end of the tick range is exclusive */
    rtc_process_load(ticks < LOAD_TOTAL_TICKS ? ticks + 1 : LOAD_TOTAL_TICKS);
}

static void rtc_event(enum sched_item_id id) {
    uint8_t control = rtc.control, interrupts;

    switch (rtc.mode) {
        case RTC_TICK:
            /* Process any remaining load operations */
            if (rtc.loadTicksProcessed < LOAD_TOTAL_TICKS) {
                rtc_process_load(LOAD_TOTAL_TICKS);
            }

            /* Next event is latch */
            rtc.mode = RTC_LATCH;
            sched_repeat(id, LATCH_TICK_OFFSET);

            if (!likely(control & 1)) { /* (Bit 0) -- Enable ticking */
                break;
            }

            interrupts = 1;
            if (unlikely(++rtc.counter.sec >= 60)) {
                if (likely(rtc.counter.sec == 60)) {
                    interrupts |= 2;
                    if (unlikely(++rtc.counter.min >= 60)) {
                        if (likely(rtc.counter.min == 60)) {
                            interrupts |= 4;
                            if (unlikely(++rtc.counter.hour >= 24)) {
                                if (likely(rtc.counter.hour == 24)) {
                                    interrupts |= 8;
                                    rtc.counter.day++;
                                }
                                rtc.counter.hour = 0;
                            }
                        }
                        rtc.counter.min = 0;
                    }
                }
                rtc.counter.sec = 0;
            }

            if ((uint32_t)(rtc.counter.value >> (RTC_DATETIME_BITS - RTC_TIME_BITS)) == rtc.alarm.value) {
                interrupts |= 16;
            }
            if (interrupts &= control >> 1) {
                if (!rtc.interrupt) {
                    intrpt_set(INT_RTC, true);
                }
                rtc.interrupt |= interrupts;
            }
            break;

        case RTC_LATCH:
            if (likely(control & 128)) {  /* (Bit 7) -- Latch enable */
                rtc.latched = rtc.counter;
            }
            if (unlikely(control & 64)) { /* (Bit 6) -- Load operation */
                /* Enable load processing */
                assert(rtc.loadTicksProcessed == LOAD_PENDING);
                rtc.loadTicksProcessed = 0;
                /* Next event is load latch */
                rtc.mode = RTC_LOAD_LATCH;
                sched_repeat(id, LOAD_LATCH_TICK_OFFSET - LATCH_TICK_OFFSET);
            } else {
                /* Next event is tick */
                rtc.mode = RTC_TICK;
                sched_repeat(id, TICKS_PER_SECOND - LATCH_TICK_OFFSET);
            }
            break;

        case RTC_LOAD_LATCH:
            /* Always latches regardless of control register */
            rtc.latched = rtc.load;
            /* Load latch complete interrupt */
            rtc.interrupt |= 32;
            intrpt_set(INT_RTC, true);
            /* Next event is tick */
            rtc.mode = RTC_TICK;
            sched_repeat(id, TICKS_PER_SECOND - LOAD_LATCH_TICK_OFFSET);
            break;

        default:
            unreachable();
    }
}

static uint8_t rtc_read(const uint16_t pio, bool peek) {
    uint8_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;

    uint8_t value = 0;
    (void)peek;

    switch (index) {
        case 0x00:
            value = rtc.latched.sec;
            break;
        case 0x04:
            value = rtc.latched.min;
            break;
        case 0x08:
            value = rtc.latched.hour;
            break;
        case 0x0C: case 0x0D:
            value = read8(rtc.latched.day, bit_offset);
            break;
        case 0x10:
            value = rtc.alarm.sec;
            break;
        case 0x14:
            value = rtc.alarm.min;
            break;
        case 0x18:
            value = rtc.alarm.hour;
            break;
        case 0x20:
            rtc_update_load();
            value = rtc.control;
            break;
        case 0x24:
            value = rtc.load.sec;
            break;
        case 0x28:
            value = rtc.load.min;
            break;
        case 0x2C:
            value = rtc.load.hour;
            break;
        case 0x30: case 0x31:
            value = read8(rtc.load.day, bit_offset);
            break;
        case 0x34:
            value = rtc.interrupt;
            break;
        case 0x3C: case 0x3D: case 0x3E: case 0x3F:
            value = read8(0x00010500, bit_offset);
            break;
        case 0x40: {
            rtc_update_load();
            /* Convert to signed to treat LOAD_PENDING as negative */
            int8_t ticks = rtc.loadTicksProcessed;
            if (likely(ticks >= LOAD_TOTAL_TICKS)) {
                value = 0;
            } else {
                value = 8 | ((ticks < LOAD_SEC_FINISHED)  << 4)
                          | ((ticks < LOAD_MIN_FINISHED)  << 5)
                          | ((ticks < LOAD_HOUR_FINISHED) << 6)
                          | ((ticks < LOAD_DAY_FINISHED)  << 7);
            }
            break;
        }
        case 0x44: case 0x45: case 0x46: case 0x47: {
            uint32_t combined = rtc.latched.sec | (rtc.latched.min << 6) | (rtc.latched.hour << 12) | (rtc.latched.day << 17);
            value = read8(combined, bit_offset);
            break;
        }
        default:
            break;
    }

    return value;
}

static void rtc_write(const uint16_t pio, const uint8_t byte, bool poke) {
    uint16_t index = pio & 0xFF;
    uint8_t bit_offset = (index & 3) << 3;
    (void)poke;

    switch (index) {
        case 0x10:
            rtc.alarm.sec = byte & 63;
            break;
        case 0x14:
            rtc.alarm.min = byte & 63;
            break;
        case 0x18:
            rtc.alarm.hour = byte & 31;
            break;
        case 0x20:
            if (byte & 64) {
                rtc_update_load();
                if (!(rtc.control & 64)) {
                    /* Load can be pended as soon as the previous one is finished */
                    assert(rtc.loadTicksProcessed >= RTC_DATETIME_BITS);
                    rtc.loadTicksProcessed = LOAD_PENDING;
                }
                rtc.control = byte;
            } else {
                /* Don't allow resetting the load bit via write */
                rtc.control = byte | (rtc.control & 64);
            }
            break;
        case 0x24:
            rtc_update_load();
            rtc.load.sec = byte & 63;
            break;
        case 0x28:
            rtc_update_load();
            rtc.load.min = byte & 63;
            break;
        case 0x2C:
            rtc_update_load();
            rtc.load.hour = byte & 31;
            break;
        case 0x30: case 0x31:
            rtc_update_load();
            write8(rtc.load.day, bit_offset, byte);
            break;
        case 0x34:
            intrpt_set(INT_RTC, rtc.interrupt &= ~byte);
            break;
        default:
            break;
    }
}

static void rtc_init_events(void) {
    sched_init_event(SCHED_RTC, CLOCK_32K, rtc_event);
}

void rtc_reset(void) {
    memset(&rtc, 0, sizeof rtc);
    rtc.mode = RTC_LATCH;
    rtc.loadTicksProcessed = LOAD_TOTAL_TICKS;

    rtc_init_events();
    sched_repeat_relative(SCHED_RTC, SCHED_SECOND, 0, LATCH_TICK_OFFSET);

    gui_console_printf("[CEmu] RTC reset.\n");
}

static const eZ80portrange_t device = {
    .read  = rtc_read,
    .write = rtc_write
};

eZ80portrange_t init_rtc(void) {
    gui_console_printf("[CEmu] Initialized Real Time Clock...\n");
    return device;
}

bool rtc_save(FILE *image) {
    return fwrite(&rtc, sizeof(rtc), 1, image) == 1;
}

bool rtc_restore(FILE *image) {
    rtc_init_events();
    return fread(&rtc, sizeof(rtc), 1, image) == 1;
}

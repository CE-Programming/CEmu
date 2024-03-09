#ifndef SCHEDULE_H
#define SCHEDULE_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

/* All clock rates must be a factor of this value, and greater than 1 */
/* If events at 1 Hz are required, schedule them relative to SCHED_SECOND */
#define SCHED_BASE_CLOCK_RATE ((uint64_t)7680000000ULL)

enum clock_id { CLOCK_CPU, CLOCK_PANEL, CLOCK_RUN, CLOCK_48M, CLOCK_24M, CLOCK_12M, CLOCK_6M,
                CLOCK_3M, CLOCK_1M, CLOCK_32K, CLOCK_NUM_ITEMS };

enum sched_item_id {
    SCHED_SECOND,

    SCHED_RUN,
    SCHED_WATCHDOG,
    SCHED_TIMER_DELAY, /* Must be before TIMER# */
    SCHED_TIMER1,
    SCHED_TIMER2,
    SCHED_TIMER3,
    SCHED_OSTIMER,
    SCHED_KEYPAD,
    SCHED_LCD,
    SCHED_RTC,
    SCHED_USB,
    SCHED_USB_DEVICE,
    SCHED_SPI,
    SCHED_UART,
    SCHED_PANEL,

    SCHED_FIRST_EVENT = SCHED_RUN,
    SCHED_LAST_EVENT = SCHED_PANEL,

    SCHED_LCD_DMA,
    SCHED_USB_DMA,

    SCHED_FIRST_DMA = SCHED_LCD_DMA,
    SCHED_LAST_DMA = SCHED_USB_DMA,

    SCHED_NO_DMA,

    SCHED_NUM_ITEMS
};

struct sched_item {
    uint64_t timestamp; /* bit 63 set if disabled */
    union sched_callback {
        void (*event)(enum sched_item_id id);
        uint32_t (*dma)(enum sched_item_id id);
    } callback;
    enum clock_id clock;
};

struct sched_clock {
    uint32_t tick_unit;
    uint32_t shift;
    uint64_t recip;
};

typedef struct sched_state {
    struct sched_item items[SCHED_NUM_ITEMS];
    struct sched_clock clocks[CLOCK_NUM_ITEMS];
    struct sched_event {
        enum sched_item_id next;
        uint32_t next_cycle;
    } event;
    struct sched_dma {
        enum sched_item_id next;
        uint64_t last_mem_timestamp;
    } dma;
    bool run_event_triggered;
} sched_state_t;

extern sched_state_t sched;

void sched_init(void);
void sched_reset(void);
void sched_run_event(enum sched_item_id id);
uint32_t sched_event_next_cycle(void);
uint32_t sched_dma_next_cycle(void);
void sched_process_pending_events(void);
void sched_process_pending_dma(uint8_t duration);
void sched_rewind_cpu(uint8_t duration);
void sched_init_event(enum sched_item_id id, enum clock_id clock, void (*event)(enum sched_item_id id));
void sched_init_dma(enum sched_item_id id, enum clock_id clock, uint32_t (*dma)(enum sched_item_id id));
void sched_set_item_clock(enum sched_item_id id, enum clock_id clock);
void sched_clear(enum sched_item_id id);
void sched_set(enum sched_item_id id, uint64_t ticks);
void sched_repeat(enum sched_item_id id, uint64_t ticks);
void sched_repeat_relative(enum sched_item_id id, enum sched_item_id base, uint32_t offset, uint64_t ticks);
bool sched_active(enum sched_item_id id);
uint64_t sched_cycle(enum sched_item_id id);
uint64_t sched_cycles_remaining(enum sched_item_id id);
uint64_t sched_tick(enum sched_item_id id);
uint64_t sched_ticks_remaining(enum sched_item_id id);
uint64_t sched_ticks_remaining_relative(enum sched_item_id id, enum sched_item_id base, uint32_t offset);
bool sched_set_clock(enum clock_id clock, uint32_t new_rate);
uint32_t sched_get_clock_rate(enum clock_id clock);
uint64_t sched_total_cycles(void);
uint64_t sched_total_time(enum clock_id clock);
uint64_t event_next_cycle(enum sched_item_id id);
bool sched_restore(FILE *image);
bool sched_save(FILE *image);

#ifdef __cplusplus
}
#endif

#endif

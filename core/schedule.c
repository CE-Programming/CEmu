/* Copyright (C) 2015  Fabian Vogt
 * Modified for the CE calculator by CEmu developers
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
*/

#include "schedule.h"
#include "cpu.h"
#include "emu.h"
#include "debug/debug.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#ifdef _MSC_VER
# include <intrin.h>
# if defined(_M_X64) || defined(_M_ARM64)
#  define HAS_MULH
# elif defined(_M_IX86)
#  define HAS_LLSHIFT
# endif
# if _MSC_VER >= 1920
#  if defined(_M_X64)
#   define HAS_DIV128
#   define HAS_DIV64
#  elif defined(_M_IX86)
#   define HAS_DIV64
#  endif
# endif
#endif

#define SCHED_INACTIVE_FLAG (UINT64_C(1) << 63)

sched_state_t sched;

static inline bool timestamp_before(uint64_t a, uint64_t b) {
#if defined(_MSC_VER) && defined(_M_IX86)
    /* Avoid branchy codegen on 32-bit MSVC */
    uint32_t temp;
    bool borrow = _subborrow_u32(0, (uint32_t)a, (uint32_t)b, &temp);
    return _subborrow_u32(borrow, (uint32_t)(a >> 32), (uint32_t)(b >> 32), &temp);
#else
    return a < b;
#endif
}

static inline uint64_t timestamp_diff(uint64_t a, uint64_t b) {
#if defined(_MSC_VER) && defined(_M_IX86)
    /* Avoid branchy codegen on 32-bit MSVC */
    union {
        struct {
            uint32_t low;
            uint32_t high;
        };
        uint64_t value;
    } u;
    bool borrow = _subborrow_u32(0, (uint32_t)a, (uint32_t)b, &u.low);
    if (_subborrow_u32(borrow, (uint32_t)(a >> 32), (uint32_t)(b >> 32), &u.high)) {
        u.low = u.high = 0;
    }
    return u.value;
#else
    return a < b ? 0 : a - b;
#endif
}

static inline uint32_t floor_log2(uint32_t value) {
    if (value == 0) {
        unreachable();
    }
    uint32_t count;
#if __has_builtin(__builtin_clz) || (__GNUC__ > 3) || (__GNUC__ == 3 && __GNUC_MINOR__ >= 4)
    count = 31 - __builtin_clz(value);
#elif defined(_MSC_VER)
    _BitScanReverse(&count, value);
#else
    count = 0;
    if (value > 0xFFFF) {
        value >>= 16;
        count += 16;
    }
    if (value > 0xFF) {
        value >>= 8;
        count += 8;
    }
    if (value > 0xF) {
        value >>= 4;
        count += 4;
    }
    count += 0xFFFFAA50U >> (value * 2) & 3;
#endif
    return count;
}

static void update_reciprocal(struct sched_clock *clock) {
    uint32_t divisor = clock->tick_unit;
    assert(divisor > 1);
    uint32_t shift = floor_log2(divisor);
    if (divisor == (uint32_t)1 << shift) {
        clock->recip = (uint64_t)1 << 63;
        clock->shift = shift - 1;
        return;
    }

    uint64_t dividend = ((uint32_t)1 << shift) - 1;
#if defined(__SIZEOF_INT128__)
    clock->recip = (((__uint128_t)dividend << 64) | (uint64_t)-1) / divisor;
#elif defined(HAS_DIV128)
    uint64_t remainder;
    clock->recip = _udiv128(dividend, (uint64_t)-1, divisor, &remainder);
#else
    dividend = (dividend << 32) | (uint32_t)-1;
    uint32_t quotient, remainder;
# if defined(HAS_DIV64)
    quotient = _udiv64(dividend, divisor, &remainder);
# else
    quotient = dividend / divisor;
    remainder = dividend % divisor;
# endif

    dividend = ((uint64_t)remainder << 32) | (uint32_t)-1;
# if defined(HAS_DIV64)
    clock->recip = ((uint64_t)quotient << 32) | _udiv64(dividend, divisor, &remainder);
# else
    clock->recip = ((uint64_t)quotient << 32) | (uint32_t)(dividend / divisor);
# endif
#endif
    clock->recip++;
    clock->shift = shift;
}

static inline uint64_t div_floor(uint64_t dividend, struct sched_clock *clock) {
    assert(dividend < SCHED_INACTIVE_FLAG);
    uint64_t high, quotient;
#if defined(__SIZEOF_INT128__)
    high = ((__uint128_t)dividend * clock->recip) >> 64;
    quotient = high >> clock->shift;
#elif defined(HAS_MULH)
    high = __umulh(dividend, clock->recip);
    quotient = high >> clock->shift;
#else
    uint32_t a1 = dividend >> 32, a0 = dividend;
    uint32_t b1 = clock->recip >> 32, b0 = clock->recip;
    uint64_t mid = (uint64_t)a1 * b0;
    high = (uint64_t)a1 * b1 + (uint32_t)(mid >> 32);
    mid = (uint32_t)mid + (uint64_t)a0 * b1 + (uint32_t)(((uint64_t)a0 * b0) >> 32);
    high += (uint32_t)(mid >> 32);
# if defined(HAS_LLSHIFT)
    quotient = __ull_rshift(high, clock->shift);
# else
    if (clock->shift > 31) {
        unreachable();
    }
    quotient = high >> clock->shift;
# endif
#endif

    assert(quotient == dividend / clock->tick_unit);
    return quotient;
}

static inline uint64_t div_round(uint64_t dividend, struct sched_clock *clock) {
    return div_floor(dividend + (clock->tick_unit / 2), clock);
}

static inline uint64_t div_ceil(uint64_t dividend, struct sched_clock *clock) {
    return div_floor(dividend + (clock->tick_unit - 1), clock);
}

static uint32_t muldiv_floor(uint32_t value, struct sched_clock *numer, struct sched_clock *denom) {
    if (likely(numer->tick_unit == denom->tick_unit)) {
        return value;
    } else {
        return (uint32_t)div_floor((uint64_t)value * numer->tick_unit, denom);
    }
}

#if 0
static uint32_t muldiv_ceil(uint32_t value, struct sched_clock *numer, struct sched_clock *denom) {
    if (likely(numer->tick_unit == denom->tick_unit)) {
        return value;
    } else {
        return (uint32_t)div_ceil((uint64_t)value * numer->tick_unit, denom);
    }
}
#endif

void sched_run_event(enum sched_item_id id) {
    (void)id;
    sched.run_event_triggered = true;
}

static inline uint64_t sched_cpu_timestamp(void) {
    return (uint64_t)cpu.cycles * sched.clocks[CLOCK_CPU].tick_unit;
}

uint32_t sched_event_next_cycle(void) {
    return sched.event.next_cycle;
}

static void sched_update_next(enum sched_item_id id) {
    sched.event.next = id;
    sched.event.next_cycle = (uint32_t)sched_cycle(id);
    cpu_restore_next();
}

static void sched_update_events(void) {
    enum sched_item_id next_id = SCHED_SECOND;
    for (enum sched_item_id id = SCHED_FIRST_EVENT; id <= SCHED_LAST_EVENT; id++) {
        struct sched_item *item = &sched.items[id];
        /* Implicitly ignore inactive events */
        if (timestamp_before(item->timestamp, sched.items[next_id].timestamp)) {
            assert(item->callback.event != NULL);
            next_id = id;
        }
    }
    sched_update_next(next_id);
}

static void sched_update_dmas(void) {
    enum sched_item_id next_id = SCHED_NO_DMA;
    for (enum sched_item_id id = SCHED_FIRST_DMA; id <= SCHED_LAST_DMA; id++) {
        struct sched_item *item = &sched.items[id];
        /* Implicitly ignore inactive events */
        if (timestamp_before(item->timestamp, sched.items[next_id].timestamp)) {
            assert(item->callback.dma != NULL);
            next_id = id;
        }
    }
    sched.dma.next = next_id;
}

static inline void sched_update_item(enum sched_item_id id) {
    assert(sched_active(id));
    struct sched_item *item = &sched.items[id];
    if (id <= SCHED_LAST_EVENT) {
        assert(id >= SCHED_FIRST_EVENT);
        assert(item->callback.event != NULL);
        if (id == sched.event.next) {
            sched_update_events();
        } else if (timestamp_before(item->timestamp, sched.items[sched.event.next].timestamp)) {
            sched_update_next(id);
        }
    } else {
        assert(id >= SCHED_FIRST_DMA && id <= SCHED_LAST_DMA);
        assert(item->callback.dma != NULL);
        if (id == sched.dma.next) {
            sched_update_dmas();
        } else if (timestamp_before(item->timestamp, sched.items[sched.dma.next].timestamp)) {
            sched.dma.next = id;
        }
    }
}

static void sched_schedule(enum sched_item_id id, uint64_t timestamp) {
    struct sched_item *item = &sched.items[id];
    item->timestamp = timestamp;
    sched_update_item(id);
}

void sched_repeat_relative(enum sched_item_id id, enum sched_item_id base, uint32_t offset, uint64_t ticks) {
    struct sched_item *item = &sched.items[id];
    struct sched_item *base_item = &sched.items[base];
    uint32_t tick_unit = sched.clocks[item->clock].tick_unit;
    uint32_t base_tick_unit = sched.clocks[base_item->clock].tick_unit;
    uint64_t base_timestamp = (base_item->timestamp & ~SCHED_INACTIVE_FLAG) + (uint64_t)offset * base_tick_unit;
    if (unlikely(tick_unit != base_tick_unit)) {
        ticks += div_ceil(base_timestamp, &sched.clocks[item->clock]);
        base_timestamp = 0;
    }
    sched_schedule(id, base_timestamp + ticks * tick_unit);
}

void sched_repeat(enum sched_item_id id, uint64_t ticks) {
    struct sched_item *item = &sched.items[id];
    uint32_t tick_unit = sched.clocks[item->clock].tick_unit;
    sched_schedule(id, (item->timestamp & ~SCHED_INACTIVE_FLAG) + ticks * tick_unit);
}

void sched_set(enum sched_item_id id, uint64_t ticks) {
    struct sched_clock *clock = &sched.clocks[sched.items[id].clock];
    ticks += muldiv_floor(cpu.cycles, &sched.clocks[CLOCK_CPU], clock);
    sched_schedule(id, ticks * clock->tick_unit);
}

void sched_clear(enum sched_item_id id) {
    assert(id != SCHED_SECOND && id != SCHED_NO_DMA);
    struct sched_item *item = &sched.items[id];
    if (!(item->timestamp & SCHED_INACTIVE_FLAG)) {
        item->timestamp |= SCHED_INACTIVE_FLAG;
        if (id == sched.event.next) {
            sched_update_events();
        } else if (id == sched.dma.next) {
            sched_update_dmas();
        }
    }
}

bool sched_active(enum sched_item_id id) {
    return !(sched.items[id].timestamp & SCHED_INACTIVE_FLAG);
}

uint64_t sched_cycle(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(sched_active(id));
    return div_ceil(item->timestamp, &sched.clocks[CLOCK_CPU]);
}

uint64_t sched_cycles_remaining(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(sched_active(id));
    return div_ceil(timestamp_diff(item->timestamp, sched_cpu_timestamp()), &sched.clocks[CLOCK_CPU]);
}

uint64_t sched_tick(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(sched_active(id));
    /* Timestamp is always a multiple of the tick unit, so floor division is sufficient */
    return div_floor(item->timestamp, &sched.clocks[item->clock]);
}

uint64_t sched_ticks_remaining(enum sched_item_id id) {
    struct sched_item *item = &sched.items[id];
    assert(sched_active(id));
    return div_ceil(timestamp_diff(item->timestamp, sched_cpu_timestamp()), &sched.clocks[item->clock]);
}

uint64_t sched_ticks_remaining_relative(enum sched_item_id id, enum sched_item_id base, uint32_t offset) {
    struct sched_item *item = &sched.items[id];
    struct sched_item *base_item = &sched.items[base];
    assert(sched_active(id));
    uint64_t base_timestamp = (base_item->timestamp & ~SCHED_INACTIVE_FLAG) + (uint64_t)offset * sched.clocks[base_item->clock].tick_unit;
    return div_ceil(timestamp_diff(item->timestamp, base_timestamp), &sched.clocks[item->clock]);
}

void sched_process_pending_events(void) {
    while (sched.event.next_cycle <= cpu.cycles) {
        enum sched_item_id id = sched.event.next;
        struct sched_item *item = &sched.items[id];
        assert(sched_active(id));
        /* Never disable the SCHED_SECOND event, its event handler will handle rescheduling */
        if (likely(id != SCHED_SECOND)) {
            /* Optimized schedule clear since this is currently the next event */
            item->timestamp |= SCHED_INACTIVE_FLAG;
            sched_update_events();
        }
        item->callback.event(id);
    }
}

void sched_process_pending_dma(uint8_t duration) {
    uint64_t cpu_timestamp = sched_cpu_timestamp();
    uint64_t last_mem_timestamp = sched.dma.last_mem_timestamp;
    while (true) {
        if (timestamp_before(cpu_timestamp, last_mem_timestamp)) {
            if (duration) {
                uint32_t prev_cycle = (uint32_t)div_ceil(last_mem_timestamp, &sched.clocks[CLOCK_CPU]);
                cpu.dmaCycles += prev_cycle - cpu.cycles;
                cpu.cycles = prev_cycle;
            }
            break;
        }
        enum sched_item_id id = sched.dma.next;
        struct sched_item *item = &sched.items[id];
        if (timestamp_before(cpu_timestamp, item->timestamp)) {
            break;
        }
        assert(id != SCHED_NO_DMA);
        assert(sched_active(id));
        if (timestamp_before(last_mem_timestamp, item->timestamp)) {
            last_mem_timestamp = item->timestamp;
        }
        /* Optimized schedule clear since this is currently the next DMA */
        item->timestamp |= SCHED_INACTIVE_FLAG;
        sched_update_dmas();
        last_mem_timestamp += (uint64_t)item->callback.dma(id) * sched.clocks[item->clock].tick_unit;
    }
    if (duration) {
        cpu.cycles += duration;
        last_mem_timestamp = sched_cpu_timestamp();
    }
    sched.dma.last_mem_timestamp = last_mem_timestamp;
}

static void sched_second(enum sched_item_id id) {
    sched_process_pending_dma(0);
    for (id = SCHED_FIRST_EVENT; id <= SCHED_LAST_DMA; id++) {
        if (sched_active(id)) {
            sched.items[id].timestamp -= SCHED_BASE_CLOCK_RATE;
            assert(sched_active(id));
        }
    }
    uint32_t cpu_clock_rate = sched_get_clock_rate(CLOCK_CPU);
    assert(cpu.cycles >= cpu_clock_rate);
    cpu.seconds++;
    cpu.cycles -= cpu_clock_rate;
    cpu.eiDelay -= cpu_clock_rate;
    cpu.baseCycles += cpu_clock_rate;
    sched.dma.last_mem_timestamp = timestamp_diff(sched.dma.last_mem_timestamp, SCHED_BASE_CLOCK_RATE);
    assert(sched.items[SCHED_SECOND].timestamp == SCHED_BASE_CLOCK_RATE);
    sched_update_events();
}

void sched_rewind_cpu(uint8_t duration) {
#if __has_builtin(__builtin_sub_overflow) || (__GNUC__ >= 5)
    if (likely(!__builtin_sub_overflow(cpu.cycles, duration, &cpu.cycles))) {
#elif defined(_MSC_VER) && (defined(_M_IX86) || defined(_M_X64))
    if (likely(!_subborrow_u32(0, cpu.cycles, duration, &cpu.cycles))) {
#else
    bool underflow = cpu.cycles < duration;
    cpu.cycles -= duration;
    if (likely(!underflow)) {
#endif
        return;
    }

    /* Cycles underflowed, so undo the effects of sched_second */
    enum sched_item_id id;
    for (id = SCHED_FIRST_EVENT; id <= SCHED_LAST_DMA; id++) {
        if (sched_active(id)) {
            sched.items[id].timestamp += SCHED_BASE_CLOCK_RATE;
            assert(sched_active(id));
        }
    }
    uint32_t cpu_clock_rate = sched_get_clock_rate(CLOCK_CPU);
    cpu.seconds--;
    cpu.cycles += cpu_clock_rate;
    cpu.eiDelay += cpu_clock_rate;
    cpu.baseCycles -= cpu_clock_rate;
    sched.dma.last_mem_timestamp += SCHED_BASE_CLOCK_RATE;
    assert(sched.items[SCHED_SECOND].timestamp == SCHED_BASE_CLOCK_RATE);
    sched_update_next(SCHED_SECOND); /* All other events are >= 1 second now */
}

void sched_init_event(enum sched_item_id id, enum clock_id clock, void (*event)(enum sched_item_id id)) {
    assert(id >= SCHED_FIRST_EVENT && id <= SCHED_LAST_EVENT);
    struct sched_item *item = &sched.items[id];
    sched_clear(id);
    item->clock = clock;
    item->callback.event = event;
}

void sched_init_dma(enum sched_item_id id, enum clock_id clock, uint32_t (*dma)(enum sched_item_id id)) {
    assert(id >= SCHED_FIRST_DMA && id <= SCHED_LAST_DMA);
    struct sched_item *item = &sched.items[id];
    sched_clear(id);
    item->clock = clock;
    item->callback.dma = dma;
}

bool sched_set_clock(enum clock_id clock, uint32_t new_rate) {
    enum sched_item_id id;
    struct sched_item *item;

    assert(clock >= CLOCK_CPU && clock < CLOCK_48M);
    if (new_rate <= (SCHED_BASE_CLOCK_RATE >> 32)) {
        return false;
    }
    struct sched_clock new_clock = { .tick_unit = (uint32_t)(SCHED_BASE_CLOCK_RATE / new_rate) };
    uint32_t remainder = (uint32_t)(SCHED_BASE_CLOCK_RATE % new_rate);
    if (clock == CLOCK_CPU) {
        /* Don't allow non-integer CPU clock rates */
        if (remainder != 0) {
            return false;
        }
    } else {
        /* Round up the tick unit */
        new_clock.tick_unit += (remainder * 2 >= new_rate);
        if (new_clock.tick_unit == 0) {
            return false;
        }
    }
    update_reciprocal(&new_clock);
    uint32_t base_tick = muldiv_floor(cpu.cycles, &sched.clocks[CLOCK_CPU], &new_clock);
    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        item = &sched.items[id];
        if (item->clock == clock && sched_active(id)) {
            uint64_t ticks = base_tick + sched_ticks_remaining(id);
            item->timestamp = ticks * new_clock.tick_unit;
        }
    }
    if (clock == CLOCK_CPU) {
        cpu.baseCycles += cpu.cycles;
        cpu.cycles = base_tick;
        cpu.baseCycles -= cpu.cycles;
    }
    sched.clocks[clock] = new_clock;
    sched_update_events();
    return true;
}

void sched_set_item_clock(enum sched_item_id id, enum clock_id clock) {
    struct sched_item *item = &sched.items[id];
    if (sched_active(id) && sched.clocks[clock].tick_unit != sched.clocks[item->clock].tick_unit) {
        uint64_t ticks = sched_ticks_remaining(id);
        item->clock = clock;
        sched_set(id, ticks);
    } else {
        item->clock = clock;
    }
}

uint32_t sched_get_clock_rate(enum clock_id clock) {
    return (uint32_t)div_round(SCHED_BASE_CLOCK_RATE, &sched.clocks[clock]);
}

void sched_reset(void) {
    const uint32_t def_rates[CLOCK_NUM_ITEMS] = { 48000000, 10000000, 60, 48000000, 24000000, 12000000, 6000000, 3000000, 1000000, 32768 };

    struct sched_item usb_device_item = sched.items[SCHED_USB_DEVICE];

    memset(&sched, 0, sizeof sched);
    for (enum sched_item_id id = 0; id < SCHED_NUM_ITEMS; id++) {
        sched.items[id].timestamp = SCHED_INACTIVE_FLAG;
    }

    for (enum clock_id clock = 0; clock < CLOCK_NUM_ITEMS; clock++) {
        assert(SCHED_BASE_CLOCK_RATE % def_rates[clock] == 0);
        assert(def_rates[clock] > (SCHED_BASE_CLOCK_RATE >> 32));
        sched.clocks[clock].tick_unit = (uint32_t)(SCHED_BASE_CLOCK_RATE / def_rates[clock]);
        update_reciprocal(&sched.clocks[clock]);
    }

    sched.items[SCHED_SECOND].callback.event = sched_second;
    sched.items[SCHED_SECOND].clock = CLOCK_32K;
    sched.items[SCHED_SECOND].timestamp = SCHED_BASE_CLOCK_RATE;
    sched_update_next(SCHED_SECOND);

    sched_init_event(SCHED_RUN, CLOCK_RUN, sched_run_event);
    sched_set(SCHED_RUN, 0);

    sched.items[SCHED_USB_DEVICE] = usb_device_item;

    sched.items[SCHED_NO_DMA].callback.dma = NULL;
    sched.items[SCHED_NO_DMA].clock = CLOCK_48M;
    sched.items[SCHED_NO_DMA].timestamp = SCHED_INACTIVE_FLAG;
    sched.dma.next = SCHED_NO_DMA;
    sched.dma.last_mem_timestamp = 0;
}

void sched_init(void) {
    memset(&sched, 0, sizeof sched);
    for (enum sched_item_id id = 0; id < SCHED_NUM_ITEMS; id++) {
        sched.items[id].timestamp = SCHED_INACTIVE_FLAG;
    }
}

uint64_t sched_total_cycles(void) {
    return cpu.baseCycles + cpu.cycles;
}

uint64_t sched_total_time(enum clock_id clock) {
    return (uint64_t)cpu.seconds * sched_get_clock_rate(clock) + muldiv_floor(cpu.cycles, &sched.clocks[CLOCK_CPU], &sched.clocks[clock]);
}

uint64_t event_next_cycle(enum sched_item_id id) {
    return sched_cycle(id) - sched_event_next_cycle() + cpu.baseCycles;
}

bool sched_save(FILE *image) {
    return fwrite(&sched, sizeof(sched), 1, image) == 1;
}

bool sched_restore(FILE *image) {
    bool ret;
    enum sched_item_id id;
    union sched_callback callbacks[SCHED_NUM_ITEMS];

    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        callbacks[id] = sched.items[id].callback;
    }

    ret = fread(&sched, sizeof(sched), 1, image) == 1;

    for (id = 0; id < SCHED_NUM_ITEMS; id++) {
        sched.items[id].callback = callbacks[id];
    }

    return ret;
}

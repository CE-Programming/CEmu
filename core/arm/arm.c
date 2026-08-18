#include "armstate.h"
#include "../defines.h"
#include "../os/os.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void reset(arm_t *arm, uint8_t rcause) {
    sync_wake(&arm->sync);
    arm_mem_reset(&arm->mem, rcause);
    arm_cpu_reset(arm);
    spsc_queue_clear(&arm->usart[0]);
    spsc_queue_clear(&arm->usart[1]);
}

static void run_quantum(arm_t *arm) {
    uint8_t i = 0;
    spsc_queue_entry_t peek;
    uint16_t val;

    do {
        arm_cpu_execute(arm);
    } while (++i && !arm->sync.slp);

    peek = spsc_queue_peek(&arm->usart[0]);
    if (unlikely(peek != SPSC_QUEUE_INVALID_ENTRY &&
                 arm_mem_usart_recv(arm, 3, peek))) {
        spsc_queue_entry_t entry = spsc_queue_dequeue(&arm->usart[0]);
        (void)entry;
        assert(entry == peek && "Already successfully peeked");
    }
    if (unlikely(spsc_queue_flush(&arm->usart[1]) &&
                 arm_mem_usart_send(arm, 3, &val))) {
        bool success = spsc_queue_enqueue(&arm->usart[1], val);
        (void)success;
        assert(success && "Already successfully flushed, so can't fail");
    }
}

enum { ARM_SPI_EVENT_MAX_QUANTA = 64 };

static void service_spi_event(arm_t *arm, uint8_t pending_flag) {
    /* The coprocessor and ASIC run concurrently on hardware. Hand each frame
     * edge to firmware before exposing the next edge to the eZ80, preventing
     * SSL, RXC, and TXC from collapsing into one observation. The ceiling is
     * a guard for firmware that is not currently servicing SPI; event flags
     * normally clear after only a few instructions. */
    for (uint8_t i = 0;
         i < ARM_SPI_EVENT_MAX_QUANTA && !arm->sync.slp &&
         (i == 0 ||
          (arm->mem.sercom[0].SPI.INTFLAG.reg & pending_flag));
         ++i) {
        run_quantum(arm);
    }
}

static int arm_thrd(void *context) {
    arm_t *arm = context;
    reset(arm, PM_RCAUSE_POR);
    while (sync_loop(&arm->sync)) {
        run_quantum(arm);
    }
    spsc_queue_destroy(&arm->usart[1]);
    spsc_queue_destroy(&arm->usart[0]);
    arm_mem_destroy(&arm->mem);
    free(arm);
    return 0;
}

arm_t *arm_create(void) {
    arm_t *arm = malloc(sizeof(arm_t));
    if (likely(arm)) {
        if (likely(sync_init(&arm->sync))) {
            if (likely(arm_mem_init(&arm->mem))) {
                if (likely(spsc_queue_init(&arm->usart[0]))) {
                    if (likely(spsc_queue_init(&arm->usart[1]))) {
                        arm->debug = false;
                        if (likely(thrd_create(&arm->thrd, &arm_thrd, arm) == thrd_success)) {
                            return arm;
                        }
                        spsc_queue_destroy(&arm->usart[1]);
                    }
                    spsc_queue_destroy(&arm->usart[0]);
                }
                arm_mem_destroy(&arm->mem);
            }
            sync_destroy(&arm->sync);
        }
        free(arm);
    }
    return NULL;
}

void arm_destroy(arm_t *arm) {
    if (arm) {
        thrd_t thread = arm->thrd;
        sync_enter(&arm->sync);
        arm->sync.run = false;
        sync_wake(&arm->sync);
        sync_leave(&arm->sync);
        (void)thrd_join(thread, NULL);
    }
}

void arm_reset(arm_t *arm) {
    sync_enter(&arm->sync);
    reset(arm, PM_RCAUSE_EXT);
    sync_leave(&arm->sync);
}

bool arm_load(arm_t *arm, const char *path) {
    bool success = false;
    FILE *file = fopen_utf8(path, "rb");
    if (likely(file)) {
        sync_enter(&arm->sync);
        success = arm_mem_load_rom(&arm->mem, file);
        fclose(file);
        reset(arm, PM_RCAUSE_POR);
        //arm->debug = true;
        sync_leave(&arm->sync);
    }
    return success;
}

void arm_spi_sel(arm_t *arm, bool low) {
    sync_enter(&arm->sync);
    bool power_down_pending = arm->cpu.pm &&
                              arm->mem.pm.SLEEP.bit.IDLE == PM_SLEEP_IDLE_APB_Val &&
                              (arm->cpu.scb.scr & SCB_SCR_SLEEPDEEP_Msk);
    if (low && power_down_pending) {
        /* The firmware's shutdown command masks interrupts, configures deep
         * sleep, and then waits for an external reset. A state image can land
         * between those operations and WFI, so the register state is the
         * reliable indication that selection should restart the coprocessor. */
        reset(arm, PM_RCAUSE_EXT);
    } else {
        sync_wake(&arm->sync);
    }
    //printf("%c\n", low ? 'L' : 'H');
    arm_mem_spi_sel(arm, 0, low);
    service_spi_event(arm, low ? SERCOM_SPI_INTFLAG_SSL :
                                SERCOM_SPI_INTFLAG_TXC);
    if (arm->sync.slp) {
        sync_leave(&arm->sync);
    } else {
        sync_run_leave(&arm->sync);
    }
}

static void debug_byte(bool dir, unsigned char c) {
    //printf("\x1b[%dm%02X\x1b[m", 94 + dir, c);
    //fflush(stdout);
}

static void debug_char(bool dir, char c) {
    if (c >= ' ' && c <= '~') {
        //printf("\x1b[%dm%c\x1b[m", 94 + dir, c);
        //fflush(stdout);
    }
}

uint8_t arm_spi_peek(arm_t *arm, uint32_t *res) {
    sync_enter(&arm->sync);
    sync_wake(&arm->sync);
    service_spi_event(arm, 0);
    uint8_t bits = arm_mem_spi_peek(arm, 0, res);
    if (bits == 8) {
        debug_byte(true, *res);
    }
    if (arm->sync.slp) {
        sync_leave(&arm->sync);
    } else {
        sync_run_leave(&arm->sync);
    }
    return bits;
}

uint8_t arm_spi_xfer(arm_t *arm, uint32_t val, uint32_t *res) {
    sync_enter(&arm->sync);
    sync_wake(&arm->sync);
    debug_byte(false, val);
    //printf("%02X ", val);
    arm_mem_spi_xfer(arm, 0, val);
    service_spi_event(arm, SERCOM_SPI_INTFLAG_RXC);
    uint8_t bits = arm_mem_spi_peek(arm, 0, res);
    if (bits == 8) {
        debug_byte(true, *res);
    }
    //printf("<-> %02X\n", *res);
    if (arm->sync.slp) {
        sync_leave(&arm->sync);
    } else {
        sync_run_leave(&arm->sync);
    }
    return bits;
}

bool arm_usart_send(arm_t *arm, uint8_t val) {
    bool success = spsc_queue_enqueue(&arm->usart[0], val);
    if (likely(success)) {
        (void)spsc_queue_flush(&arm->usart[0]);
        debug_char(false, val);
    }

    // The firmware can sleep while waiting for SERCOM RX; queued data must wake it.
    sync_enter(&arm->sync);
    sync_wake(&arm->sync);
    sync_leave(&arm->sync);

    return success;
}

bool arm_usart_recv(arm_t *arm, uint8_t *val) {
    spsc_queue_entry_t entry = spsc_queue_dequeue(&arm->usart[1]);
    *val = entry;
    if (entry != SPSC_QUEUE_INVALID_ENTRY) {
        debug_char(true, *val);

        // Freeing transmit-queue space can make SERCOM DRE progress again
        sync_enter(&arm->sync);
        sync_wake(&arm->sync);
        sync_leave(&arm->sync);
    }
    return entry != SPSC_QUEUE_INVALID_ENTRY;
}

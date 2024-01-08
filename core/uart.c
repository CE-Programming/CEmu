#include "uart.h"
#include "emu.h"
#include "interrupt.h"
#include "schedule.h"

#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#define UART_RX_TIMEOUT_DELAY 4

uart_state_t uart;

static uart_transfer_t loopback_transfer;
static bool loopback_available = false;

static bool bit_parity(uint8_t byte) {
#if __has_builtin(__builtin_parity)
    return __builtin_parity(byte);
#else
    byte ^= byte >> 4;
    byte ^= byte >> 2;
    byte ^= byte >> 1;
    return byte & 1;
#endif
}

static uint8_t uart_get_modem_inputs(void) {
    if (uart.mcr >> 4 & 1) {
        /* Loopback mode */
        return uart.mcr & UART_MSR_BITS;
    } else {
        return uart.msr >> 4 & UART_MSR_BITS;
    }
}

void uart_set_modem_inputs(uint8_t msr) {
    uint8_t modemDiff = uart_get_modem_inputs();
    uart.msr &= UART_MSR_BITS;
    uart.msr |= msr << 4;
    modemDiff ^= uart_get_modem_inputs();
    if (unlikely(modemDiff)) {
        uart.msr |= modemDiff;
        uart.isr |= 1 << 3;
        intrpt_set(INT_UART, uart.ier & uart.isr);
    }
}

static void uart_set_modem_outputs(uint8_t mcr) {
    (void)mcr;
}

static void uart_transmit_loopback(const uart_transfer_t* transfer) {
    loopback_transfer = *transfer;
    loopback_available = true;
}

static bool uart_receive_loopback(uart_transfer_t* transfer) {
    if (!loopback_available) {
        return false;
    }

    *transfer = loopback_transfer;
    loopback_available = false;
    return true;
}

static void uart_transmit_null(const uart_transfer_t* transfer) {
    (void)transfer;
}

static bool uart_receive_null(uart_transfer_t* transfer) {
    (void)transfer;
    return false;
}

static void uart_set_device_funcs(void) {
    if (uart.mcr >> 4 & 1) {
        /* Loopback mode */
        uart.transmit_char = uart_transmit_loopback;
        uart.receive_char = uart_receive_loopback;
        /* Modem outputs not seen externally */
        uart_set_modem_outputs(0);
    }
    else {
        /* For non-Python models, discard transmitted characters */
        uart.transmit_char = uart_transmit_null;
        uart.receive_char = uart_receive_null;
        /* Modem outputs seen externally */
        uart_set_modem_outputs(uart.mcr & UART_MSR_BITS);
    }
}

static bool uart_fifo_enabled(void) {
    return uart.fcr >> 0 & 1;
}

static uint32_t uart_char_ticks(void) {
    uint32_t divisor = (uint16_t)(uart.divisor - 1) + 1;
    /* Start + Character + Parity + Stop */
    uint32_t bits = 1 + 5 + (uart.lcr >> 0 & 3) + (uart.lcr >> 3 & 1) + 1 + (uart.lcr >> 2 & 1);
    return divisor * bits;
}

static bool uart_timer_should_run(void) {
    return uart.txActive || uart.rxTimeoutChars;
}

static void uart_set_timer(bool force) {
    if (uart_timer_should_run()) {
        if (force || !sched_active(SCHED_UART)) {
            sched_set(SCHED_UART, uart_char_ticks());
        }
    } else {
        sched_clear(SCHED_UART);
    }
}

static void uart_reset_rx_timeout(void) {
    uart.rxTimeout = false;
    uart.rxTimeoutChars = uart.rfve && uart_fifo_enabled() ? UART_RX_TIMEOUT_DELAY : 0;
    uart_set_timer(false);
}

static bool uart_receive_char() {
    uart_transfer_t transfer;
    if (!uart.receive_char(&transfer)) {
        /* RX timeout not reset*/
        return false;
    }

    uint8_t stat = 0;
    if (unlikely(transfer.divisor != uart.divisor)) {
        /* Frame error */
        stat = 1 << 3;
    } else {
        uint8_t lcrDiff = uart.lcr & (UART_LCR_CHAR_LEN | UART_LCR_STOP_BITS | UART_LCR_PARITY_ENABLE | UART_LCR_PARITY_EVEN | UART_LCR_PARITY_STICK);
        lcrDiff ^= transfer.lcr;
        if (unlikely(lcrDiff)) {
            if (lcrDiff & UART_LCR_BREAK) {
                /* Break/frame error */
                transfer.ch = 0;
                stat = 1 << 4 | 1 << 3;
            } else if (lcrDiff & (UART_LCR_CHAR_LEN | UART_LCR_STOP_BITS | UART_LCR_PARITY_ENABLE)) {
                /* Frame error */
                stat = 1 << 3;
            } else if (transfer.lcr & UART_LCR_PARITY_ENABLE) {
                bool parityDiff = lcrDiff & UART_LCR_PARITY_EVEN;
                if (lcrDiff & UART_LCR_PARITY_STICK) {
                    parityDiff ^= bit_parity(transfer.ch);
                }
                /* Parity error */
                stat = parityDiff << 2;
            }
        }
    }

    size_t index = (uart.rfvi + uart.rfve) & (UART_FIFO_DEPTH - 1);
    if (uart_fifo_enabled()) {
        /* When FIFO is enabled, received bytes are discarded on overrun */
        if (uart.rfve == UART_FIFO_DEPTH) {
            /* Overrun error */
            uart.lsr |= 1 << 1;
            uart.isr |= 1 << 2;
            /* RX timeout not reset*/
            return false;
        }
        /* Write new data into the FIFO */
        uart.rxfifo[index] = transfer.ch;
        uart.rxstat[index] = stat;
        /* On error, track the error count and set the FIFO error bit */
        if (stat) {
            uart.rxstatCount++;
            uart.lsr |= 1 << 7;
        }
        /* Restart the timeout counter if not timed out */
        if (!uart.rxTimeoutChars) {
            uart.rxTimeoutChars = UART_RX_TIMEOUT_DELAY;
        }
    } else {
        /* When FIFO is disabled, holding register is unconditionally overwritten */
        uart.rxfifo[index] = transfer.ch;
        if (uart.rfve == 1) {
            /* Overrun error, plus any other errors from the received byte */
            uart.lsr |= 1 << 1 | stat;
            uart.isr |= 1 << 2;
            /* RX timeout reset */
            return true;
        }
    }
    if (uart.rfve++ == 0) {
        /* If this is the first FIFO entry inserted, update LSR */
        uart.lsr |= 1 << 0 | stat;
        /* Also trigger an interrupt on error conditions */
        uart.isr |= (stat != 0) << 2;
    }
    /* RX timeout reset */
    return true;
}

static void uart_update_rxfifo_trigger(void) {
    uint8_t level = 1;
    if (uart_fifo_enabled() && !uart.rxTimeout) {
        level = 0x0D080401 >> ((uart.fcr >> 6 & 3) * 8);
    }
    uart.isr &= ~(1 << 0);
    uart.isr |= (uart.rfve >= level) << 0;
}

static void uart_next_transmit(void) {
    /* Put character in transmit shift register */
    uint8_t bits = 5 + (uart.lcr >> 0 & 3);
    uart.tsr = uart.txfifo[uart.tfvi++ & (UART_FIFO_DEPTH - 1)] & ((1 << bits) - 1);
    /* If FIFO is now empty, set corresponding status and interrupt */
    if (!likely(--uart.tfve)) {
        uart.lsr |= 1 << 5;
        uart.isr |= 1 << 1;
    }
}

static void uart_event(enum sched_item_id id) {
    if (uart.txActive) {
        /* Finish transmit of current byte */
        uart_transfer_t transfer = {
            .ch = uart.tsr,
            .lcr = uart.lcr,
            .divisor = uart.divisor
        };
        uart.transmit_char(&transfer);

        /* Transmit next byte or stop */
        if (likely(uart.tfve)) {
            uart_next_transmit();
        } else {
            uart.txActive = false;
            /* Indicate empty FIFO and empty shift register */
            uart.lsr |= 1 << 6;
        }
    }

    if (!uart_receive_char() && uart.rxTimeoutChars) {
        /* Set the timeout flag if the count expired */
        uart.rxTimeout = (--uart.rxTimeoutChars == 0);
    }

    /* Update RX FIFO interrupt based on new fill level and timeout */
    uart_update_rxfifo_trigger();
    intrpt_set(INT_UART, uart.ier & uart.isr);

    if (uart_timer_should_run()) {
        sched_repeat(id, uart_char_ticks());
    }
}

/* Read from the 0xE000 range of ports */
static uint8_t uart_read(const uint16_t pio, bool peek) {
    uint8_t value;

    switch (pio) {
        case 0x00:
            if (uart.lcr >> 7 & 1) {
                value = uart.divisor & 0xFF;
            } else if (uart.rfve) {
                /* FIFO read */
                size_t index = uart.rfvi & (UART_FIFO_DEPTH - 1);
                value = uart.rxfifo[index];
                if (!peek) {
                    /* Remove the error status tracking from the FIFO */
                    if (uart.rxstat[index]) {
                        if (--uart.rxstatCount == 0) {
                            uart.lsr &= ~(1 << 7);
                        }
                    }
                    uart.rfvi++;
                    if (--uart.rfve) {
                        /* Indicate error status from the next FIFO entry */
                        index = uart.rxstat[uart.rfvi & (UART_FIFO_DEPTH - 1)];
                        uart.lsr |= uart.rxstat[index];
                        uart.isr |= (uart.rxstat[index] != 0) << 2;
                    } else {
                        /* Indicate empty FIFO */
                        uart.lsr &= ~(1 << 0);
                    }
                    /* Reset the timeout status and interrupt based on the new FIFO state */
                    uart_reset_rx_timeout();
                    uart_update_rxfifo_trigger();
                    intrpt_set(INT_UART, uart.ier & uart.isr);
                }
            } else {
                /* Empty FIFO */
                value = 0;
            }
            break;
        case 0x04:
            if (uart.lcr >> 7 & 1) {
                value = uart.divisor >> 8;
            } else {
                value = uart.ier;
            }
            break;
        case 0x08:
            /* Determine interrupt by priority */
            value = uart.ier & uart.isr;
            if (value >> 2 & 1) {
                /* Receiver Line Status */
                value = 6;
            } else if (value >> 0 & 1) {
                /* Received Data Available / Timeout */
                value = 4 | uart.rxTimeout << 3;
            } else if (value >> 1 & 1) {
                /* Transmitter Holding Register Empty */
                uart.isr &= ~(1 << 1);
                intrpt_set(INT_UART, value & ~(1 << 1));
                value = 2;
            } else {
                /* Modem Status or no interrupt */
                value = !value;
            }
            /* Indicate FIFO full in Bit 4 (non-standard extension) */
            value |= (uart.tfve & UART_FIFO_DEPTH);
            if (uart_fifo_enabled()) {
                value |= 0xC0;
            }
            break;
        case 0x0C:
            value = uart.lcr;
            break;
        case 0x10:
            value = uart.mcr;
            break;
        case 0x14:
            value = uart.lsr;
            /* Clear error bits and pending interrupt */
            uart.lsr &= ~(1 << 4 | 1 << 3 | 1 << 2 | 1 << 1);
            if (unlikely(uart.isr >> 2 & 1)) {
                uart.isr &= ~(1 << 2);
                intrpt_set(INT_UART, uart.ier & uart.isr);
            }
            break;
        case 0x18:
            value = uart_get_modem_inputs() << 4;
            value |= uart.msr & UART_MSR_BITS;
            /* Clear delta bits and pending interrupt */
            uart.msr &= ~UART_MSR_BITS;
            if (unlikely(uart.isr >> 3 & 1)) {
                uart.isr &= ~(1 << 3);
                intrpt_set(INT_UART, uart.ier & uart.isr);
            }
            break;
        case 0x1C:
            value = uart.scratch;
            break;
        /* Revision registers? */
        case 0x68:
            value = 0x01;
            break;
        case 0x6C:
            value = 0x01;
            break;
        case 0x70:
            value = 0x10;
            break;
        default:
            value = 0x00;
            break;
    }
    return value;
}

/* Write to the 0xE000 range of ports */
static void uart_write(const uint16_t pio, const uint8_t byte, bool poke) {
    (void)poke;
    switch (pio) {
        case 0x00:
            if (unlikely(uart.lcr >> 7 & 1)) {
                uart.divisor &= ~0xFF;
                uart.divisor |= byte;
                uart_set_timer(true);
            } else {
                /* FIFO write */
                if (uart.tfve != (uart_fifo_enabled() ? UART_FIFO_DEPTH : 1)) {
                    uart.txfifo[(uart.tfvi + uart.tfve++) & (UART_FIFO_DEPTH - 1)] = byte;
                    uart.lsr &= ~(1 << 6 | 1 << 5);
                    uart.isr &= ~(1 << 1);
                    if (!likely(uart.txActive)) {
                        uart_next_transmit();
                        uart.txActive = true;
                        uart_set_timer(true);
                    }
                    intrpt_set(INT_UART, uart.ier & uart.isr);
                }
            }
            break;
        case 0x04:
            if (unlikely(uart.lcr >> 7 & 1)) {
                uart.divisor &= ~(0xFF << 8);
                uart.divisor |= byte << 8;
                uart_set_timer(true);
            }
            else {
                uart.ier = byte;
                intrpt_set(INT_UART, uart.ier & uart.isr);
            }
            break;
        case 0x08: {
            const uint8_t diff = uart.fcr ^ byte;
            uart.fcr = byte & ~(1 << 2 | 1 << 1);
            if (diff & (1 << 2 | 1 << 0)) {
                /* Clear TX FIFO */
                uart.tfvi = uart.tfve = 0;
                uart.isr |= 1 << 1;
                uart.lsr |= (1 << 6 | 1 << 5);
                uart.lsr &= ~(uart.txActive << 6);
            }
            if (diff & (1 << 1 | 1 << 0)) {
                /* Clear RX FIFO */
                memset(&uart.rxstat, 0, sizeof(uart.rxstat));
                uart.rfvi = uart.rfve = uart.rxstatCount = 0;
                uart.lsr &= ~(1 << 7 | 1 << 0);
                uart_reset_rx_timeout();
            }
            uart_update_rxfifo_trigger();
            intrpt_set(INT_UART, uart.ier & uart.isr);
            break;
        }
        case 0x0C: {
            uint8_t lcrDiff = uart.lcr ^ byte;
            uart.lcr = byte;
            if (lcrDiff & (UART_LCR_CHAR_LEN | UART_LCR_STOP_BITS | UART_LCR_PARITY_ENABLE)) {
                uart_set_timer(true);
            }
            break;
        }
        case 0x10: {
            uint8_t modemDiff = uart_get_modem_inputs();
            uart.mcr = byte;
            modemDiff ^= uart_get_modem_inputs();
            if (unlikely(modemDiff)) {
                uart.msr |= modemDiff;
                uart.isr |= 1 << 3;
                intrpt_set(INT_UART, uart.ier & uart.isr);
            }
            uart_set_device_funcs();
            break;
        }
        case 0x1C:
            uart.scratch = byte;
            break;
        default:
            break;
    }
}

static const eZ80portrange_t puart = {
    .read = uart_read,
    .write = uart_write
};

eZ80portrange_t init_uart(void) {
    gui_console_printf("[CEmu] Initialized UART...\n");
    return puart;
}

void uart_reset(void) {
    memset(&uart, 0, sizeof(uart));
    uart.lsr = 1 << 6 | 1 << 5;

    uart_set_device_funcs();

    sched.items[SCHED_UART].callback.event = uart_event;
    sched.items[SCHED_UART].clock = CLOCK_3M;
    uart_set_timer(true);
}

bool uart_save(FILE *image) {
    return fwrite(&uart, offsetof(uart_state_t, transmit_char), 1, image) == 1;
}

bool uart_restore(FILE *image) {
    if (fread(&uart, offsetof(uart_state_t, transmit_char), 1, image) == 1) {
        uart_set_device_funcs();
        return true;
    }
    return false;
}

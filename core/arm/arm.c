#include "armstate.h"
#include "../defines.h"
#include "../os/os.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define BOOTLOADER_REGION_SIZE UINT32_C(0x2000)

static const uint8_t *find_bytes(const uint8_t *data, size_t data_size, const char *text) {
    size_t text_size = strlen(text);
    if (!text_size || text_size > data_size) {
        return NULL;
    }
    for (size_t offset = 0; offset <= data_size - text_size; ++offset) {
        if (memcmp(data + offset, text, text_size) == 0) {
            return data + offset;
        }
    }
    return NULL;
}

static void copy_description(char *description, size_t desc_size, const char *prefix,
                             const uint8_t *value, size_t value_size) {
    if (!desc_size) {
        return;
    }
    size_t prefix_size = strlen(prefix);
    size_t copy_prefix = prefix_size < desc_size - 1 ? prefix_size : desc_size - 1;
    memcpy(description, prefix, copy_prefix);
    size_t remaining = desc_size - 1 - copy_prefix;
    size_t copy_value = value_size < remaining ? value_size : remaining;
    if (copy_value) {
        memcpy(description + copy_prefix, value, copy_value);
    }
    description[copy_prefix + copy_value] = '\0';
}

static bool parse_uf2_info(const uint8_t *flash, const char *line_prefix,
                           const char *metadata, const char *description_prefix,
                           char *description, size_t desc_size) {
    const uint8_t *search = flash;
    const uint8_t *region_end = flash + BOOTLOADER_REGION_SIZE;
    size_t remaining = BOOTLOADER_REGION_SIZE;
    while (remaining) {
        const uint8_t *line = find_bytes(search, remaining, line_prefix);
        if (!line) {
            return false;
        }
        const uint8_t *version = line + strlen(line_prefix);
        const uint8_t *end = version;
        while (end < region_end && *end >= UINT8_C(0x20) && *end <= UINT8_C(0x7E)) {
            ++end;
        }
        size_t metadata_size = strlen(metadata);
        if (end != version && (size_t)(region_end - end) >= metadata_size &&
            memcmp(end, metadata, metadata_size) == 0) {
            copy_description(description, desc_size, description_prefix, version, (size_t)(end - version));
            return true;
        }
        search = line + 1;
        remaining = (size_t)(region_end - search);
    }
    return false;
}

static arm_bootloader_type_t parse_bootloader_info(const uint8_t *flash, char *desc, size_t desc_size) {
    static const char free_prefix[] = "UF2 Bootloader CEmu free ";
    static const char free_metadata[] = "\r\nModel: TI-Python compatible\r\nBoard-ID: TI Python\r\n";
    static const char ti_prefix[] = "UF2 Bootloader ";
    static const char ti_metadata[] = "\r\nModel: TI-Python\r\nBoard-ID: TI Python\r\n";

    if (parse_uf2_info(flash, free_prefix, free_metadata, "CEmu free ", desc, desc_size)) {
        return ARM_BOOTLOADER_CEMU_FREE;
    }
    if (parse_uf2_info(flash, ti_prefix, ti_metadata, "TI UF2 ", desc, desc_size)) {
        return ARM_BOOTLOADER_TI_UF2;
    }

    copy_description(desc, desc_size, "unknown/custom", NULL, 0);
    return ARM_BOOTLOADER_UNKNOWN;
}

static void reset(arm_t *arm, uint8_t rcause) {
    sync_wake(&arm->sync);
    arm_mem_reset(&arm->mem, rcause);
    arm_cpu_reset(arm);
    spsc_queue_clear(&arm->usart[0]);
    spsc_queue_clear(&arm->usart[1]);
}

#ifdef COPROC_DEBUG_SUPPORT
static uint32_t debug_current_pc(const arm_t *arm) {
    return arm->cpu.pc >= 2 ? (arm->cpu.pc - 2) & ~UINT32_C(1) : 0;
}

static void debug_stop(arm_t *arm, arm_debug_stop_reason_t reason) {
    arm->gdb.stopped = true;
    arm->gdb.step_pending = false;
    arm->gdb.stop_reason = reason;
    sync_throttle(&arm->sync);
}

static bool debug_breakpoint_hit(arm_t *arm) {
    arm_debug_state_t *debug = &arm->gdb;
    if (!debug->attached || !debug->breakpoint_count) {
        debug->skip_breakpoint_once = false;
        return false;
    }

    const uint32_t pc = debug_current_pc(arm);
    if (debug->skip_breakpoint_once) {
        const bool skip = debug->skip_breakpoint == pc;
        debug->skip_breakpoint_once = false;
        if (skip) {
            return false;
        }
    }
    for (size_t index = 0; index != debug->breakpoint_count; ++index) {
        if (debug->breakpoints[index] == pc) {
            debug_stop(arm, ARM_DEBUG_STOP_BREAKPOINT);
            return true;
        }
    }
    return false;
}
#endif

static void run_quantum(arm_t *arm, bool limited) {
    uint8_t i = 0;
    spsc_queue_entry_t peek;
    uint16_t val;

#ifdef COPROC_DEBUG_SUPPORT
    if (!arm->gdb.stopped &&
        (!limited || arm->cycles < arm->cycle_limit || arm->gdb.step_pending)) {
#else
    if (!limited || arm->cycles < arm->cycle_limit) {
#endif
        do {
#ifdef COPROC_DEBUG_SUPPORT
            if (debug_breakpoint_hit(arm)) {
                break;
            }
#endif
            arm_cpu_execute(arm);
#ifdef COPROC_DEBUG_SUPPORT
            if (arm->gdb.step_pending) {
                debug_stop(arm, ARM_DEBUG_STOP_STEP);
                break;
            }
        } while (++i && !arm->sync.slp && !arm->gdb.stopped &&
                 (!limited || arm->cycles < arm->cycle_limit));
#else
        } while (++i && !arm->sync.slp &&
                 (!limited || arm->cycles < arm->cycle_limit));
#endif
    }

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
#ifdef COPROC_DEBUG_SUPPORT
         !arm->gdb.stopped &&
#endif
         (i == 0 ||
          (arm->mem.sercom[0].SPI.INTFLAG.reg & pending_flag));
         ++i) {
        run_quantum(arm, false);
    }
}

static int arm_thrd(void *context) {
    arm_t *arm = context;
    reset(arm, PM_RCAUSE_POR);
#ifdef COPROC_DEBUG_SUPPORT
    while (sync_loop(&arm->sync,
                     arm->gdb.stopped ||
                     (arm->cycles >= arm->cycle_limit && !arm->gdb.step_pending))) {
#else
    while (sync_loop(&arm->sync, arm->cycles >= arm->cycle_limit)) {
#endif
        run_quantum(arm, true);
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
                        arm->cycles = 0;
                        arm->cycle_limit = 0;
#ifdef COPROC_DEBUG_SUPPORT
                        memset(&arm->gdb, 0, sizeof(arm->gdb));
#endif
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
        sync_throttle_wake(&arm->sync);
        sync_leave(&arm->sync);
        (void)thrd_join(thread, NULL);
    }
}

void arm_set_time(arm_t *arm, uint64_t cycles) {
    sync_enter(&arm->sync);
    arm->cycles = cycles;
    arm->cycle_limit = cycles;
    sync_leave(&arm->sync);
}

void arm_advance_to(arm_t *arm, uint64_t cycles) {
    sync_enter(&arm->sync);
    if (cycles > arm->cycle_limit) {
        arm->cycle_limit = cycles;
    }
    if (arm->sync.slp) {
        /* Deep sleep gates the core clock. Keep its virtual timestamp aligned
         * without ticking SysTick or executing instructions. */
        if (cycles > arm->cycles) {
            arm->cycles = cycles;
        }
        sync_leave(&arm->sync);
        return;
    }
#ifdef COPROC_DEBUG_SUPPORT
    if (!arm->gdb.stopped && arm->cycles < arm->cycle_limit) {
#else
    if (arm->cycles < arm->cycle_limit) {
#endif
        sync_throttle_wake(&arm->sync);
    }
    sync_leave(&arm->sync);
}

void arm_run_until(arm_t *arm, uint64_t cycles) {
    arm_advance_to(arm, cycles);

    sync_wait_idle(&arm->sync);

    sync_enter(&arm->sync);
    if (arm->sync.slp && cycles > arm->cycles) {
        arm->cycles = cycles;
    }
    sync_leave(&arm->sync);
}

void arm_pause(arm_t *arm) {
    sync_enter(&arm->sync);
    arm->cycle_limit = arm->cycles;
    sync_throttle(&arm->sync);
    sync_leave(&arm->sync);
}

uint64_t arm_get_time(arm_t *arm) {
    uint64_t cycles;
    sync_enter(&arm->sync);
    cycles = arm->cycles;
    sync_leave(&arm->sync);
    return cycles;
}

bool arm_get_cpu_snapshot(arm_t *arm, arm_cpu_snapshot_t *snapshot) {
    if (!arm || !snapshot) {
        return false;
    }

    sync_enter(&arm->sync);
    memcpy(snapshot->registers, arm->cpu.r, sizeof(snapshot->registers));
    snapshot->alternate_stack_pointer = arm->cpu.altsp;
    snapshot->active_exceptions = arm->cpu.active;
    snapshot->overflow = arm->cpu.v;
    snapshot->carry = arm->cpu.c;
    snapshot->zero = arm->cpu.z;
    snapshot->negative = arm->cpu.n;
    snapshot->primask = arm->cpu.pm;
    snapshot->process_stack = arm->cpu.spsel;
    snapshot->exception = arm->cpu.exc;
    snapshot->wait_for_interrupt = arm->cpu.wfi;
    snapshot->svc_pending = arm->cpu.svc_pending;
    snapshot->systick.control = arm->cpu.systick.ctrl;
    snapshot->systick.reload = arm->cpu.systick.load;
    snapshot->systick.current = arm->cpu.systick.val;
    snapshot->systick.calibration = arm->cpu.systick.calib;
    snapshot->nvic.interrupt_enable = arm->cpu.nvic.ier;
    snapshot->nvic.interrupt_pending = arm->cpu.nvic.ipr;
    memcpy(snapshot->nvic.priorities, arm->cpu.nvic.ip,
           sizeof(snapshot->nvic.priorities));
    snapshot->scb.interrupt_control = arm->cpu.scb.icsr;
    snapshot->scb.vector_table = arm->cpu.scb.vtor;
    snapshot->scb.application_interrupt_reset_control = arm->cpu.scb.aircr;
    snapshot->scb.system_control = arm->cpu.scb.scr;
    memcpy(snapshot->scb.system_priorities, arm->cpu.scb.shp,
           sizeof(snapshot->scb.system_priorities));
    snapshot->cycles = arm->cycles;
    snapshot->cycle_limit = arm->cycle_limit;
    snapshot->sleeping = arm->sync.slp;
    sync_leave(&arm->sync);
    return true;
}

uint8_t arm_read_byte(arm_t *arm, uint32_t address) {
    sync_enter(&arm->sync);
    const uint8_t value = arm_mem_load_byte(arm, address);
    sync_leave(&arm->sync);
    return value;
}

uint16_t arm_read_half(arm_t *arm, uint32_t address) {
    sync_enter(&arm->sync);
    const uint16_t value = arm_mem_load_half(arm, address);
    sync_leave(&arm->sync);
    return value;
}

uint32_t arm_read_word(arm_t *arm, uint32_t address) {
    sync_enter(&arm->sync);
    const uint32_t value = arm_mem_load_word(arm, address);
    sync_leave(&arm->sync);
    return value;
}

void arm_write_byte(arm_t *arm, uint32_t address, uint8_t value) {
    sync_enter(&arm->sync);
    arm_mem_store_byte(arm, value, address);
    sync_leave(&arm->sync);
}

void arm_write_half(arm_t *arm, uint32_t address, uint16_t value) {
    sync_enter(&arm->sync);
    arm_mem_store_half(arm, value, address);
    sync_leave(&arm->sync);
}

void arm_write_word(arm_t *arm, uint32_t address, uint32_t value) {
    sync_enter(&arm->sync);
    arm_mem_store_word(arm, value, address);
    sync_leave(&arm->sync);
}

#ifdef COPROC_DEBUG_SUPPORT
bool arm_debug_attach(arm_t *arm) {
    if (!arm) {
        return false;
    }
    sync_enter(&arm->sync);
    arm->gdb.attached = true;
    arm->gdb.skip_breakpoint_once = false;
    debug_stop(arm, ARM_DEBUG_STOP_ATTACH);
    sync_leave(&arm->sync);
    return true;
}

void arm_debug_detach(arm_t *arm) {
    if (!arm) {
        return;
    }
    sync_enter(&arm->sync);
    memset(&arm->gdb, 0, sizeof(arm->gdb));
    /* Release the debug stop independently of firmware sleep. If the core was
     * already cycle-throttled, its worker will immediately throttle again. */
    sync_throttle_wake(&arm->sync);
    sync_leave(&arm->sync);
}

bool arm_debug_status(arm_t *arm, bool *stopped, arm_debug_stop_reason_t *reason) {
    if (!arm || !stopped || !reason) {
        return false;
    }
    sync_enter(&arm->sync);
    const bool attached = arm->gdb.attached;
    *stopped = arm->gdb.stopped;
    *reason = arm->gdb.stop_reason;
    sync_leave(&arm->sync);
    return attached;
}

bool arm_debug_interrupt(arm_t *arm) {
    if (!arm) {
        return false;
    }
    sync_enter(&arm->sync);
    const bool attached = arm->gdb.attached;
    if (attached) {
        debug_stop(arm, ARM_DEBUG_STOP_INTERRUPT);
    }
    sync_leave(&arm->sync);
    return attached;
}

bool arm_debug_resume(arm_t *arm, bool step) {
    if (!arm) {
        return false;
    }
    sync_enter(&arm->sync);
    if (!arm->gdb.attached) {
        sync_leave(&arm->sync);
        return false;
    }
    if (arm->gdb.stop_reason == ARM_DEBUG_STOP_BREAKPOINT) {
        arm->gdb.skip_breakpoint = debug_current_pc(arm);
        arm->gdb.skip_breakpoint_once = true;
    }
    arm->gdb.stopped = false;
    arm->gdb.step_pending = step;
    arm->gdb.stop_reason = ARM_DEBUG_STOP_NONE;
    if (step) {
        sync_wake(&arm->sync);
    }
    if (step || (!arm->sync.slp && arm->cycles < arm->cycle_limit)) {
        sync_throttle_wake(&arm->sync);
    }
    sync_leave(&arm->sync);
    return true;
}

static uint32_t debug_xpsr(const arm_cpu_t *cpu) {
    return (uint32_t)cpu->n << 31 |
           (uint32_t)cpu->z << 30 |
           (uint32_t)cpu->c << 29 |
           (uint32_t)cpu->v << 28 |
           UINT32_C(1) << 24 |
           (cpu->scb.icsr & SCB_ICSR_VECTACTIVE_Msk);
}

bool arm_debug_get_registers(arm_t *arm, arm_debug_registers_t *registers) {
    if (!arm || !registers) {
        return false;
    }
    sync_enter(&arm->sync);
    memcpy(registers->registers, arm->cpu.r, sizeof(registers->registers));
    registers->registers[15] = debug_current_pc(arm);
    registers->xpsr = debug_xpsr(&arm->cpu);
    sync_leave(&arm->sync);
    return true;
}

bool arm_debug_set_registers(arm_t *arm, const arm_debug_registers_t *registers) {
    if (!arm || !registers) {
        return false;
    }
    sync_enter(&arm->sync);
    memcpy(arm->cpu.r, registers->registers, 15 * sizeof(uint32_t));
    arm->cpu.pc = (registers->registers[15] & ~UINT32_C(1)) + 2;
    arm->cpu.n = registers->xpsr >> 31 & 1;
    arm->cpu.z = registers->xpsr >> 30 & 1;
    arm->cpu.c = registers->xpsr >> 29 & 1;
    arm->cpu.v = registers->xpsr >> 28 & 1;
    sync_leave(&arm->sync);
    return true;
}

bool arm_debug_read_memory(arm_t *arm, uint32_t address, uint8_t *data, size_t size) {
    if (!arm || (!data && size) || size > UINT32_MAX - address) {
        return false;
    }
    sync_enter(&arm->sync);
    size_t offset = 0;
    while (offset != size) {
        const uint32_t current = address + (uint32_t)offset;
        if (!(current & 3) && size - offset >= 4) {
            const uint32_t value = arm_mem_load_word(arm, current);
            for (unsigned int byte = 0; byte != 4; ++byte) {
                data[offset + byte] = (uint8_t)(value >> (byte * 8));
            }
            offset += 4;
        } else if (!(current & 1) && size - offset >= 2) {
            const uint16_t value = arm_mem_load_half(arm, current);
            data[offset++] = (uint8_t)value;
            data[offset++] = (uint8_t)(value >> 8);
        } else {
            data[offset++] = arm_mem_load_byte(arm, current);
        }
    }
    sync_leave(&arm->sync);
    return true;
}

bool arm_debug_write_memory(arm_t *arm, uint32_t address, const uint8_t *data, size_t size) {
    if (!arm || (!data && size) || size > UINT32_MAX - address) {
        return false;
    }
    sync_enter(&arm->sync);
    size_t offset = 0;
    while (offset != size) {
        const uint32_t current = address + (uint32_t)offset;
        if (!(current & 3) && size - offset >= 4) {
            const uint32_t value = (uint32_t)data[offset] |
                (uint32_t)data[offset + 1] << 8 |
                (uint32_t)data[offset + 2] << 16 |
                (uint32_t)data[offset + 3] << 24;
            arm_mem_store_word(arm, value, current);
            offset += 4;
        } else if (!(current & 1) && size - offset >= 2) {
            const uint16_t value = (uint16_t)data[offset] |
                (uint16_t)data[offset + 1] << 8;
            arm_mem_store_half(arm, value, current);
            offset += 2;
        } else {
            arm_mem_store_byte(arm, data[offset++], current);
        }
    }
    sync_leave(&arm->sync);
    return true;
}

bool arm_debug_add_breakpoint(arm_t *arm, uint32_t address) {
    if (!arm) {
        return false;
    }
    address &= ~UINT32_C(1);
    sync_enter(&arm->sync);
    for (size_t index = 0; index != arm->gdb.breakpoint_count; ++index) {
        if (arm->gdb.breakpoints[index] == address) {
            sync_leave(&arm->sync);
            return true;
        }
    }
    if (arm->gdb.breakpoint_count == ARM_DEBUG_MAX_BREAKPOINTS) {
        sync_leave(&arm->sync);
        return false;
    }
    arm->gdb.breakpoints[arm->gdb.breakpoint_count++] = address;
    sync_leave(&arm->sync);
    return true;
}

bool arm_debug_remove_breakpoint(arm_t *arm, uint32_t address) {
    if (!arm) {
        return false;
    }
    address &= ~UINT32_C(1);
    sync_enter(&arm->sync);
    for (size_t index = 0; index != arm->gdb.breakpoint_count; ++index) {
        if (arm->gdb.breakpoints[index] == address) {
            --arm->gdb.breakpoint_count;
            arm->gdb.breakpoints[index] = arm->gdb.breakpoints[arm->gdb.breakpoint_count];
            sync_leave(&arm->sync);
            return true;
        }
    }
    sync_leave(&arm->sync);
    return true;
}
#endif

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

arm_bootloader_type_t arm_get_bootloader_info(arm_t *arm, char *desc, size_t desc_size) {
    arm_bootloader_type_t type;
    sync_enter(&arm->sync);
    type = parse_bootloader_info((const uint8_t *)arm->mem.nvm, desc, desc_size);
    sync_leave(&arm->sync);
    return type;
}

bool arm_save_flash(arm_t *arm, FILE *image) {
    bool success;
    sync_enter(&arm->sync);
    success = fwrite(arm->mem.nvm, 1, FLASH_SIZE, image) == FLASH_SIZE;
    sync_leave(&arm->sync);
    return success;
}

bool arm_save_state(arm_t *arm, FILE *image) {
    bool success;
    sync_enter(&arm->sync);
    success = fwrite(&arm->cpu, sizeof(arm->cpu), 1, image) == 1 &&
              fwrite(arm->mem.ram, 1, HMCRAMC0_SIZE, image) == HMCRAMC0_SIZE &&
              fwrite(arm->mem.pb, sizeof(arm->mem.pb), 1, image) == 1 &&
              fwrite(arm->mem.aux, sizeof(arm->mem.aux), 1, image) == 1 &&
              fwrite(&arm->mem.pm, sizeof(arm->mem.pm), 1, image) == 1 &&
              fwrite(&arm->mem.gclk, sizeof(arm->mem.gclk), 1, image) == 1 &&
              fwrite(&arm->mem.nvmctrl, sizeof(arm->mem.nvmctrl), 1, image) == 1 &&
              fwrite(arm->mem.sercom, sizeof(arm->mem.sercom), 1, image) == 1;
    sync_leave(&arm->sync);
    return success;
}

bool arm_restore_flash(arm_t *arm, FILE *image) {
    bool success;
    sync_enter(&arm->sync);
    success = fread(arm->mem.nvm, 1, FLASH_SIZE, image) == FLASH_SIZE;
    sync_leave(&arm->sync);
    return success;
}

bool arm_restore_state(arm_t *arm, FILE *image) {
    bool success;
    sync_enter(&arm->sync);
    success = fread(&arm->cpu, sizeof(arm->cpu), 1, image) == 1 &&
              fread(arm->mem.ram, 1, HMCRAMC0_SIZE, image) == HMCRAMC0_SIZE &&
              fread(arm->mem.pb, sizeof(arm->mem.pb), 1, image) == 1 &&
              fread(arm->mem.aux, sizeof(arm->mem.aux), 1, image) == 1 &&
              fread(&arm->mem.pm, sizeof(arm->mem.pm), 1, image) == 1 &&
              fread(&arm->mem.gclk, sizeof(arm->mem.gclk), 1, image) == 1 &&
              fread(&arm->mem.nvmctrl, sizeof(arm->mem.nvmctrl), 1, image) == 1 &&
              fread(arm->mem.sercom, sizeof(arm->mem.sercom), 1, image) == 1;
    if (success) {
        spsc_queue_clear(&arm->usart[0]);
        spsc_queue_clear(&arm->usart[1]);
        sync_wake(&arm->sync);
    }
    sync_leave(&arm->sync);
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
    if (sync_idle(&arm->sync)) {
        sync_leave(&arm->sync);
    } else {
        sync_run_leave(&arm->sync);
    }
}

static void debug_byte(bool dir, unsigned char c) {
    (void)dir;
    (void)c;
    //printf("\x1b[%dm%02X\x1b[m", 94 + dir, c);
    //fflush(stdout);
}

static void debug_char(bool dir, char c) {
    (void)dir;
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
    if (sync_idle(&arm->sync)) {
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
    if (sync_idle(&arm->sync)) {
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

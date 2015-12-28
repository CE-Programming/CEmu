#ifdef __EMSCRIPTEN__
#include <emscripten.h>
#endif

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#include "core/emu.h"
#include "core/schedule.h"
#include "core/asic.h"
#include "core/os/os.h"

#include <QtCore/QThread>

const char *rom_image = NULL;

/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

int throttle_delay = 10; /* in milliseconds */

uint32_t cpu_events;

bool do_translate = true;
bool turbo_mode = false;

volatile bool exiting, debug_on_start, debug_on_warn;

const char log_type_tbl[] = LOG_TYPE_TBL;
int log_enabled[MAX_LOG];
FILE *log_file[MAX_LOG];
void logprintf(int type, const char *str, ...) {
    if (log_enabled[type]) {
        va_list va;
        va_start(va, str);
        vfprintf(log_file[type], str, va);
        va_end(va);
    }
}

void emuprintf(const char *format, ...) {
    va_list va;
    va_start(va, format);
    gui_console_vprintf(format, va);
    va_end(va);
}

void error(const char *fmt, ...) {
    va_list va;
    va_start(va, fmt);
    gui_console_printf("Error (%06X): ", asic.cpu->registers.PC);
    gui_console_vprintf(fmt, va);
    gui_console_printf("\n");
    va_end(va);
    //debugger(DBG_EXCEPTION, 0);
    cpu_events |= EVENT_RESET;
}

void throttle_interval_event(int index) {
    event_repeat(index, 27000000 / 60);

    static int intervals = 0, prev_intervals = 0;
    intervals += 1;

    // Calculate speed
    auto interval_end = std::chrono::high_resolution_clock::now();
    static auto prev = interval_end;
    static double speed = 1.0;
    auto time = std::chrono::duration_cast<std::chrono::microseconds>(interval_end - prev).count();
    if (time >= 500000) {
        speed = (double)10000 * (intervals - prev_intervals) / time;
        prev_intervals = intervals;
        prev = interval_end;
    }

    gui_do_stuff(true);

    if (speed > 0.7) {
        throttle_timer_wait();
    }
}

bool emu_start() {
    long lSize;

    asic_init(TI84PCE);

    if (rom_image == NULL) {
        gui_console_printf("No ROM image specified.");
        return false;
    } else {
        FILE *rom = fopen_utf8(rom_image, "rb");
        if (!rom) {
            gui_console_printf("Error opening ROM image.\n", rom_image);
            emu_cleanup();
            return false;
        }

        // get rom file size
        fseek(rom , 0L , SEEK_END);
        lSize=ftell(rom);
        rewind(rom);

        fread(asic.mem->flash.block, 1, lSize, rom);

        fclose(rom);
    }

    return true;
}

static void emu_reset() {
    cpu_events &= EVENT_DEBUG_STEP;

    sched_reset();

    sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
    sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

    asic_reset();

    /* Drain everything */
    cycle_count_delta = 0;

    sched_update_next_event(0);
}

#ifdef __EMSCRIPTEN__
void emu_inner_loop(void)
{
  while (!exiting) {
      sched_process_pending_events();
      if (cpu_events & EVENT_RESET) {
          gui_console_printf("CPU Reset triggered...");
          emu_reset();
      }
      if (cycle_count_delta < 0) {
          cpu_execute();  // execute instructions with available clock cycles
  }
}
#endif

void emu_loop(bool reset) {

    if (reset) {
        emu_reset();
    }

    exiting = false;

#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop(emu_inner_loop, -1, 1);
#else
    while (!exiting) {
        sched_process_pending_events();
        if (cpu_events & EVENT_RESET) {
            gui_console_printf("CPU Reset triggered...");
            emu_reset();
        }
        if (cycle_count_delta < 0) {
            cpu_execute();  // execute instructions with available clock cycles
        } else {
            QThread::yieldCurrentThread();
        }
    }
#endif
}

void emu_cleanup(void) {
    asic_free();
}

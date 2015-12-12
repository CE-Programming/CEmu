#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cctype>

#include "core/emu.h"
#include "core/schedule.h"
#include "core/asic.h"

const char *rom_image = NULL;

// For the debugger
uint32_t reg_array[0x0E];

/* cycle_count_delta is a (usually negative) number telling what the time is relative
 * to the next scheduled event. See sched.c */
int cycle_count_delta = 0;

int throttle_delay = 10; /* in milliseconds */

uint32_t cpu_events;

bool turbo_mode = false;

volatile bool exiting, is_running, debug_on_start, debug_on_warn;

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

static void mem_load_vram(const char *path) {
  FILE *romfile;
  size_t index;
  uint8_t byte_read;

  romfile=fopen(path, "rb");

  for(index=0;index<(320*240*2);index++)
  {
      fread(&byte_read,1,1,romfile);
      mem.ram[index+0x40000] = byte_read;
  }
  fclose(romfile);
}

void throttle_interval_event(int index)
{
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
        //gui_show_speed(speed);
        prev_intervals = intervals;
        prev = interval_end;
    }

    gui_do_stuff(true);

    if (!turbo_mode && speed > 0.7)
        throttle_timer_wait();
}

bool emu_start()
{
    long lSize;

    throttle_timer_on();

    log_enabled[LOG_CPU] = 1;
    log_file[LOG_CPU] = fopen("C:/LOG_CPU", "w");
    asic_init(TI84pCE);

    if (rom_image == NULL) {
        gui_console_printf("No ROM image specified.");
         return false;
    } else {
         FILE *rom = fopen(rom_image, "rb");
         if (!rom) {
            gui_console_printf("Error opening ROM image.\n", rom_image);
            emu_cleanup();
            return false;
         }

         // get rom file size
         fseek(rom , 0L , SEEK_END);
         lSize=ftell(rom);
         rewind(rom);

         fread(asic.mem->flash, 1, lSize, rom);

         fclose(rom);
    }

    mem_load_vram(rom_image);

    return true;
}

void emu_loop(bool reset)
{
    if(reset)
    {
    reset:

        cpu_events &= EVENT_DEBUG_STEP;

        sched_reset();

        sched.items[SCHED_THROTTLE].clock = CLOCK_27M;
        sched.items[SCHED_THROTTLE].proc = throttle_interval_event;

        asic_reset();
    }

    sched_update_next_event(0);

    exiting = false;

    while (!exiting) {
        sched_process_pending_events();
        while (!exiting && cycle_count_delta < 0) {
            if (cpu_events & EVENT_RESET) {
                gui_console_printf("CPU Reset triggered...");
                goto reset;
            }
            cpu_execute();  // execute instructions with available clock cycles
        }
    }
}

void emu_cleanup(void)
{

    asic_free();
}

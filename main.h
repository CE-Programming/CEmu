#ifndef MAIN_H
#define MAIN_H

#include "mainwindow.h"
#include "cemusettings.h"
#include "debugwindow.h"
#include "optionswindow.h"

typedef struct {
    ti_device_type device;
    asic_state_t *device_asic;
    char *rom_file;
    int cycles;
    int print_state;
    int no_rom_check;
} context_t;

context_t create_context(void);

void mem_load_vram(const char *path);

#endif // MAIN_H


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <core/emu.h>
#include <core/runloop.h>

#include <stdint.h>

// Settings
volatile bool exiting;

const char *rom_image = NULL;

static void mem_load_vram(const char *path) {
  FILE *romfile;
  size_t index;
  uint8_t byte_read;

  romfile=fopen(path, "rb");

  for(index=0;index<(320*240*2);index++)
  {
      fread(&byte_read,1,1,romfile);
      mem.vram[index] = byte_read;
  }
  fclose(romfile);
}

int emulate(void)
{
    int num_cycles_execute = -7;
    size_t lSize;

    asic_init(TI84pCE);

    if (rom_image == NULL) {
           gui_debug_printf("No ROM image specified.");
           return 1;
        } else {
           FILE *rom = fopen(rom_image, "rb");
           if (!rom) {
              gui_debug_printf("Error opening '%s'.\n", rom_image);
              asic_free();
              return 1;
           }

           // get rom file size
           fseek(rom , 0L , SEEK_END);
           lSize=ftell(rom);
           rewind(rom);

           fread(asic.mem->flash, 1, lSize, rom);

           fclose(rom);
        }

        mem_load_vram(rom_image);
        if (num_cycles_execute < 0) { // Run indefinitely
            for(;;) {
                runloop_tick();
                if (asic.stopped) {
                    break;
                }
//                nanosleep((const struct timespec[]){{0, 16666667L}}, NULL);
            }
        } else {
                runloop_tick_cycles(num_cycles_execute);
        }

    asic_free();
    return 0;
}

void emu_cleanup(void)
{

}

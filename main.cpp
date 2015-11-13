#include "mainwindow.h"
#include "cemusettings.h"
#include "debugwindow.h"
#include "optionswindow.h"

#include <QApplication>
#include <QStandardPaths>
#include <QSettings>
#include <QDir>
#include <iostream>
#include <cstdlib>
#include <ctime>
#include <climits>

#include "core/runloop.h"

typedef struct {
	ti_device_type device;
	asic_state_t *device_asic;
	char *rom_file;
	int cycles;
	int print_state;
	int no_rom_check;
} context_t;

context_t emucontext;

context_t create_context(void) {
	context_t context;
	context.device = TI84pCE;
	context.rom_file = NULL;
	context.cycles = -1;
	context.print_state = 0;
	context.no_rom_check = 0;
	return context;
}

void mem_load_vram(char *path) {
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

int main(int argc, char *argv[])
{
    QApplication z(argc, argv);

    MainWindow EmuWin;
    RomSelection a;
    a.exec();

    size_t lSize;

    QString str = CEmuSettings::Instance()->getROMLocation();
    QByteArray strarry = str.toLatin1();
    char *rom_file = strarry.data();

    std::cout << rom_file << std::endl;

    emucontext = create_context();
    asic_init(emucontext.device);
    emucontext.device_asic = &asic;
    emucontext.rom_file = rom_file;
    mem_load_vram(rom_file);

    EmuWin.show();

    if (emucontext.rom_file == NULL) {
           printf("No ROM image specified. Exiting...");
           return 1;
        } else {
           FILE *rom = fopen(emucontext.rom_file, "rb");
           if (!rom) {
              printf("Error opening '%s'.\n", emucontext.rom_file);
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

        //port_write_byte(0x0000,0xFF);
        std::cout<<std::hex<<(int)port_read_byte(0xE014)<<std::endl;

        if (emucontext.cycles < 0) { // Run indefinitely
            for(;;) {
                runloop_tick();
                if (asic.stopped) {
                    break;
                }
                QApplication::processEvents();
                //nanosleep((const struct timespec[]){{0, 16666667L}}, NULL);
            }
        } else {
                runloop_tick_cycles(emucontext.cycles);
        }

    return z.exec();
}

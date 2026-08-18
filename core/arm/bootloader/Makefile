CROSS ?= arm-none-eabi-
CC := $(CROSS)gcc
OBJCOPY := $(CROSS)objcopy
SIZE := $(CROSS)size
XXD ?= xxd
SED ?= sed

CFLAGS := -mcpu=cortex-m0plus -mthumb -Os -std=c11 -ffreestanding -fno-builtin
CFLAGS += -ffunction-sections -fdata-sections -Wall -Wextra -Werror
LDFLAGS := -nostdlib -Wl,--gc-sections -Wl,-Tlinker.ld -Wl,-Map,free_bootloader.map

all: ../free_bootloader_image.h

free_bootloader.elf: startup.c bootloader.c linker.ld
	$(CC) $(CFLAGS) $(LDFLAGS) startup.c bootloader.c -lgcc -o $@
	$(SIZE) $@

free_bootloader.bin: free_bootloader.elf
	$(OBJCOPY) -O binary $< $@

../free_bootloader_image.h: free_bootloader.bin
	$(XXD) -i -n cemu_free_bootloader $< | $(SED) \
		-e 's/^unsigned char/static const unsigned char/' \
		-e '1s/^/\/\* Generated from bootloader\/free_bootloader.bin; do not edit manually. \*\/\n/' \
		-e '/^unsigned int/d' > $@

clean:
	rm -f free_bootloader.elf free_bootloader.bin free_bootloader.map

distclean: clean
	rm -f ../free_bootloader_image.h

.PHONY: all clean distclean

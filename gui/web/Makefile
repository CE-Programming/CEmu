# Quick and dirty Makefile for an Emscripten build

CC := emcc
CFLAGS := -O3 -std=c11 -Wall --llvm-lto 1 --closure 1 -I..
#CFLAGS := -O3 -std=c11 -Wall -g3 --profiling -I..
SOURCES := ../../core/*.c ../../core/os/os-linux.c test.c
OUTPUT := cemu_web

corejs:
	$(CC) $(CFLAGS) $(SOURCES) -o $(OUTPUT)_c.bc
	$(CC) $(CFLAGS) *.bc -o $(OUTPUT).js --preload-file 84pce_51.rom

clean:
	rm -rf *.bc $(OUTPUT).js $(OUTPUT).data $(OUTPUT).js.mem $(OUTPUT)_*.bc

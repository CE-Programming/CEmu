# Quick and dirty Makefile for an Emscripten build

EMCC := emcc
EMCPP := em++
CFLAGS := -O3 -std=c11 -Wall -I..
CPPFLAGS := -O3 -std=c++14 -Wall -I..
SOURCES_C := ../../core/*.c ../../core/os/os-linux.c
SOURCES_CPP := test.cpp 
OUTPUT := cemu_web

corejs:
	$(EMCC) $(CFLAGS) $(SOURCES_C) -o $(OUTPUT)_c.bc
	$(EMCPP) $(CPPFLAGS) $(SOURCES_CPP) -o $(OUTPUT)_cpp.bc
	$(EMCC) -O3 *.bc -o $(OUTPUT).js -s TOTAL_MEMORY=50000000 --preload-file 84pce_51.rom

clean:
	rm -rf *.bc $(OUTPUT).js $(OUTPUT).data $(OUTPUT).js.mem $(OUTPUT)_*.bc

# Quick and dirty Makefile for an Emscripten build

EMCC := emcc
EMCPP := em++
CFLAGS := -O2 -std=c11 -Wall -I..
CPPFLAGS := -O2 -std=c++14 -Wall -I..
SOURCES_C := ../core/*.c
SOURCES_CPP := test.cpp ../core/*.cpp
OUTPUT := cemu_core

corejs:
	$(EMCC) $(CFLAGS) $(SOURCES_C) -o $(OUTPUT)_c.bc
	$(EMCPP) $(CPPFLAGS) $(SOURCES_CPP) -o $(OUTPUT)_cpp.bc
	$(EMCC) -O2 *.bc -o $(OUTPUT).html -s TOTAL_MEMORY=50000000 -s EXPORTED_FUNCTIONS="['_main', '_getScreenshot']" --preload-file 84pce_51.rom

clean:
	rm -rf *.bc $(OUTPUT).html $(OUTPUT).js $(OUTPUT).data $(OUTPUT).html.mem

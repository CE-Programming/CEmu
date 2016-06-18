CC := emcc
CXX := em++

# Add -g3 and disable some opts if needed
FLAGS := -O3 --llvm-lto 3 -Wall -I.. --closure 1 -s DISABLE_EXCEPTION_CATCHING=1 -s ASSERTIONS=0
CFLAGS := -std=c11 $(FLAGS)
CXXFLAGS := -std=c++11 $(FLAGS)

CSOURCES := $(wildcard ../../core/*.c) ../../core/os/os-linux.c main_emscripten.c
CPPSOURCES := ../../tests/autotester/autotester.cpp

OBJS = $(patsubst %.c, %.bc, $(CSOURCES))
OBJS += $(patsubst %.cpp, %.bc, $(CPPSOURCES))

OUTPUT := cemu_web

all: $(OUTPUT).js

%.bc: %.c Makefile
	$(CC) $(CFLAGS) $< -o $@

%.bc: %.cpp Makefile
	$(CXX) $(CXXFLAGS) $< -o $@

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

clean:
	rm -f $(OBJS) $(OUTPUT).js $(OUTPUT).data $(OUTPUT).js.mem

CC      = emcc

# Add -g3 and disable some opts if needed
CFLAGS  = -W -Wall -O3

# For console printing, software commands etc.
CFLAGS += -DDEBUG_SUPPORT

# Emscripten stuff
CFLAGS += -s TOTAL_MEMORY=33554432 --llvm-lto 3 -s INVOKE_RUN=0 -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['callMain', 'ccall', 'cwrap']"

# You may want to try with closure 1
asmjs:  CFLAGS += --closure 0 -s WASM=0
wasmfb: CFLAGS += --closure 0 -s WASM=1 -s "BINARYEN_METHOD='native-wasm,asmjs'"
wasm:   CFLAGS += --closure 0 -s WASM=1

CSOURCES := $(wildcard *.c) $(wildcard ./usb/*.c) ./debug/debug.c ./os/os-emscripten.c

OBJS = $(patsubst %.c, %.bc, $(CSOURCES))

OUTPUT := cemu_web

asmjs:  $(OUTPUT).js
wasmfb: $(OUTPUT).js
wasm:   $(OUTPUT).js

all: asmjs

%.bc: %.c
	$(CC) $(CFLAGS) $< -o $@

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

clean:
	$(RM) -f $(OBJS) $(OUTPUT).js* $(OUTPUT).data $(OUTPUT).asm.js $(OUTPUT).was*

.PHONY: all clean asmjs wasm wasmfb

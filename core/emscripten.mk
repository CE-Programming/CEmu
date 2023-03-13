CC      = emcc

# Add -g3 and disable some opts if needed
CFLAGS  = -W -Wall -O3 -flto

# For console printing, software commands etc.
CFLAGS += -DDEBUG_SUPPORT

# Emscripten stuff
EMFLAGS := -s TOTAL_MEMORY=33554432 --memory-init-file 0 -s WASM=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME="'WebCEmu'" -s INVOKE_RUN=0 -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0 -s "EXTRA_EXPORTED_RUNTIME_METHODS=['callMain', 'ccall', 'cwrap']"

LFLAGS := -flto $(EMFLAGS)

CSOURCES := $(wildcard *.c) $(wildcard ./usb/*.c) ./debug/debug.c ./os/os-emscripten.c

OBJS = $(patsubst %.c, %.bc, $(CSOURCES))

OUTPUT := WebCEmu

wasm:  $(OUTPUT).js

all: wasm

%.bc: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

clean:
	$(RM) -f $(OBJS) $(OUTPUT).js* $(OUTPUT).was*

.PHONY: all clean wasm

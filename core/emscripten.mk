CC      = emcc

# Add -g3 and disable some opts if needed
CFLAGS  = -W -Wall -O3 -flto

# For console printing, software commands etc.
CFLAGS += -DDEBUG_SUPPORT

# Emscripten stuff
EMFLAGS := -s TOTAL_MEMORY=33554432 -s ALLOW_MEMORY_GROWTH=0 --memory-init-file 0 -s WASM=1 -s EXPORT_ES6=1 -s MODULARIZE=1 -s EXPORT_NAME="'WebCEmu'" -s INVOKE_RUN=0 -s NO_EXIT_RUNTIME=1 -s ASSERTIONS=0
EMFLAGS += -s "EXPORTED_RUNTIME_METHODS=['FS', 'callMain', 'ccall', 'cwrap']"
EMFLAGS += -s "EXPORTED_FUNCTIONS=['_main', '_malloc', '_free', '_emu_keypad_event', '_lcd_get_frame', '_emu_reset', '_emu_exit', '_emu_load', '_emu_save', '_set_file_to_send', '_emsc_set_main_loop_timing', '_emsc_pause_main_loop', '_emsc_resume_main_loop', '_emsc_cancel_main_loop', '_get_device_type', '_get_asic_revision', '_get_asic_python', '_emu_send_variable', '_sendCSC', '_sendKey', '_sendLetterKeyPress']"

LFLAGS := -flto $(EMFLAGS)

CSOURCES := $(wildcard *.c) $(wildcard ./usb/*.c) ./debug/debug.c ./os/os-emscripten.c

OBJS = $(patsubst %.c, %.o, $(CSOURCES))

OUTPUT := WebCEmu

wasm:  $(OUTPUT).js

all: wasm

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

$(OUTPUT).js: $(OBJS)
	$(CC) $(CFLAGS) $(LFLAGS) $^ -o $@

clean:
	$(RM) -f $(OBJS) $(OUTPUT).js* $(OUTPUT).was*

.PHONY: all clean wasm

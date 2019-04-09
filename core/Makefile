CC = gcc

# If you want debug/symbol info, add -gX and remove -OX as needed
# If you want core debug support, add -DDEBUG_SUPPORT
# If you want the emulator to run on a different thread than the gui, add -DMULTITHREAD
CFLAGS = -Wall -Wextra -fPIC -O3 -static

# Add debugging support, with zdis disassembler
# Note that this will require a compatible c11 compiler
#CFLAGS += -DDEBUG_SUPPORT

# Add these flags if your compiler supports it
#CFLAGS += -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error

## Build rules, you shouldn't need to edit anything below this line

CFLAGS += -std=gnu11

OBJS  = $(patsubst %.c,   %.o, $(shell find . -name \*.c))

STATICLIB = libcemucore.a

all: lib

lib: $(STATICLIB)

$(STATICLIB): $(OBJS)
	ar rcs $@ $?

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(STATICLIB)

.PHONY: clean all lib

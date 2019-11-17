CC = gcc
AR = ar
RM = rm -f

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

ifneq (,$(findstring DEBUG_SUPPORT,$(CFLAGS)))
    CFLAGS += -std=gnu11
else
    CFLAGS += -std=c89
endif

OBJS  = $(patsubst %.c,   %.o, $(shell find . -name usb -prune -or -name \*.c -print))

STATICLIB = libcemucore.a

all: lib

lib: $(STATICLIB)

$(STATICLIB): $(OBJS)
	$(AR) rcs $@ $?

%.o: %.c
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJS) $(STATICLIB)

.PHONY: clean all lib

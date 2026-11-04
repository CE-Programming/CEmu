CC = gcc
AR = ar
RM = rm -f

# If you want debug/symbol info, add -gX and remove -OX as needed
# Set DEBUG_SUPPORT=1 to enable core debugging (requires the zdis submodule).
# If you want the emulator to run on a different thread than the gui, add -DMULTITHREAD
CFLAGS = -Wall -Wextra -fPIC -O3 -std=gnu11 -static

DEBUG_SUPPORT ?= 0
ifeq ($(DEBUG_SUPPORT),1)
override CPPFLAGS += -DDEBUG_SUPPORT
endif

# Print what programs write to the debug console (dbg_printf) through gui_console_printf /
# gui_console_err_printf without debugging support too (cemu-headless, the autotester)
#CFLAGS += -DDEBUG_CONSOLE_SUPPORT

# Add these flags if your compiler supports it
#CFLAGS += -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error

## Build rules, you shouldn't need to edit anything below this line

# source: http://blog.jgc.org/2011/07/gnu-make-recursive-wildcard-function.html
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2)$(filter $(subst *,%,$2),$d))

OBJS = $(patsubst %.c,%.o,$(filter-out arm/%,$(call rwildcard,,*.c)))

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

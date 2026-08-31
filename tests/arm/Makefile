appname := arm-tests
debug_appname := arm-debug-tests

CC ?= cc

CPPFLAGS += -I../../core/arm/samd21a/include -I../../core/arm/CMSIS/Core/Include
CFLAGS ?= -O2
CFLAGS += -std=gnu11 -Wall -Wextra -DNDEBUG
LDLIBS += -pthread

sources := arm_tests.c \
	../../core/coproc.c \
	../../core/arm/arm.c \
	../../core/arm/armcpu.c \
	../../core/arm/armmem.c \
	../../core/arm/spscqueue.c \
	../../core/arm/sync.c \
	../../core/arm/tinycthread.c \
	../../core/os/os-linux.c

all: $(appname)

$(appname): $(sources)
	$(CC) $(CPPFLAGS) $(CFLAGS) $(LDFLAGS) -o $@ $(sources) $(LDLIBS)

$(debug_appname): $(sources)
	$(CC) $(CPPFLAGS) -DDEBUG_SUPPORT -DCOPROC_DEBUG_SUPPORT $(CFLAGS) $(LDFLAGS) -o $@ $(sources) $(LDLIBS)

test: $(appname) $(debug_appname)
	./$(appname)
	./$(debug_appname)

clean:
	rm -f $(appname) $(debug_appname)

.PHONY: all test clean

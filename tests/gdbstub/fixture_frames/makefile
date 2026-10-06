NAME = GDBFRAME
DESCRIPTION = IX IY and frameless GDB regression
COMPRESSED = NO
ARCHIVED = NO
HAS_PRINTF = NO
HAS_LIBC = NO
HAS_LIBCXX = NO
CFLAGS = -Wall -Wextra -O0
include $(shell cedev-config --makefile)

# Keep the caller's locals visible; compile the leaf functions with the
# optimizer that selects an unsaved IY frame and register-held variables.
$(OBJDIR)/src/leaves.c.s: CC_DEBUG = -DDEBUG=1 -O2 -gdwarf-5 -g3 -fomit-frame-pointer

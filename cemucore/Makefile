define newline


endef
reverse = $(if $(wordlist 2,2,$1),$(call reverse,$(wordlist 2,$(words $1),$1)) $(firstword $1),$1)

ifeq ($(OS),Windows_NT)
MKDIR         = ( md $1 2>nul || call )
REMOVE        = ( del /f /q $1 2>nul || call )
REMOVE_DIR    = ( rd /q $1 2>nul || call )
WHERE_IS      = where 2>nul

SHLIB_EXT     = .dll
OBJECT_EXT    = .obj
else
MKDIR         = mkdir -p $1
REMOVE        = rm -f $1
REMOVE_DIR    = ( rmdir $1 2>/dev/null || true )
WHERE_IS      = which 2>/dev/null

ifeq ($(shell uname -s),Darwin)
SHLIB_EXT     = .dylib
else
SHLIB_EXT     = .so
endif
OBJECT_EXT    = .o
endif

STATICLIB_EXT = .a

FIRST_EXE     = $(or $(firstword $(shell $(WHERE_IS) $1)),$(lastword $1))

CC            = $(call FIRST_EXE,$(addprefix $(CROSS_COMPILE),clang gcc cc))
ALL_CFLAGS    = -Wall -Wextra -Wimplicit-fallthrough -pedantic -D_GNU_SOURCE -fPIC
ALL_CFLAGS   += -std=c11 -fvisibility=internal -Wno-gnu-zero-variadic-macro-arguments
ifneq ($(NOTHREADS),)
ALL_CFLAGS   += -DCEMUCORE_NOTHREADS
else
ALL_CFLAGS   += -pthread
ALL_LFLAGS   += -pthread
endif
ALL_CFLAGS   += $(CFLAGS) $(CLFLAGS)
ALL_LFLAGS   += $(LFLAGS) $(CLFLAGS)
AR            = $(call FIRST_EXE,$(subst clang,llvm,$(notdir $(CC)))-ar $(CROSS_COMPILE)ar) cqs

BUILD = $(CURDIR)

ifndef STATICLIB
# Default to shared
SHLIB = 1
endif

SHLIB_DIR = shared
STATICLIB_DIR = static

HEADERS += \
	cemucore.h \
	compiler.h \
	core.h \
	cpu.h \
	debug.h \
	memory.h \
	os.h \
	scheduler.h \
	sync.h \
	usb/usb.h

SOURCES += \
	core.c \
	cpu.c \
	debug.c \
	memory.c \
	os.c \
	scheduler.c \
	sync.c

TARGET = libcemucore
$(eval $(foreach S,SH STATIC, \
	$SLIB_TARGET = $$(BUILD)/$($SLIB_DIR)/$$(TARGET)$($SLIB_EXT)$(newline) \
	$SLIB_OBJECTS = $(addprefix $(BUILD)/$($SLIB_DIR)/,$(addsuffix $(OBJECT_EXT),$(SOURCES)))$(newline)))
TARGETS = $(foreach S,SH STATIC,$(if $($SLIB),$($SLIB_TARGET)))
OBJECTS = $(foreach S,SH STATIC,$(if $($SLIB),$($SLIB_OBJECTS)))

ARTIFACTS = $(OBJECTS) $(TARGETS)
ARTIFACT_DIRS = $(sort $(dir $(ARTIFACTS)))

all: $(TARGETS)

$(SHLIB_TARGET): $(SHLIB_OBJECTS) | $(ARTIFACT_DIRS)
	$(CC) $(ALL_LFLAGS) -shared -o $@ $^

$(STATICLIB_TARGET): $(STATICLIB_OBJECTS) | $(ARTIFACT_DIRS)
	$(call REMOVE,$@)
	$(AR) $@ $^

$(BUILD)/$(SHLIB_DIR)/%.o: % $(HEADERS) | $(ARTIFACT_DIRS)
	$(CC) $(ALL_CFLAGS) -DCEMUCORE_EXPORT -c -o $@ $<

$(BUILD)/$(STATICLIB_DIR)/%.o: % $(HEADERS) | $(ARTIFACT_DIRS)
	$(CC) $(ALL_CFLAGS) -DCEMUCORE_STATIC -c -o $@ $<

$(ARTIFACT_DIRS):
	$(call MKDIR,$@)

clean:
	$(call REMOVE,$(call reverse,$(OBJECTS)))
	$(call REMOVE_DIR,$(call reverse,$(ARTIFACT_DIRS)))

distclean:
	$(call REMOVE,$(call reverse,$(ARTIFACTS)))
	$(call REMOVE_DIR,$(call reverse,$(ARTIFACT_DIRS)))

.PHONY: all clean distclean

.EXTRA_PREREQS = Makefile

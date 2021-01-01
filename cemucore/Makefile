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

CC            = $(or $(firstword $(shell $(WHERE_IS) $(addprefix $(CROSS_COMPILE),clang gcc cc))),$(CROSS_COMPILE)cc)
ALL_CFLAGS    = -std=c11 -Wall -Wextra -pedantic
ifneq ($(NOTHREADS),)
ALL_CFLAGS   += -DCEMUCORE_NOTHREADS
else
ALL_CFLAGS   += -pthread
ALL_LDFLAGS  += -pthread
endif
ALL_CFLAGS   += $(CFLAGS) $(CLDFLAGS)
ALL_LDFLAGS  += $(LDFLAGS) $(CLDFLAGS)
AR_C          = $(CROSS_COMPILE)ar cqs

BUILD = $(CURDIR)

ifndef STATICLIB
# Default to shared
SHLIB = 1
endif

TARGET = libcemucore
$(eval $(foreach S,SH STATIC,$SLIB_TARGET = $$(BUILD)/$$(TARGET)$($SLIB_EXT)$(newline)))
TARGETS = $(foreach S,SH STATIC,$(if $($SLIB),$($SLIB_TARGET)))

HEADERS += \
	cemucore.h \
	core.h

SOURCES += \
	core.c

OBJECTS = $(addprefix $(BUILD)/,$(addsuffix $(OBJECT_EXT),$(SOURCES)))

ARTIFACTS = $(OBJECTS) $(TARGETS)
ARTIFACT_DIRS = $(sort $(dir $(ARTIFACTS)))

all: $(TARGETS)

$(SHLIB_TARGET): $(OBJECTS) | $(ARTIFACT_DIRS)
	$(CC) $(ALL_LDFLAGS) -shared -o $@ $^

$(STATICLIB_TARGET): $(OBJECTS) | $(ARTIFACT_DIRS)
	$(call REMOVE,$@)
	$(AR_C) $@ $^

$(BUILD)/%.o: % $(HEADERS) | $(ARTIFACT_DIRS)
	$(CC) $(ALL_CFLAGS) -c -o $@ $<

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

define newline


endef
reverse = $(if $(wordlist 2,2,$1),$(call reverse,$(wordlist 2,$(words $1),$1)) $(firstword $1),$1)

ifeq ($(OS),Windows_NT)
MKDIR          = ( md $1 2>nul || call )
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

SHLIB_EXT     = .so
OBJECT_EXT    = .o
endif

CC            = $(or $(firstword $(shell $(WHERE_IS) clang gcc cc)),cc)

STATICLIB_EXT = .a

BUILD = $(CURDIR)

ifndef STATICLIB
# Default to shared
SHLIB = 1
endif

TARGET = libcemucore
$(eval $(foreach S,SH STATIC,$SLIB_TARGET = $$(BUILD)/$$(TARGET)$($SLIB_EXT)$(newline)))
TARGETS = $(foreach S,SH STATIC,$(if $($SLIB),$($SLIB_TARGET)))

HEADERS += \
	cemucore.h

SOURCES += \
	cemucore.c

OBJECTS = $(addprefix $(BUILD)/,$(addsuffix $(OBJECT_EXT),$(SOURCES)))

ARTIFACTS = $(OBJECTS) $(TARGETS)
ARTIFACT_DIRS = $(sort $(dir $(ARTIFACTS)))

all: $(TARGETS)

$(SHLIB_TARGET): $(OBJECTS) | $(ARTIFACT_DIRS)
	$(CC) $(LDFLAGS) -shared -o $@ $^

$(STATICLIB_TARGET): $(OBJECTS) | $(ARTIFACT_DIRS)
	$(call REMOVE,$@)
	$(AR) qc $@ $^

$(BUILD)/%.o: % $(HEADERS) | $(ARTIFACT_DIRS)
	$(CC) $(CFLAGS) -c -o $@ $<

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

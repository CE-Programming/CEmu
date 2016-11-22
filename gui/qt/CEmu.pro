lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5 to build CEmu!")

# CEmu version
if (0) { # GitHub release/deployment build. Has to correspond to the git tag.
    DEFINES += CEMU_VERSION=\\\"1.0\\\"
} else { # Development build. Used in the about screen
    GIT_VERSION = $$system(git describe --abbrev=7 --dirty --always --tags)
    DEFINES += CEMU_VERSION=\\\"0.9dev_$$GIT_VERSION\\\"
}

# Continuous Integration (variable checked later)
CI = $$(CI)

# Code beautifying
DISTFILES += ../../.astylerc

QT += core gui widgets network

TARGET = CEmu
TEMPLATE = app

# Localization
TRANSLATIONS += i18n/fr_FR.ts i18n/es_ES.ts

CONFIG += c++11 console

# Core options
DEFINES += DEBUG_SUPPORT

CONFIG(release, debug|release) {
    #This is a release build
    DEFINES += QT_NO_DEBUG_OUTPUT
} else {
    #This is a debug build
    GLOBAL_FLAGS += -g3
}

# GCC/clang flags
if (!win32-msvc*) {
    GLOBAL_FLAGS    += -W -Wall -Wextra -Wunused-function -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -Winit-self
    GLOBAL_FLAGS    += -ffunction-sections -fdata-sections -fno-strict-overflow
    QMAKE_CFLAGS    += -std=gnu11
    QMAKE_CXXFLAGS  += -fno-exceptions
    isEmpty(CI) {
        # Only enable opts for non-CI release builds
        # -flto might cause an internal compiler error on GCC in some circumstances (with -g3?)... Comment it if needed.
        CONFIG(release, debug|release): GLOBAL_FLAGS += -O3 -flto
    }
} else {
    # TODO: add equivalent flags
    # Example for -Werror=shadow: /weC4456 /weC4457 /weC4458 /weC4459
    #     Source: https://connect.microsoft.com/VisualStudio/feedback/details/1355600/
    QMAKE_CXXFLAGS  += /Wall
}

if (macx|linux) {
    # Be more secure by default...
    GLOBAL_FLAGS    += -fPIE -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1
    # Use ASAN on debug builds. Watch out about ODR crashes when built with -flto. detect_odr_violation=0 as an env var may help.
    CONFIG(debug, debug|release): GLOBAL_FLAGS += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -O0
}

macx:  QMAKE_LFLAGS += -Wl,-dead_strip
linux: QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie

QMAKE_CFLAGS    += $$GLOBAL_FLAGS
QMAKE_CXXFLAGS  += $$GLOBAL_FLAGS
QMAKE_LFLAGS    += $$GLOBAL_FLAGS

macx: ICON = resources/icons/icon.icns

SOURCES +=  utils.cpp \
    main.cpp \
    mainwindow.cpp \
    romselection.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    emuthread.cpp \
    datawidget.cpp \
    dockwidget.cpp \
    lcdpopout.cpp \
    searchwidget.cpp \
    basiccodeviewerwindow.cpp \
    keypad/qtkeypadbridge.cpp \
    keypad/keymap.cpp \
    keypad/keypadwidget.cpp \
    keypad/rectkey.cpp \
    keypad/arrowkey.cpp \
    qhexedit/chunks.cpp \
    qhexedit/commands.cpp \
    qhexedit/qhexedit.cpp \
    capture/gif.cpp \
    tivarslib/utils_tivarslib.cpp \
    tivarslib/TypeHandlers/DummyHandler.cpp \
    tivarslib/TypeHandlers/TH_0x00.cpp \
    tivarslib/TypeHandlers/TH_0x01.cpp \
    tivarslib/TypeHandlers/TH_0x02.cpp \
    tivarslib/TypeHandlers/TH_0x05.cpp \
    tivarslib/TypeHandlers/TH_0x0C.cpp \
    tivarslib/TypeHandlers/TH_0x0D.cpp \
    tivarslib/TypeHandlers/TH_0x1B.cpp \
    tivarslib/TypeHandlers/TH_0x1C.cpp \
    tivarslib/TypeHandlers/TH_0x1D.cpp \
    tivarslib/TypeHandlers/TH_0x1E.cpp \
    tivarslib/TypeHandlers/TH_0x1F.cpp \
    tivarslib/TypeHandlers/TH_0x20.cpp \
    tivarslib/TypeHandlers/TH_0x21.cpp \
    ../../tests/autotester/autotester.cpp \
    ../../core/asic.c \
    ../../core/cpu.c \
    ../../core/keypad.c \
    ../../core/lcd.c \
    ../../core/registers.c \
    ../../core/port.c \
    ../../core/interrupt.c \
    ../../core/flash.c \
    ../../core/misc.c \
    ../../core/schedule.c \
    ../../core/timers.c \
    ../../core/usb.c \
    ../../core/sha256.c \
    ../../core/realclock.c \
    ../../core/backlight.c \
    ../../core/cert.c \
    ../../core/control.c \
    ../../core/mem.c \
    ../../core/link.c \
    ../../core/vat.c \
    ../../core/emu.c \
    ../../core/extras.c \
    ../../core/debug/disasm.cpp \
    ../../core/debug/debug.c \
    ../../core/debug/profiler.c \
    sendinghandler.cpp \
    capture/optimize.c \
    capture/opttemplate.c \
    capture/gifread.c \
    capture/gifwrite.c \
    capture/quantize.c \
    capture/giffunc.c \
    capture/xform.c \
    debugger.cpp \
    hexeditor.cpp \
    settings.cpp \
    ../../core/debug/stepping.cpp

linux|macx: SOURCES += ../../core/os/os-linux.c
win32: SOURCES += ../../core/os/os-win32.c win32-console.cpp

HEADERS  +=  utils.h \
    mainwindow.h \
    romselection.h \
    qtframebuffer.h \
    lcdwidget.h \
    emuthread.h \
    datawidget.h \
    dockwidget.h \
    lcdpopout.h \
    searchwidget.h \
    basiccodeviewerwindow.h \
    keypad/qtkeypadbridge.h \
    keypad/keymap.h \
    keypad/keypadwidget.h \
    keypad/key.h \
    keypad/keyconfig.h \
    keypad/rectkey.h \
    keypad/graphkey.h \
    keypad/secondkey.h \
    keypad/alphakey.h \
    keypad/otherkey.h \
    keypad/numkey.h \
    keypad/operkey.h \
    keypad/arrowkey.h \
    qhexedit/chunks.h \
    qhexedit/commands.h \
    qhexedit/qhexedit.h \
    capture/gif.h \
    capture/giflib.h \
    tivarslib/autoloader.h \
    tivarslib/utils_tivarslib.h \
    tivarslib/TypeHandlers/TypeHandlerFuncGetter.h \
    tivarslib/TypeHandlers/TypeHandlers.h \
    ../../tests/autotester/autotester.h \
    ../../core/asic.h \
    ../../core/cpu.h \
    ../../core/defines.h \
    ../../core/keypad.h \
    ../../core/lcd.h \
    ../../core/registers.h \
    ../../core/tidevices.h \
    ../../core/port.h \
    ../../core/interrupt.h \
    ../../core/emu.h \
    ../../core/flash.h \
    ../../core/misc.h \
    ../../core/schedule.h \
    ../../core/timers.h \
    ../../core/usb.h \
    ../../core/sha256.h \
    ../../core/realclock.h \
    ../../core/backlight.h \
    ../../core/cert.h \
    ../../core/control.h \
    ../../core/mem.h \
    ../../core/link.h \
    ../../core/vat.h \
    ../../core/extras.h \
    ../../core/os/os.h \
    ../../core/debug/debug.h \
    ../../core/debug/disasm.h \
    ../../core/debug/stepping.h \
    ../../core/debug/profiler.h \
    cemuopts.h \
    sendinghandler.h \
    capture/kcolor.h \
    capture/gifsicle.h \
    capture/lcdf/clp.h \
    capture/lcdf/inttypes.h \
    capture/lcdfgif/gif.h \
    capture/lcdfgif/gifx.h \
    keypad/keycode.h \
    debugger.h

FORMS    += mainwindow.ui \
    romselection.ui \
    lcdpopout.ui \
    searchwidget.ui \
    basiccodeviewerwindow.ui

RESOURCES += \
    resources.qrc

RC_ICONS += resources\icons\icon.ico

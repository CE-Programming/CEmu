lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5 to build CEmu!")

# Version
DEFINES += CEMU_VERSION=0.2

QT += core gui quick widgets quickwidgets

TARGET = CEmu
TEMPLATE = app

# Localization
TRANSLATIONS += i18n/fr_FR.ts

CONFIG += c++11

# Only add Clang/GCC flags if we're not using MSVC
if (!win32-msvc*) {
    GLOBAL_FLAGS += -W -Wall -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -Winit-self
    GLOBAL_FLAGS += -ffunction-sections -fdata-sections -fno-strict-overflow
    GLOBAL_FLAGS_RELEASE += -O3 -flto
    # Note: On OS X at least, -flto seems to trigger odr-violations that ASAN doesn't like.
}

macx|linux: GLOBAL_FLAGS += -fPIE -Wstack-protector \
                            -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -fstack-protector-all --param=ssp-buffer-size=1

macx:  QMAKE_LFLAGS += -Wl,-dead_strip
linux: QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie

QMAKE_CFLAGS += $$GLOBAL_FLAGS
QMAKE_LFLAGS += $$GLOBAL_FLAGS

QMAKE_CFLAGS_RELEASE += $$GLOBAL_FLAGS_RELEASE
QMAKE_CXXFLAGS_RELEASE += $$GLOBAL_FLAGS_RELEASE
QMAKE_LFLAGS_RELEASE += $$GLOBAL_FLAGS_RELEASE

if (!win32-msvc*) {
    # Clang/GCC specific flags
    QMAKE_CXXFLAGS += $$GLOBAL_FLAGS -fno-exceptions
} else {
    # MSVC specific flags
    # TODO: add equivalent -Werror flags
    # For -Werror=shadow: /weC4456 /weC4457 /weC4458 /weC4459
    #     Source: https://connect.microsoft.com/VisualStudio/feedback/details/1355600/
    QMAKE_CXXFLAGS += /Wall $$GLOBAL_FLAGS
}

ios {
    DEFINES += IS_IOS_BUILD
    QMAKE_INFO_PLIST = Info.plist
}

macx: ICON = resources/icons/icon.icns

SOURCES +=  utils.cpp \
    main.cpp\
    mainwindow.cpp \
    romselection.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    emuthread.cpp \
    qtkeypadbridge.cpp \
    qmlbridge.cpp \
    disasmwidget.cpp \
    core/asic.c \
    core/cpu.c \
    core/keypad.c \
    core/lcd.c \
    core/registers.c \
    core/apb.c \
    core/interrupt.c \
    core/flash.c \
    core/misc.c \
    core/schedule.c \
    core/emu.cpp \
    core/timers.c \
    core/usb.c \
    core/sha256.c \
    core/realclock.c \
    core/backlight.c \
    core/cert.c \
    core/control.c \
    core/mem.c \
    core/link.c \
    core/vat.c \
    core/capture/gif.cpp \
    core/debug/disasm.cpp \
    core/debug/debug.c \
    qhexedit/chunks.cpp \
    qhexedit/commands.cpp \
    qhexedit/qhexedit.cpp

linux|macx|ios: SOURCES += os/os-linux.c
win32: SOURCES += os/os-win32.c

HEADERS  +=  os/os.h \
    utils.h \
    mainwindow.h \
    romselection.h \
    qtframebuffer.h \
    lcdwidget.h \
    emuthread.h \
    disasmwidget.h \
    qtkeypadbridge.h \
    qmlbridge.h \
    keymap.h \
    core/asic.h \
    core/cpu.h \
    core/defines.h \
    core/keypad.h \
    core/lcd.h \
    core/registers.h \
    core/tidevices.h \
    core/apb.h \
    core/interrupt.h \
    core/emu.h \
    core/flash.h \
    core/misc.h \
    core/schedule.h \
    core/timers.h \
    core/usb.h \
    core/sha256.h \
    core/realclock.h \
    core/backlight.h \
    core/cert.h \
    core/control.h \
    core/mem.h \
    core/link.h \
    core/vat.h \
    core/capture/gif.h \
    core/capture/giflib.h \
    core/debug/debug.h \
    core/debug/disasm.h \
    core/debug/disasmc.h \
    qhexedit/chunks.h \
    qhexedit/commands.h \
    qhexedit/qhexedit.h

FORMS    += mainwindow.ui \
    romselection.ui

RESOURCES += \
    resources.qrc

RC_ICONS += resources\icons\icon.ico

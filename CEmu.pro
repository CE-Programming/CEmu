lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5 to build CEmu!")

# Version
DEFINES += CEMU_VERSION=0.2

QT += core gui quick widgets quickwidgets

TARGET = CEmu
TEMPLATE = app

# Localization
TRANSLATIONS += i18n/fr_FR.ts

CONFIG += c++11

GLOBAL_FLAGS = -W -Wall -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -fno-strict-overflow -Winit-self -ffunction-sections -fdata-sections

if (macx | linux) {
    GLOBAL_FLAGS += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -fstack-protector-all -Wstack-protector --param=ssp-buffer-size=1 -fPIC
}
if (macx) {
    MORE_LFLAGS += -Wl,-dead_strip
}
if (linux) {
    MORE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie
}

QMAKE_CFLAGS += $$GLOBAL_FLAGS
QMAKE_CXXFLAGS += $$GLOBAL_FLAGS -fno-exceptions
QMAKE_LFLAGS += -flto -fPIE $$GLOBAL_FLAGS $$MORE_LFLAGS

ios {
    DEFINES += IS_IOS_BUILD
    QMAKE_INFO_PLIST = Info.plist
}

macx: ICON = resources/icons/icon.icns


SOURCES += main.cpp\
    mainwindow.cpp \
    romselection.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    core/asic.c \
    core/backlightport.c \
    core/controlport.c \
    core/cpu.c \
    core/keypad.c \
    core/lcd.c \
    core/memory.c \
    core/registers.c \
    core/apb.c \
    core/interrupt.c \
    emuthread.cpp \
    settings.cpp \
    core/flash.c \
    core/misc.c \
    core/schedule.c \
    core/emu.cpp \
    core/debug.c \
    qtkeypadbridge.cpp \
    qmlbridge.cpp \
    core/timers.c \
    core/usb.c \
    core/sha256.c \
    core/realclock.c \
    core/gif.cpp

HEADERS  += mainwindow.h \
    romselection.h \
    qtframebuffer.h \
    lcdwidget.h \
    core/asic.h \
    core/backlightport.h \
    core/controlport.h \
    core/cpu.h \
    core/defines.h \
    core/keypad.h \
    core/lcd.h \
    core/memory.h \
    core/registers.h \
    core/tidevices.h \
    core/apb.h \
    core/interrupt.h \
    emuthread.h \
    core/emu.h \
    settings.h \
    core/flash.h \
    core/misc.h \
    core/schedule.h \
    core/debug.h \
    keymap.h \
    qtkeypadbridge.h \
    qmlbridge.h \
    core/timers.h \
    core/usb.h \
    core/sha256.h \
    core/realclock.h \
    core/giflib.h \
    core/gif.h

FORMS    += mainwindow.ui \
    romselection.ui

RESOURCES += \
    resources.qrc

RC_ICONS += resources\icons\icon.ico

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CEmu
TEMPLATE = app

# Localization
TRANSLATIONS += i18n/fr_FR.ts

CONFIG += c++11 c11

GLOBAL_FLAGS = -W -Wall -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -fno-strict-overflow -Winit-self -ffunction-sections -fdata-sections

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

SOURCES += main.cpp\
    mainwindow.cpp \
    romselection.cpp \
    aboutwindow.cpp \
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
    core/debug.c

HEADERS  += mainwindow.h \
    romselection.h \
    aboutwindow.h \
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
    core/debug.h

FORMS    += mainwindow.ui \
    romselection.ui \
    aboutwindow.ui

RESOURCES += \
    resources.qrc

RC_ICONS += icon.ico

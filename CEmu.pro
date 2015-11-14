QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CEmu
TEMPLATE = app

CONFIG += c++11 c11

GLOBAL_FLAGS = -W -Wall -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-nonliteral -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -fno-strict-overflow -Winit-self --param=ssp-buffer-size=1 -ffunction-sections -fdata-sections

QMAKE_CFLAGS += $$GLOBAL_FLAGS -fstack-protector-all -Wstack-protector -fPIC

QMAKE_CXXFLAGS += $$GLOBAL_FLAGS -fno-exceptions -fstack-protector-all -Wstack-protector -fPIC

QMAKE_LFLAGS += -flto -fPIE -fstack-protector-all -Wstack-protector -fPIC

if (macx | linux) {
    QMAKE_CFLAGS   += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error
    QMAKE_CXXFLAGS += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error
    QMAKE_LFLAGS   += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error
}
if (macx) {
    QMAKE_LFLAGS += -Wl,-dead_strip
}
if (linux) {
    QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie
}

SOURCES += main.cpp\
    mainwindow.cpp \
    romselection.cpp \
    aboutwindow.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    core/asic.c \
    core/backlightport.c \
    core/context.c \
    core/controlport.c \
    core/cpu.c \
    core/keypad.c \
    core/lcd.c \
    core/memory.c \
    core/registers.c \
    core/runloop.c \
    core/apb.c \
    core/exxx.c \
    core/dxxx.c \
    core/cxxx.c \
    core/fxxx.c \
    core/interrupt.c \
    emuthread.cpp \
    core/emu.c \
    settings.cpp \
    core/flash.c

HEADERS  += mainwindow.h \
    romselection.h \
    aboutwindow.h \
    qtframebuffer.h \
    lcdwidget.h \
    core/asic.h \
    core/backlightport.h \
    core/context.h \
    core/controlport.h \
    core/cpu.h \
    core/defines.h \
    core/keypad.h \
    core/lcd.h \
    core/memory.h \
    core/registers.h \
    core/runloop.h \
    core/tidevices.h \
    core/apb.h \
    core/exxx.h \
    core/dxxx.h \
    core/cxxx.h \
    core/fxxx.h \
    core/interrupt.h \
    emuthread.h \
    core/emu.h \
    settings.h \
    core/flash.h

FORMS    += mainwindow.ui \
    romselection.ui \
    aboutwindow.ui

RESOURCES += \
    resources.qrc

RC_ICONS += icon.ico

QT += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = CEmu
TEMPLATE = app

CONFIG += c++11 c11

# Warnings, optimizations and hardening...

GLOBAL_FLAGS = -W -Wall -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-nonliteral -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=date-time -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -fno-strict-overflow -Winit-self -fstack-protector-all -Wstack-protector --param=ssp-buffer-size=1 -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -ffunction-sections -fdata-sections -fPIC

QMAKE_CFLAGS += $$GLOBAL_FLAGS

QMAKE_CXXFLAGS += $$GLOBAL_FLAGS -fno-exceptions

QMAKE_LFLAGS += -flto -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -fPIC -fPIE

if (macx) {
    QMAKE_LFLAGS += -Wl,-dead_strip
} else {
    QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie
}


SOURCES += main.cpp\
    mainwindow.cpp \
    cemusettings.cpp \
    optionswindow.cpp \
    romselection.cpp \
    aboutwindow.cpp \
    debugwindow.cpp \
    qtframebuffer.cpp \
    lcdwidget.cpp \
    core/asic.c \
    core/backlightport.c \
    core/context.c \
    core/controlport.c \
    core/cpu.c \
    core/flashport.c \
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
    core/interrupt.c

HEADERS  += mainwindow.h \
    cemusettings.h \
    optionswindow.h \
    romselection.h \
    aboutwindow.h \
    debugwindow.h \
    qtframebuffer.h \
    lcdwidget.h \
    core/asic.h \
    core/backlightport.h \
    core/context.h \
    core/controlport.h \
    core/cpu.h \
    core/defines.h \
    core/flashport.h \
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
    main.h

FORMS    += mainwindow.ui \
    optionswindow.ui \
    romselection.ui \
    aboutwindow.ui \
    debugwindow.ui

RESOURCES +=

RC_ICONS += icon.ico

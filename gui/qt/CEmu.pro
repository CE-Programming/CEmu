lessThan(QT_MAJOR_VERSION, 6) {
    lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5.15 to build CEmu!")
    lessThan(QT_MINOR_VERSION, 10) : error("You need at least Qt 5.10 to build CEmu!")
}

QT += core gui widgets network

TARGET = CEmu
TEMPLATE = app

CONFIG += c++11 console
LIBS += -L$$_PRO_FILE_PWD_/deps/KDDockWidgets/build/lib -L$$_PRO_FILE_PWD_/deps/KDDockWidgets/build/lib64
LIBS += -lkddockwidgets

INCLUDEPATH += $$_PRO_FILE_PWD_/deps/KDDockWidgets/build/include
DEPENDPATH += $$_PRO_FILE_PWD_/deps/KDDockWidgets/build/include

# Core options
DEFINES += DEBUG_SUPPORT

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
} else {
    GLOBAL_FLAGS += -g3
}

if (!win32-msvc*) {
    GLOBAL_FLAGS += -W -Wall -Wextra -Wunused-function -Wwrite-strings -Wredundant-decls -Wformat -Wformat-security -Wreturn-type -Wpointer-arith -Winit-self
    GLOBAL_FLAGS += -ffunction-sections -fdata-sections -fno-strict-overflow
    QMAKE_CFLAGS += -std=gnu11
} else {
    QMAKE_CXXFLAGS  += /Wall /wd5045
    QMAKE_CXXFLAGS += /MP
}

if (macx|linux) {
    GLOBAL_FLAGS += -fPIE -Wstack-protector -fstack-protector-strong --param=ssp-buffer-size=1
    CONFIG(debug, debug|release): GLOBAL_FLAGS += -fsanitize=address,bounds -fsanitize-undefined-trap-on-error -O0
}

macx:  QMAKE_LFLAGS += -Wl,-dead_strip
linux: QMAKE_LFLAGS += -Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,--gc-sections -pie

QMAKE_CFLAGS    += $$GLOBAL_FLAGS
QMAKE_CXXFLAGS  += $$GLOBAL_FLAGS
QMAKE_LFLAGS    += $$GLOBAL_FLAGS

if(macx) {
    QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12
    LIBS += -framework Cocoa
}

SOURCES += \
    ../../core/asic.c \
    ../../core/bus.c \
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
    ../../core/usb/disconnected.c \
    ../../core/usb/dusb.c \
    ../../core/usb/usb.c \
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
    ../../core/spi.c \
    ../../core/debug/debug.c \
    ../../core/debug/zdis/zdis.c \
    calculatorwidget.cpp \
    consolewidget.cpp \
    corewindow.cpp \
    dockwidget.cpp \
    emuthread.cpp \
    keyhistorywidget.cpp \
    keypad/arrowkey.cpp \
    keypad/keymap.cpp \
    keypad/keypadwidget.cpp \
    keypad/qtkeypadbridge.cpp \
    keypad/rectkey.cpp \
    main.cpp \
    screenwidget.cpp \
    settings.cpp \
    statewidget.cpp

linux|macx: SOURCES += ../../core/os/os-linux.c
win32: SOURCES += ../../core/os/os-win32.c win32-console.cpp
win32: LIBS += -lpsapi

macx: SOURCES += os/mac/kdmactouchbar.mm
macx: HEADERS += os/mac/kdmactouchbar.h os/mac/kdmactouchbar_global.h

HEADERS  += \
    ../../core/asic.h \
    ../../core/cpu.h \
    ../../core/atomics.h \
    ../../core/defines.h \
    ../../core/keypad.h \
    ../../core/lcd.h \
    ../../core/registers.h \
    ../../core/port.h \
    ../../core/interrupt.h \
    ../../core/emu.h \
    ../../core/flash.h \
    ../../core/misc.h \
    ../../core/schedule.h \
    ../../core/timers.h \
    ../../core/usb/device.h \
    ../../core/usb/fotg210.h \
    ../../core/usb/usb.h \
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
    ../../core/spi.h \
    ../../core/debug/debug.h \
    ../../core/debug/zdis/zdis.h \
    calculatorwidget.h \
    consolewidget.h \
    corewindow.h \
    dockwidget.h \
    emuthread.h \
    keyhistorywidget.h \
    keypad/alphakey.h \
    keypad/arrowkey.h \
    keypad/graphkey.h \
    keypad/key.h \
    keypad/keycode.h \
    keypad/keyconfig.h \
    keypad/keymap.h \
    keypad/keypadwidget.h \
    keypad/numkey.h \
    keypad/operkey.h \
    keypad/otherkey.h \
    keypad/qtkeypadbridge.h \
    keypad/rectkey.h \
    keypad/secondkey.h \
    overlaywidget.h \
    screenwidget.h \
    settings.h \
    statewidget.h

DISTFILES +=

RESOURCES += \
    resources.qrc \
    resources.qrc

lessThan(QT_MAJOR_VERSION, 6) : if (lessThan(QT_MAJOR_VERSION, 5) | lessThan(QT_MINOR_VERSION, 10) : error("You need at least Qt 5.10 to build CEmu!"))

QT += core gui widgets network KDDockWidgets

TARGET = CEmu
TEMPLATE = app

CONFIG += c++11 console

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
    ../../core/backlight.c \
    ../../core/bus.c \
    ../../core/cert.c \
    ../../core/control.c \
    ../../core/cpu.c \
    ../../core/debug/debug.c \
    ../../core/debug/zdis/zdis.c \
    ../../core/emu.c \
    ../../core/extras.c \
    ../../core/flash.c \
    ../../core/interrupt.c \
    ../../core/keypad.c \
    ../../core/lcd.c \
    ../../core/link.c \
    ../../core/mem.c \
    ../../core/misc.c \
    ../../core/port.c \
    ../../core/realclock.c \
    ../../core/registers.c \
    ../../core/schedule.c \
    ../../core/sha256.c \
    ../../core/spi.c \
    ../../core/timers.c \
    ../../core/usb/disconnected.c \
    ../../core/usb/dusb.c \
    ../../core/usb/usb.c \
    ../../core/vat.c \
    calculatorwidget.cpp \
    capturewidget.cpp \
    consolewidget.cpp \
    corewindow.cpp \
    developer/autotesterwidget.cpp \
    developer/clockswidget.cpp \
    developer/controlwidget.cpp \
    developer/cpuwidget.cpp \
    developer/devmiscwidget.cpp \
    developer/disassemblywidget.cpp \
    developer/flashramwidget.cpp \
    developer/memorywidget.cpp \
    developer/osstackswidget.cpp \
    developer/osvarwidget.cpp \
    developer/performancewidget.cpp \
    developer/portmonitorwidget.cpp \
    developer/visualizerwidget.cpp \
    developer/watchpointswidget.cpp \
    developer/widgets/disasmwidget.cpp \
    developer/widgets/hexwidget.cpp \
    developer/widgets/highlighteditwidget.cpp \
    developer/widgets/visualizerlcdwidget.cpp \
    dockwidget.cpp \
    emuthread.cpp \
    keyhistorywidget.cpp \
    keypad/arrowkey.cpp \
    keypad/keymap.cpp \
    keypad/keypadwidget.cpp \
    keypad/qtkeypadbridge.cpp \
    keypad/rectkey.cpp \
    main.cpp \
    overlaywidget.cpp \
    romdialog.cpp \
    screenwidget.cpp \
    settings.cpp \
    settingsdialog.cpp \
    statewidget.cpp \
    util.cpp \
    variablewidget.cpp

linux|macx: SOURCES += ../../core/os/os-linux.c
win32: SOURCES += ../../core/os/os-win32.c
win32: LIBS += -lpsapi

HEADERS  += \
    ../../core/asic.h \
    ../../core/atomics.h \
    ../../core/backlight.h \
    ../../core/cert.h \
    ../../core/control.h \
    ../../core/cpu.h \
    ../../core/debug/debug.h \
    ../../core/debug/zdis/zdis.h \
    ../../core/defines.h \
    ../../core/emu.h \
    ../../core/extras.h \
    ../../core/flash.h \
    ../../core/interrupt.h \
    ../../core/keypad.h \
    ../../core/lcd.h \
    ../../core/link.h \
    ../../core/mem.h \
    ../../core/misc.h \
    ../../core/os/os.h \
    ../../core/port.h \
    ../../core/realclock.h \
    ../../core/registers.h \
    ../../core/schedule.h \
    ../../core/sha256.h \
    ../../core/spi.h \
    ../../core/timers.h \
    ../../core/usb/device.h \
    ../../core/usb/fotg210.h \
    ../../core/usb/usb.h \
    ../../core/vat.h \
    calculatorwidget.h \
    capturewidget.h \
    consolewidget.h \
    corewindow.h \
    developer/autotesterwidget.h \
    developer/clockswidget.h \
    developer/controlwidget.h \
    developer/cpuwidget.h \
    developer/devmiscwidget.h \
    developer/devwidget.h \
    developer/disassemblywidget.h \
    developer/flashramwidget.h \
    developer/memorywidget.h \
    developer/osstackswidget.h \
    developer/osvarswidget.h \
    developer/performancewidget.h \
    developer/portmonitorwidget.h \
    developer/visualizerwidget.h \
    developer/watchpointswidget.h \
    developer/widgets/disasmwidget.h \
    developer/widgets/hexwidget.h \
    developer/widgets/highlighteditwidget.h \
    developer/widgets/visualizerlcdwidget.h \
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
    romdialog.h \
    screenwidget.h \
    settings.h \
    settingsdialog.h \
    statewidget.h \
    util.h \
    variablewidget.h

DISTFILES +=

RESOURCES += \
    resources.qrc

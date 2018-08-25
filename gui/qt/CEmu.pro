lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5.5 to build CEmu!")
lessThan(QT_MINOR_VERSION, 5) : error("You need at least Qt 5.5 to build CEmu!")

# Error if git submodules are not downloaded
!exists("../../core/debug/zdis/zdis.c"): error("You have to run 'git submodule init' and 'git submodule update' first.")

# CEmu version and info
CEMU_RELEASE = true
CEMU_GIT_SHA = $$system(git describe --abbrev=7 --always)
isEmpty(CEMU_VERSION) {
    CEMU_VERSION = 1.1dev
    CEMU_RELEASE = false
}
DEFINES += CEMU_VERSION=\\\"$$CEMU_VERSION\\\"
DEFINES += CEMU_GIT_SHA=\\\"$$CEMU_GIT_SHA\\\"
DEFINES += CEMU_RELEASE=$$CEMU_RELEASE
DEFINES += MULTITHREAD

# Continuous Integration (variable checked later)
CI = $$(CI)

# Code beautifying
DISTFILES += ../../.astylerc

# Linux desktop files
if (linux) {
    isEmpty(PREFIX) {
        PREFIX = /usr
    }
    target.path = $$PREFIX/bin
    desktop.target = desktop
    desktop.path = $$PREFIX/share/applications
    desktop.commands += command -v xdg-desktop-menu && \(xdg-desktop-menu install --novendor --mode system resources/linux/cemu.desktop &&
    desktop.commands += xdg-mime install --novendor --mode system resources/linux/cemu.xml &&
    desktop.commands += xdg-mime default cemu.desktop application/x-tice-rom &&
    desktop.commands += xdg-mime default cemu.desktop application/x-cemu-image &&
    desktop.commands += for length in 512 256 192 160 128 96 72 64 48 42 40 36 32 24 22 20 16; do xdg-icon-resource install --novendor --context apps --mode system --size \$\$length resources/icons/linux/cemu-\$\$\{length\}x\$\$length.png cemu; done\) || true
    desktop.uninstall += command -v xdg-desktop-menu && \(xdg-desktop-menu uninstall --novendor --mode system resources/linux/cemu.desktop &&
    desktop.uninstall += xdg-mime uninstall --novendor --mode system resources/linux/cemu.xml &&
    desktop.uninstall += for length in 512 256 192 160 128 96 72 64 48 42 40 36 32 24 22 20 16; do xdg-icon-resource uninstall --novendor --context apps --mode system --size \$\$length resources/icons/linux/cemu-\$\$\{length\}x\$\$length.png cemu; done\) || true
    INSTALLS += target desktop
}

QT += core concurrent gui widgets network

isEmpty(TARGET_NAME) {
    TARGET_NAME = CEmu
}
TARGET = $$TARGET_NAME
TEMPLATE = app

# Localization
TRANSLATIONS += i18n/fr_FR.ts i18n/es_ES.ts i18n/nl_NL.ts

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
    isEmpty(CI) {
        # Only enable opts for non-CI release builds
        # -flto might cause an internal compiler error on GCC in some circumstances (with -g3?)... Comment it if needed.
        CONFIG(release, debug|release): GLOBAL_FLAGS += -O3 -flto
    }

    CONFIG += link_pkgconfig
    PKGCONFIG += zlib libarchive
    # You should run ./capture/get_libpng-apng.sh first!
    isEmpty(USE_LIBPNG) {
        exists("$$PWD/capture/libpng-apng/.libs/libpng16.a") {
            message("Built libpng-apng detected, using it")
            USE_LIBPNG = internal
        } else {
            packagesExist(libpng) {
                USE_LIBPNG = system
            } else {
                error("You have to run $$PWD/capture/get_libpng-apng.sh first, or at least get libpng-dev(el)!")
            }
        }
    }
    equals(USE_LIBPNG, "system") {
        message("Warning: unless your system libpng has APNG support, you will not be able to record screen captures!")
        PKGCONFIG += libpng
    } else {
        !exists("$$PWD/capture/libpng-apng/.libs/libpng16.a"): error("You have to run $$PWD/capture/get_libpng-apng.sh first!")
        INCLUDEPATH += $$PWD/capture/libpng-apng
        LIBS += $$PWD/capture/libpng-apng/.libs/libpng16.a
    }
} else {
    # TODO: add equivalent flags
    # Example for -Werror=shadow: /weC4456 /weC4457 /weC4458 /weC4459
    #     Source: https://connect.microsoft.com/VisualStudio/feedback/details/1355600/
    QMAKE_CXXFLAGS  += /Wall

    # Note that libpng/zlib LIBS/INCLUDES should be specified in the envrionment.
    # We will use LIBPNG_APNG_LIB, ZLIB_LIB, and LIBPNG_APNG_INCLUDE.
    # The logic below accounts for both specifying in the real shell environment,
    # as well as directly on the command line (e.g. qmake VAR=1).
    isEmpty(LIBPNG_APNG_LIB) {
        LIBPNG_APNG_LIB = $$(LIBPNG_APNG_LIB)
        isEmpty(LIBPNG_APNG_LIB) {
            error("For MSVC builds, we require LIBPNG_APNG_LIB to point to the libpng-apng lib file to compile against. Please set this in your environment and try again.")
        }
    }
    isEmpty(ZLIB_LIB) {
        ZLIB_LIB = $$(ZLIB_LIB)
        isEmpty(ZLIB_LIB) {
            error("For MSVC builds, we require ZLIB_LIB to point to the zlib lib file to compile against. Please set this in your environment and try again.")
        }
    }
    isEmpty(LIBPNG_APNG_INCLUDE) {
        LIBPNG_APNG_INCLUDE = $$(LIBPNG_APNG_INCLUDE)
        isEmpty(LIBPNG_APNG_INCLUDE) {
            error("For MSVC builds, we require LIBPNG_APNG_INCLUDE to point to the libpng-apng include director to use for compiling. Please set this in your environment and try again.")
        }
    }

    LIBS += $$LIBPNG_APNG_LIB $$ZLIB_LIB
    INCLUDEPATH += $$LIBPNG_APNG_INCLUDE
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

# If we want to keep supporting macOS down to 10.9, add this flag and build/deploy with Qt 5.8
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

macx: ICON = resources/icons/icon.icns

SOURCES += \
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
    ../../core/spi.c \
    ../../core/debug/debug.c \
    ../../core/debug/zdis/zdis.c \
    QArchive.cc \
    ipc.cpp \
    main.cpp \
    utils.cpp \
    mainwindow.cpp \
    romselection.cpp \
    lcdwidget.cpp \
    emuthread.cpp \
    datawidget.cpp \
    dockwidget.cpp \
    searchwidget.cpp \
    basiccodeviewerwindow.cpp \
    sendinghandler.cpp \
    debugger.cpp \
    settings.cpp \
    keyhistory.cpp \
    capture/animated-png.c \
    keypad/qtkeypadbridge.cpp \
    keypad/keymap.cpp \
    keypad/keypadwidget.cpp \
    keypad/rectkey.cpp \
    keypad/arrowkey.cpp \
    debugger/hexwidget.cpp \
    debugger/disasm.cpp \
    tivarslib/tivarslib_utils.cpp \
    tivarslib/BinaryFile.cpp \
    tivarslib/TIVarFile.cpp \
    tivarslib/TIModel.cpp \
    tivarslib/TIModels.cpp \
    tivarslib/TIVarType.cpp \
    tivarslib/TIVarTypes.cpp \
    tivarslib/TypeHandlers/DummyHandler.cpp \
    tivarslib/TypeHandlers/TH_0x00.cpp \
    tivarslib/TypeHandlers/TH_0x01.cpp \
    tivarslib/TypeHandlers/TH_0x02.cpp \
    tivarslib/TypeHandlers/TH_0x05.cpp \
    tivarslib/TypeHandlers/TH_0x0C.cpp \
    tivarslib/TypeHandlers/TH_0x0D.cpp \
    tivarslib/TypeHandlers/TH_0x15.cpp \
    tivarslib/TypeHandlers/TH_0x1B.cpp \
    tivarslib/TypeHandlers/TH_0x1C.cpp \
    tivarslib/TypeHandlers/TH_0x1D.cpp \
    tivarslib/TypeHandlers/TH_0x1E.cpp \
    tivarslib/TypeHandlers/TH_0x1F.cpp \
    tivarslib/TypeHandlers/TH_0x20.cpp \
    tivarslib/TypeHandlers/TH_0x21.cpp \
    visualizerwidget.cpp \
    debugger/visualizerdisplaywidget.cpp \
    memorywidget.cpp

linux|macx: SOURCES += ../../core/os/os-linux.c
win32: SOURCES += ../../core/os/os-win32.c win32-console.cpp
win32: LIBS += -lpsapi

# This doesn't exist, but Qt Creator ignores that
just_show_up_in_qt_creator {
SOURCES +=  ../../tests/autotester/autotester_cli.cpp \
    ../../core/os/os-emscripten.c \
    ../../core/os/os-linux.c \
    ../../core/os/os-win32.c
}

HEADERS  += \
    ../../tests/autotester/autotester.h \
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
    ../../core/spi.h \
    ../../core/debug/debug.h \
    ../../core/debug/zdis/zdis.h \
    QArchive.hpp \
    ipc.h \
    utils.h \
    cemuopts.h \
    mainwindow.h \
    romselection.h \
    lcdwidget.h \
    emuthread.h \
    datawidget.h \
    dockwidget.h \
    searchwidget.h \
    basiccodeviewerwindow.h \
    sendinghandler.h \
    keyhistory.h \
    keypad/qtkeypadbridge.h \
    keypad/keymap.h \
    keypad/keypadwidget.h \
    keypad/key.h \
    keypad/keycode.h \
    keypad/keyconfig.h \
    keypad/rectkey.h \
    keypad/graphkey.h \
    keypad/secondkey.h \
    keypad/alphakey.h \
    keypad/otherkey.h \
    keypad/numkey.h \
    keypad/operkey.h \
    keypad/arrowkey.h \
    capture/animated-png.h \
    debugger/hexwidget.h \
    debugger/disasm.h \
    tivarslib/tivarslib_utils.h \
    tivarslib/CommonTypes.h \
    tivarslib/BinaryFile.h \
    tivarslib/TIModel.h \
    tivarslib/TIModels.h \
    tivarslib/TIVarFile.h \
    tivarslib/TIVarType.h \
    tivarslib/TIVarTypes.h \
    tivarslib/TypeHandlers/TypeHandlers.h \
    visualizerwidget.h \
    debugger/visualizerdisplaywidget.h

FORMS    += \
    mainwindow.ui \
    romselection.ui \
    searchwidget.ui \
    basiccodeviewerwindow.ui \
    keyhistory.ui

RESOURCES += \
    resources.qrc

RC_ICONS += resources\icons\icon.ico

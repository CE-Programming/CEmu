lessThan(QT_MAJOR_VERSION, 5) : error("You need at least Qt 5.5 to build CEmu!")
lessThan(QT_MINOR_VERSION, 5) : error("You need at least Qt 5.5 to build CEmu!")

# Error if git submodules are not downloaded
!exists("../../core/debug/zdis/zdis.c")|!exists("../../core/jit/asmjit"): error("You have to run 'git submodule init' and 'git submodule update' first.")

# CEmu version and info
CEMU_RELEASE = true
CEMU_GIT_SHA = $$system(git describe --abbrev=7 --always)
isEmpty(CEMU_VERSION) {
    CEMU_VERSION = v1.2dev
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

QT += core gui widgets network

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

!isEmpty(NO_JIT) {
} else:equals(QMAKE_HOST.arch, x86) {
    DEFINES += JIT_SUPPORT JIT_BACKEND_X86 ASMJIT_BUILD_X86
    HEADERS += ../../core/jit/x86jit.h
    SOURCES += ../../core/jit/x86jit.cpp
} else:equals(QMAKE_HOST.arch, x86_64) {
    DEFINES += JIT_SUPPORT JIT_BACKEND_X64 ASMJIT_BUILD_X86
    HEADERS += ../../core/jit/x64jit.h
    SOURCES += ../../core/jit/x64jit.cpp
} else:equals(QMAKE_HOST.arch, arm) {
    DEFINES += JIT_SUPPORT JIT_BACKEND_ARM ASMJIT_BUILD_ARM
    HEADERS += ../../core/jit/armjit.h
    SOURCES += ../../core/jit/armjit.cpp
} else:equals(QMAKE_HOST.arch, aarch64) {
    DEFINES += JIT_SUPPORT JIT_BACKEND_AARCH64 ASMJIT_BUILD_ARM
    HEADERS += ../../core/jit/aarch64jit.h
    SOURCES += ../../core/jit/aarch64jit.cpp
}
contains(DEFINES, ASMJIT_BUILD_...) {
    DEFINES += ASMJIT_BUILD_EMBED ASMJIT_DISABLE_BUILDER ASMJIT_DISABLE_COMPILER ASMJIT_DISABLE_LOGGING ASMJIT_DISABLE_TEXT ASMJIT_DISABLE_INST_API
    INCLUDEPATH += ../../core/jit/asmjit/src
    HEADERS += $$files(../../core/jit/asmjit/src/asmjit/*.h, true)
    SOURCES += $$files(../../core/jit/asmjit/src/asmjit/*.cpp, true)
    CONFIG(release, debug|release) {
        DEFINES += ASMJIT_BUILD_RELEASE
    } else {
        DEFINES += ASMJIT_BUILD_DEBUG
    }
    # Judy/optional is only used by jit for now
    INCLUDEPATH += ../../core/jit/optional/tl
    HEADERS += ../../core/jit/Judy++.h
    SOURCES += ../../core/jit/Judy++.cpp
    LIBS += -lJudy
}

# These options can be disabled / enabled depending on
# compiler / library support for your toolchain
DEFINES += LIB_ARCHIVE_SUPPORT PNG_SUPPORT GLOB_SUPPORT

CONFIG(release, debug|release) {
    #This is a release build
    DEFINES += QT_NO_DEBUG_OUTPUT
} else {
    #This is a debug build
    GLOBAL_FLAGS += -g3
}

# GCC/clang flags
if (!win32-msvc*) {
    GLOBAL_FLAGS    += -W -Wall -Wextra -Wunused-function -Werror=write-strings -Werror=redundant-decls -Werror=format -Werror=format-security -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=missing-prototypes -Werror=return-type -Werror=pointer-arith -Winit-self
    GLOBAL_FLAGS    += -ffunction-sections -fdata-sections -fno-strict-overflow
    QMAKE_CFLAGS    += -std=gnu11
    isEmpty(CI) {
        # Only enable opts for non-CI release builds
        # -flto might cause an internal compiler error on GCC in some circumstances (with -g3?)... Comment it if needed.
        CONFIG(release, debug|release): GLOBAL_FLAGS += -O3 -flto
    }

    if (contains(DEFINES, LIB_ARCHIVE_SUPPORT)) {
        CONFIG += link_pkgconfig
        PKGCONFIG += zlib libarchive
    }
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
    # /wd5045: disable C5045
    #          (new warning that causes errors: "Compiler will insert Spectre mitigation
    #          for memory load if /Qspectre switch specified")
    QMAKE_CXXFLAGS  += /Wall /wd5045

    # Add -MP to enable speedier builds
    QMAKE_CXXFLAGS += /MP

    # Do we have a flag specifying use of libpng-apng from vcpkg?
    equals(LIBPNG_APNG_FROM_VCPKG, 1) {
        warning("Enabled using libpng-apng from vcpkg. Note that if you do not have vcpkg integrated into MSVC, and/or do not have libpng-apng installed within vcpkg, the build will likely fail.")
        # This is a bad hack, but MOC kinda needs it to work correctly...
        QMAKE_MOC_OPTIONS += -DPNG_WRITE_APNG_SUPPORTED
    }

    # Otherwise...
    !equals(LIBPNG_APNG_FROM_VCPKG, 1) {
        # If we're not using vcpkg, we rely on manual variables to find needed
        # libpng-apng components.
        # 
        # Note that libpng/zlib LIBS/INCLUDES should be specified in the envrionment.
        # We will use LIBPNG_APNG_LIB, ZLIB_LIB, and LIBPNG_APNG_INCLUDE.
        # The logic below accounts for both specifying in the real shell environment,
        # as well as directly on the command line (e.g. qmake VAR=1).
        isEmpty(LIBPNG_APNG_LIB) {
            LIBPNG_APNG_LIB = $$(LIBPNG_APNG_LIB)
            isEmpty(LIBPNG_APNG_LIB) {
                error("For MSVC builds, we require LIBPNG_APNG_LIB to point to the libpng-apng lib file to compile against. Please set this in your environment and try again. Alternatively, set LIBPNG_APNG_FROM_VCPKG=1 if you use vcpkg and installed libpng-apng in it.")
            }
        }
        isEmpty(ZLIB_LIB) {
            ZLIB_LIB = $$(ZLIB_LIB)
            isEmpty(ZLIB_LIB) {
                error("For MSVC builds, we require ZLIB_LIB to point to the zlib lib file to compile against. Please set this in your environment and try again. Alternatively, set LIBPNG_APNG_FROM_VCPKG=1 if you use vcpkg and installed libpng-apng in it.")
            }
        }
        isEmpty(LIBPNG_APNG_INCLUDE) {
            LIBPNG_APNG_INCLUDE = $$(LIBPNG_APNG_INCLUDE)
            isEmpty(LIBPNG_APNG_INCLUDE) {
                error("For MSVC builds, we require LIBPNG_APNG_INCLUDE to point to the libpng-apng include director to use for compiling. Please set this in your environment and try again. Alternatively, set LIBPNG_APNG_FROM_VCPKG=1 if you use vcpkg and installed libpng-apng in it.")
            }
        }

        LIBS += $$LIBPNG_APNG_LIB $$ZLIB_LIB
        INCLUDEPATH += $$LIBPNG_APNG_INCLUDE
    }
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

# If we want to keep supporting macOS down to 10.9, change this and build/deploy with Qt 5.8
macx: QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.12

macx: ICON = resources/icons/icon.icns

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
    ../../core/usb.c \
    ../../core/vat.c \
    ../../tests/autotester/autotester.cpp \
    archive/extractor.c \
    basiccodeviewerwindow.cpp \
    capture/animated-png.c \
    datawidget.cpp \
    debugger.cpp \
    debugger/disasm.cpp \
    debugger/hexwidget.cpp \
    debugger/visualizerdisplaywidget.cpp \
    dockwidget.cpp \
    emuthread.cpp \
    ipc.cpp \
    keyhistorywidget.cpp \
    keypad/arrowkey.cpp \
    keypad/keymap.cpp \
    keypad/keypadwidget.cpp \
    keypad/qtkeypadbridge.cpp \
    keypad/rectkey.cpp \
    lcdwidget.cpp \
    main.cpp \
    mainwindow.cpp \
    memorywidget.cpp \
    romselection.cpp \
    searchwidget.cpp \
    sendinghandler.cpp \
    settings.cpp \
    tivarslib/BinaryFile.cpp \
    tivarslib/TIModel.cpp \
    tivarslib/TIModels.cpp \
    tivarslib/TIVarFile.cpp \
    tivarslib/TIVarType.cpp \
    tivarslib/TIVarTypes.cpp \
    tivarslib/TypeHandlers/DummyHandler.cpp \
    tivarslib/TypeHandlers/STH_DataAppVar.cpp \
    tivarslib/TypeHandlers/STH_ExactFraction.cpp \
    tivarslib/TypeHandlers/STH_ExactFractionPi.cpp \
    tivarslib/TypeHandlers/STH_ExactPi.cpp \
    tivarslib/TypeHandlers/STH_ExactRadical.cpp \
    tivarslib/TypeHandlers/STH_FP.cpp \
    tivarslib/TypeHandlers/STH_PythonAppVar.cpp \
    tivarslib/TypeHandlers/TH_GenericAppVar.cpp \
    tivarslib/TypeHandlers/TH_GenericComplex.cpp \
    tivarslib/TypeHandlers/TH_GenericList.cpp \
    tivarslib/TypeHandlers/TH_GenericReal.cpp \
    tivarslib/TypeHandlers/TH_Matrix.cpp \
    tivarslib/TypeHandlers/TH_TempEqu.cpp \
    tivarslib/TypeHandlers/TH_Tokenized.cpp \
    tivarslib/tivarslib_utils.cpp \
    utils.cpp \
    visualizerwidget.cpp

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
    ../../core/asic.h \
    ../../core/atomics.h \
    ../../core/backlight.h \
    ../../core/bus.h \
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
    ../../core/usb.h \
    ../../core/vat.h \
    ../../tests/autotester/autotester.h \
    archive/extractor.h \
    basiccodeviewerwindow.h \
    capture/animated-png.h \
    cemuopts.h \
    datawidget.h \
    debugger/disasm.h \
    debugger/hexwidget.h \
    debugger/visualizerdisplaywidget.h \
    dockwidget.h \
    emuthread.h \
    ipc.h \
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
    lcdwidget.h \
    mainwindow.h \
    romselection.h \
    searchwidget.h \
    sendinghandler.h \
    tivarslib/BinaryFile.h \
    tivarslib/CommonTypes.h \
    tivarslib/TIModel.h \
    tivarslib/TIModels.h \
    tivarslib/TIVarFile.h \
    tivarslib/TIVarType.h \
    tivarslib/TIVarTypes.h \
    tivarslib/TypeHandlers/TypeHandlers.h \
    tivarslib/tivarslib_utils.h \
    utils.h \
    visualizerwidget.h

FORMS    += \
    basiccodeviewerwindow.ui \
    mainwindow.ui \
    romselection.ui \
    searchwidget.ui

RESOURCES += \
    resources.qrc

RC_ICONS += resources\icons\icon.ico

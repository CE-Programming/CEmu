cmake_minimum_required(VERSION 3.15)

project(CEmu
        VERSION 2.0
        LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../core/debug/zdis/zdis.c" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tivars_lib_cpp/src/TIVarFile.cpp")
    message(FATAL_ERROR "Some files seem to be missing - you have to run 'git submodule init' and 'git submodule update' first.")
endif()

option(TRY_STATIC_LIBS "Build using as many static libs as possible" OFF)
if (VCPKG_TARGET_TRIPLET MATCHES "-static(-md)?$")
    if(NOT TRY_STATIC_LIBS)
        message(WARNING "Setting TRY_STATIC_LIBS to ON anyway due to static vcpkg target")
        set(TRY_STATIC_LIBS ON)
    endif()
endif()

if(TRY_STATIC_LIBS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/.cmake/try_static_libs.cmake")
    message("Will try to build as statically as possible...")
    if(WIN32)
        set(VCPKG_LIBRARY_LINKAGE static)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        if (NOT VCPKG_TARGET_TRIPLET MATCHES "-static-md$")
            message("Using static MSVC runtime...")
            set(VCPKG_CRT_LINKAGE static)
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(USED_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING "Expose CMAKE_GENERATOR" FORCE)
message(STATUS "Detected system: ${CMAKE_SYSTEM_NAME} - host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR}")

# C11 and C++14
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Sane flags
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # sane defaults + hardening
    set(GLOBAL_COMPILE_FLAGS "-W -Wall -Wextra -Wno-unused-parameter -Werror=shadow -Werror=write-strings -Wredundant-decls -Werror=date-time -Werror=return-type -Werror=pointer-arith")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GLOBAL_COMPILE_FLAGS} -Werror=declaration-after-statement -Werror=implicit-function-declaration -Werror=missing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GLOBAL_COMPILE_FLAGS}")
    # useful flags for debugging
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
endif()

include(GNUInstallDirs)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Gui Network Widgets)

if(APPLE)
    set(MAYBE_MACOSX_BUNDLE MACOSX_BUNDLE)
endif()

qt_add_executable(CEmu ${MAYBE_MACOSX_BUNDLE}
    ../../core/asic.c ../../core/asic.h
    ../../core/atomics.h
    ../../core/backlight.c ../../core/backlight.h
    ../../core/bus.c ../../core/bus.h
    ../../core/cert.c ../../core/cert.h
    ../../core/control.c ../../core/control.h
    ../../core/cpu.c ../../core/cpu.h
    ../../core/debug/debug.c ../../core/debug/debug.h
    ../../core/debug/zdis/zdis.c ../../core/debug/zdis/zdis.h
    ../../core/defines.h
    ../../core/emu.c ../../core/emu.h
    ../../core/extras.c ../../core/extras.h
    ../../core/flash.c ../../core/flash.h
    ../../core/interrupt.c ../../core/interrupt.h
    ../../core/keypad.c ../../core/keypad.h
    ../../core/lcd.c ../../core/lcd.h
    ../../core/link.c ../../core/link.h
    ../../core/mem.c ../../core/mem.h
    ../../core/misc.c ../../core/misc.h
    ../../core/os/os.h
    ../../core/port.c ../../core/port.h
    ../../core/realclock.c ../../core/realclock.h
    ../../core/registers.c ../../core/registers.h
    ../../core/schedule.c ../../core/schedule.h
    ../../core/sha256.c ../../core/sha256.h
    ../../core/spi.c ../../core/spi.h
    ../../core/timers.c ../../core/timers.h
    ../../core/usb/device.h
    ../../core/usb/disconnected.c
    ../../core/usb/dusb.c
    ../../core/usb/fotg210.h
    ../../core/usb/usb.c ../../core/usb/usb.h
    ../../core/vat.c ../../core/vat.h
    ../../tests/autotester/autotester.cpp ../../tests/autotester/autotester.h
    archive/extractor.c archive/extractor.h
    basiccodeviewerwindow.cpp basiccodeviewerwindow.h basiccodeviewerwindow.ui
    basicdebugger.cpp
    capture/animated-png.c capture/animated-png.h
    cemuopts.h
    datawidget.cpp datawidget.h
    debugger.cpp
    debugger/disasm.cpp debugger/disasm.h
    debugger/hexwidget.cpp debugger/hexwidget.h
    debugger/visualizerdisplaywidget.cpp debugger/visualizerdisplaywidget.h
    dockwidget.cpp dockwidget.h
    emuthread.cpp emuthread.h
    ipc.cpp ipc.h
    keyhistorywidget.cpp keyhistorywidget.h
    keypad/alphakey.h
    keypad/arrowkey.cpp keypad/arrowkey.h
    keypad/graphkey.h
    keypad/key.h
    keypad/keycode.h
    keypad/keyconfig.h
    keypad/keymap.cpp keypad/keymap.h
    keypad/keypadwidget.cpp keypad/keypadwidget.h
    keypad/numkey.h
    keypad/operkey.h
    keypad/otherkey.h
    keypad/qtkeypadbridge.cpp keypad/qtkeypadbridge.h
    keypad/rectkey.cpp keypad/rectkey.h
    keypad/secondkey.h
    lcdwidget.cpp lcdwidget.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    memorywidget.cpp
    romselection.cpp romselection.h romselection.ui
    searchwidget.cpp searchwidget.h searchwidget.ui
    sendinghandler.cpp sendinghandler.h
    settings.cpp
    tablewidget.cpp tablewidget.h
    tivars_lib_cpp/src/BinaryFile.cpp tivars_lib_cpp/src/BinaryFile.h
    tivars_lib_cpp/src/CommonTypes.h
    tivars_lib_cpp/src/TIModel.cpp tivars_lib_cpp/src/TIModel.h
    tivars_lib_cpp/src/TIModels.cpp tivars_lib_cpp/src/TIModels.h
    tivars_lib_cpp/src/TIVarFile.cpp tivars_lib_cpp/src/TIVarFile.h
    tivars_lib_cpp/src/TIVarType.cpp tivars_lib_cpp/src/TIVarType.h
    tivars_lib_cpp/src/TIVarTypes.cpp tivars_lib_cpp/src/TIVarTypes.h
    tivars_lib_cpp/src/TypeHandlers/DummyHandler.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_DataAppVar.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_ExactFraction.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_ExactFractionPi.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_ExactPi.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_ExactRadical.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_FP.cpp
    tivars_lib_cpp/src/TypeHandlers/STH_PythonAppVar.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericAppVar.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericComplex.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericList.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericReal.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_Matrix.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_TempEqu.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_Tokenized.cpp
    tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h
    tivars_lib_cpp/src/tivarslib_utils.cpp tivars_lib_cpp/src/tivarslib_utils.h
    utils.cpp utils.h
    visualizerwidget.cpp visualizerwidget.h
)

# TODO better, see https://stackoverflow.com/a/21028226/378298
execute_process(
    COMMAND git describe --abbrev=7 --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

target_compile_definitions(CEmu PRIVATE
    CEMU_GIT_SHA=\"${GIT_COMMIT_HASH}\"
    CEMU_RELEASE=false
    CEMU_VERSION=\"v2.0dev\"
    DEBUG_SUPPORT
    MULTITHREAD
)

include(CheckSymbolExists)
check_symbol_exists(glob "glob.h" HAVE_GLOB)
if(HAVE_GLOB)
    target_compile_definitions(CEmu PRIVATE "GLOB_SUPPORT")
endif()

find_package(PkgConfig QUIET)

if(PkgConfig_FOUND AND TRY_STATIC_LIBS)
    pkg_check_modules(DEPS libarchive expat zlib)
    if(DEPS_FOUND)
        LIST(APPEND DEPS_STATIC_LIBRARIES "bz2")
        try_static_libs_if_needed()
        if(DEPS_STATIC_INCLUDE_DIRS AND DEPS_STATIC_LIBRARY_DIRS AND TRY_STATIC_DEPS_LDFLAGS)
            find_package(Iconv)
            if(Iconv_FOUND)
                if(NOT Iconv_IS_BUILT_IN)
                    target_include_directories(CEmu PRIVATE ${Iconv_INCLUDE_DIRS})
                    target_link_libraries(CEmu PRIVATE ${Iconv_LIBRARIES})
                endif()
                set(LibArchive_FOUND ON)
                set(LibArchive_INCLUDE_DIRS ${DEPS_STATIC_INCLUDE_DIRS})
                set(LibArchive_LIBRARY_DIRS ${DEPS_STATIC_LIBRARY_DIRS})
                set(LibArchive_LIBRARIES ${TRY_STATIC_DEPS_LDFLAGS})
            endif()
        endif()
    else()
        message(WARNING "Could not find LibArchive deps for a static build, will use fallback")
    endif()
endif()
if(NOT LibArchive_FOUND)
    find_package(LibArchive QUIET)
    if (NOT LibArchive_FOUND AND PkgConfig_FOUND)
        pkg_check_modules(LibArchive LibArchive)
    endif()
endif()
if(LibArchive_FOUND)
    target_compile_definitions(CEmu PRIVATE "LIB_ARCHIVE_SUPPORT")
    target_include_directories(CEmu PRIVATE ${LibArchive_INCLUDE_DIRS})
    target_link_directories(CEmu PRIVATE ${LibArchive_LIBRARY_DIRS})
    target_link_libraries(CEmu PRIVATE ${LibArchive_LIBRARIES})
else()
    message(WARNING "No LibArchive found! CE Bundle extraction/transfer will not be available")
endif()

if(NOT WIN32 AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/capture/libpng-apng/.libs/libpng16.a")
    message(STATUS "Static APNG-enabled LibPNG will be used")
    target_compile_definitions(CEmu PRIVATE "PNG_SUPPORT")
    target_include_directories(CEmu PRIVATE capture/libpng-apng)
    target_link_libraries(CEmu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/capture/libpng-apng/.libs/libpng16.a)
else()
    find_package(PNG QUIET)
    if (NOT PNG_FOUND AND PkgConfig_FOUND)
        pkg_check_modules(PNG libpng16)
    endif()
    if(PNG_FOUND)
        message(WARNING "External LibPNG will be used! It may not have APNG support!")
        target_compile_definitions(CEmu PRIVATE "PNG_SUPPORT")
        target_include_directories(CEmu PRIVATE ${PNG_INCLUDE_DIRS})
        target_link_directories(CEmu PRIVATE ${PNG_LIBRARY_DIRS})
        target_link_libraries(CEmu PRIVATE ${PNG_LIBRARIES})
        target_link_options(CEmu PRIVATE ${LibArchive_LDFLAGS})
    else()
        message(WARNING "No LibPNG found! APNG capture will not be available")
    endif()
endif()

target_link_libraries(CEmu PRIVATE
    Qt::Core
    Qt::Gui
    Qt::DBus # needed at runtime by QtGui for some reason...?
    Qt::Network
    Qt::Widgets
)

qt_add_resources(CEmu "resources"
    PREFIX
        "/skin"
    FILES
        "resources/skin/ti83pce.png"
        "resources/skin/ti84pce.png"
)
qt_add_resources(CEmu "resources1"
    PREFIX
        "/setup"
    FILES
        "resources/setup/home.png"
)
qt_add_resources(CEmu "resources2"
    PREFIX
        "/icons"
    FILES
        "resources/icons/add_mem.png"
        "resources/icons/autotester.png"
        "resources/icons/break_watch_port.png"
        "resources/icons/breakpoints.png"
        "resources/icons/bug.png"
        "resources/icons/capture.png"
        "resources/icons/change_cert.png"
        "resources/icons/change_language.png"
        "resources/icons/characters.png"
        "resources/icons/check.png"
        "resources/icons/check_update.png"
        "resources/icons/checkgray.png"
        "resources/icons/clock.png"
        "resources/icons/console.png"
        "resources/icons/cpu_status.png"
        "resources/icons/debug_control.png"
        "resources/icons/debugger.png"
        "resources/icons/disasm.png"
        "resources/icons/exit.png"
        "resources/icons/export.png"
        "resources/icons/generic_export.png"
        "resources/icons/generic_import.png"
        "resources/icons/goto.png"
        "resources/icons/icon.ico"
        "resources/icons/icon.png"
        "resources/icons/import.png"
        "resources/icons/info.png"
        "resources/icons/key_history.png"
        "resources/icons/keypad.png"
        "resources/icons/lcd.png"
        "resources/icons/memory.png"
        "resources/icons/minus.png"
        "resources/icons/misc.png"
        "resources/icons/open.png"
        "resources/icons/os_view.png"
        "resources/icons/plus.png"
        "resources/icons/port.png"
        "resources/icons/profiler.png"
        "resources/icons/ram_chip.png"
        "resources/icons/record.png"
        "resources/icons/refresh.png"
        "resources/icons/reload_rom.png"
        "resources/icons/reset.png"
        "resources/icons/reset_cemu.png"
        "resources/icons/reset_gui.png"
        "resources/icons/rom_chip.png"
        "resources/icons/run.png"
        "resources/icons/save.png"
        "resources/icons/screenshot.png"
        "resources/icons/script.png"
        "resources/icons/search.png"
        "resources/icons/settings.png"
        "resources/icons/source_code.png"
        "resources/icons/stack.png"
        "resources/icons/states.png"
        "resources/icons/stepin.png"
        "resources/icons/stepnext.png"
        "resources/icons/stepout.png"
        "resources/icons/stepover.png"
        "resources/icons/stop.png"
        "resources/icons/timers.png"
        "resources/icons/toggle_console.png"
        "resources/icons/ui_edit.png"
        "resources/icons/variables.png"
        "resources/icons/watchpoints.png"
        "resources/icons/wizard.png"
)
qt_add_resources(CEmu "resources3"
    PREFIX
        "/fonts"
    FILES
        "resources/custom_fonts/LiberationSansNarrow-Bold.ttf"
        "resources/custom_fonts/TICELarge.ttf"
)
qt_add_resources(CEmu "resources4"
    PREFIX
        "/other"
    FILES
        "tivars_lib_cpp/programs_tokens.csv"
)
qt_add_resources(CEmu "resources5"
    PREFIX
        "/i18n"
    FILES
        "i18n/es_ES.qm"
        "i18n/fr_FR.qm"
        "i18n/nl_NL.qm"
        "i18n/zh_CN.qm"
)

if(CMAKE_BUILD_TYPE STREQUAL Release OR CMAKE_BUILD_TYPE STREQUAL RelWithDebInfo)
    include(CheckIPOSupported)
    check_ipo_supported(RESULT supported OUTPUT error)
    if(supported)
        message(STATUS "IPO/LTO enabled")
        set_property(TARGET CEmu PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    else()
        message(STATUS "IPO/LTO not supported: <${error}>")
    endif()
    target_compile_definitions(CEmu PRIVATE QT_NO_DEBUG_OUTPUT)
endif()

if(UNIX OR APPLE)
    target_sources(CEmu PUBLIC ../../core/os/os-linux.c)
endif()

if(APPLE)
    target_sources(CEmu PUBLIC
        os/mac/kdmactouchbar.h os/mac/kdmactouchbar.mm
        os/mac/kdmactouchbar_global.h
    )
    target_link_libraries(CEmu PRIVATE "-framework Cocoa")
endif()

if(WIN32)
    target_sources(CEmu PUBLIC
        ../../core/os/os-win32.c
        win32-console.cpp
    )
    target_link_libraries(CEmu PRIVATE psapi)
endif()

install(TARGETS CEmu
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

# TODO: linux .desktop/.xml files

# Consider using qt_generate_deploy_app_script() for app deployment if the project can use Qt 6.3. In that case rerun qmake2cmake with --min-qt-version=6.3.

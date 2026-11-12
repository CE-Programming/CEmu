cmake_minimum_required(VERSION 3.24)

if(APPLE)
    if(NOT DEFINED CMAKE_OSX_DEPLOYMENT_TARGET)
        set(CMAKE_OSX_DEPLOYMENT_TARGET "13.0" CACHE STRING "macOS deployment target")
    endif()
    set(MAYBE_MACOSX_BUNDLE MACOSX_BUNDLE)
endif()

option(BUILD_SHARED_LIBS "Build Qt as shared libraries (CPM auxiliary libraries stay static)" OFF)
option(COPROC_DEBUG_SUPPORT "Build ARM coprocessor debugging support" ON)
option(CEMU_DEPLOY_RUNTIME "Install Qt runtime dependencies with CEmu" ON)
set(CEMU_EXECUTABLE_NAME CEmu CACHE STRING "Name of the CEmu executable")

project(CEmu
        VERSION 3.0
        LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../core/debug/zdis/zdis.c")
    message(FATAL_ERROR "zdis is missing - you have to run 'git submodule init' and 'git submodule update' first.")
endif()

get_property(CEMU_MULTI_CONFIG GLOBAL PROPERTY GENERATOR_IS_MULTI_CONFIG)
if(NOT CEMU_MULTI_CONFIG AND NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(USED_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING "Expose CMAKE_GENERATOR" FORCE)
message(STATUS "Detected system: ${CMAKE_SYSTEM_NAME} - host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR} - CXX_COMPILER: ${CMAKE_CXX_COMPILER_ID}")

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(GNUInstallDirs)
include(cmake/CEmuDependencies.cmake)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS Gui Network Widgets)
if(NOT WIN32)
    find_package(Qt6 QUIET OPTIONAL_COMPONENTS DBus)
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

qt_add_resources(CEmu_Resources resources.qrc)

set(CEmu_Sources
    ../../core/arm/free_bootloader_image.h
    ../../core/arm/arm.c ../../core/arm/arm.h
    ../../core/arm/armcpu.c ../../core/arm/armcpu.h
    ../../core/arm/armmem.c ../../core/arm/armmem.h
    ../../core/arm/armstate.h
    ../../core/arm/spscqueue.c ../../core/arm/spscqueue.h
    ../../core/arm/sync.c ../../core/arm/sync.h
    ../../core/arm/threading.h
    ../../core/asic.c ../../core/asic.h
    ../../core/atomics.h
    ../../core/backlight.c ../../core/backlight.h
    ../../core/bootver.c ../../core/bootver.h
    ../../core/bus.c ../../core/bus.h
    ../../core/cert.c ../../core/cert.h
    ../../core/control.c ../../core/control.h
    ../../core/coproc.c ../../core/coproc.h
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
    ../../core/panel.c ../../core/panel.h
    ../../core/port.c ../../core/port.h
    ../../core/realclock.c ../../core/realclock.h
    ../../core/registers.c ../../core/registers.h
    ../../core/schedule.c ../../core/schedule.h
    ../../core/sha256.c ../../core/sha256.h
    ../../core/spi.c ../../core/spi.h
    ../../core/timers.c ../../core/timers.h
    ../../core/uart.c ../../core/uart.h
    ../../core/usb/device.h
    ../../core/usb/disconnected.c
    ../../core/usb/dusb.c
    ../../core/usb/fotg210.h
    ../../core/usb/msd.c
    ../../core/usb/physical.c
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
    gotodialog.cpp gotodialog.h
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
    lcddebugwidget.cpp lcddebugwidget.h
    luascripting.cpp
    luaeditor.cpp luaeditor.h
    main.cpp
    mainwindow.cpp mainwindow.h mainwindow.ui
    memorywidget.cpp
    romselection.cpp romselection.h romselection.ui
    searchwidget.cpp searchwidget.h searchwidget.ui
    sendinghandler.cpp sendinghandler.h
    settings.cpp
    tablewidget.cpp tablewidget.h
    tibasicutils.cpp tibasicutils.h
    tivars_lib_cpp/tivars_lib_cpp.cpp tivars_lib_cpp/tivars_lib_cpp.hpp
    utils.cpp utils.h
    vartablemodel.cpp vartablemodel.h
    visualizerwidget.cpp visualizerwidget.h
    ${CEmu_Resources}
)

if(COPROC_DEBUG_SUPPORT)
    list(APPEND CEmu_Sources
        armdebugger.cpp armdebugger.h
        armgdbclient.cpp armgdbclient.h
        armgdbserver.cpp armgdbserver.h
    )
endif()

include(CheckCSourceCompiles)
check_c_source_compiles("
	#include <threads.h>
	#include <stdatomic.h>
	int main(void) { thrd_t thread; mtx_t mutex; tss_t tss; return 0; }
	" HAVE_C11_THREADS)
if (HAVE_C11_THREADS)
    add_definitions(-DHAVE_C11_THREADS)
else()
    message(WARNING "C11 <threads.h> not supported, using tinycthread library as fallback")
    set(CEmu_Sources ${CEmu_Sources} ../../core/arm/tinycthread.c ../../core/arm/tinycthread.h)
endif()

qt_add_executable(CEmu ${MAYBE_MACOSX_BUNDLE} ${CEmu_Sources})
set_target_properties(CEmu PROPERTIES OUTPUT_NAME "${CEMU_EXECUTABLE_NAME}")

# Keep CEmu's warning policy and sanitizers out of third-party builds.
if(MSVC)
    target_compile_options(CEmu PRIVATE /bigobj /utf-8)
    target_compile_definitions(CEmu PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(MSVC_VERSION GREATER_EQUAL 1935 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_compile_options(CEmu PRIVATE $<$<COMPILE_LANGUAGE:C>:/experimental:c11atomics>)
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(CEmu PRIVATE
        -W -Wall -Wextra -Wno-unused-parameter -Werror=write-strings
        -Wredundant-decls -Werror=date-time -Werror=return-type -Werror=pointer-arith
        $<$<COMPILE_LANGUAGE:C>:-Werror=implicit-function-declaration;-Werror=missing-prototypes>
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer;-fsanitize=address,bounds;-fsanitize-undefined-trap-on-error>
    )
    target_link_options(CEmu PRIVATE
        $<$<CONFIG:Debug>:-fno-omit-frame-pointer;-fsanitize=address,bounds;-fsanitize-undefined-trap-on-error>
    )
endif()

target_include_directories(CEmu PRIVATE
    ../../core/arm/CMSIS/Core/Include
)

# TODO better, see https://stackoverflow.com/a/21028226/378298
execute_process(
    COMMAND git describe --abbrev=7 --always
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_CURR_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

option(IS_OFFICIAL_RELEASE_VERSION "Whether the current build is for an official release" OFF)
if(IS_OFFICIAL_RELEASE_VERSION)
    set(VERSION_SUFFIX "")
    set(CEMU_RELEASE_BOOL true)
else()
    set(VERSION_SUFFIX "dev")
    set(CEMU_RELEASE_BOOL false)
endif()

set(SHORT_VERSION "v3.0" CACHE STRING "Version of CEmu in the form of 'vX.Y'")
set(LONG_VERSION "${SHORT_VERSION}${VERSION_SUFFIX} (${GIT_CURR_BRANCH} - ${GIT_COMMIT_HASH})")

target_compile_definitions(CEmu PRIVATE
    CEMU_GIT_SHA=\"${GIT_COMMIT_HASH}\"
    CEMU_RELEASE=${CEMU_RELEASE_BOOL}
    CEMU_VERSION=\"${SHORT_VERSION}${VERSION_SUFFIX}\"
    DEBUG_SUPPORT
    MULTITHREAD
    TH_GDB_SUPPORT=1
)

if(COPROC_DEBUG_SUPPORT)
    target_compile_definitions(CEmu PRIVATE COPROC_DEBUG_SUPPORT)
endif()

if(CYGWIN OR NOT WIN32)
    include(CheckSymbolExists)
    check_symbol_exists(glob "glob.h" HAVE_GLOB)
    if(HAVE_GLOB)
        target_compile_definitions(CEmu PRIVATE "GLOB_SUPPORT")
    else()
        message(WARNING "glob function not supported, AUTOTESTER_LIBS_DIR usage will not work!")
    endif()
endif()

target_compile_definitions(CEmu PRIVATE LIB_ARCHIVE_SUPPORT LIBUSB_SUPPORT LUA_SUPPORT)
get_target_property(CEMU_HAS_APNG CEmu::PNG CEMU_HAS_APNG)
if(CEMU_HAS_APNG)
    target_compile_definitions(CEmu PRIVATE PNG_SUPPORT)
    # moc does not preprocess png.h, but must see the same APNG-enabled declarations.
    set_property(TARGET CEmu APPEND PROPERTY AUTOMOC_MOC_OPTIONS -DPNG_SUPPORT -DPNG_WRITE_APNG_SUPPORTED)
endif()
target_link_libraries(CEmu PRIVATE
    CEmu::LibArchive CEmu::LibUSB CEmu::PNG CEmu::Lua CEmu::Sol2
)

target_link_libraries(CEmu PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Widgets
)
if(TARGET Qt::DBus)
    target_link_libraries(CEmu PRIVATE Qt::DBus)
endif()

include(CheckIPOSupported)
check_ipo_supported(RESULT lto_supported OUTPUT error)
if(lto_supported)
    set_target_properties(CEmu PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION_DEBUG FALSE
        INTERPROCEDURAL_OPTIMIZATION_RELEASE TRUE
        INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO TRUE
    )
else()
    message(STATUS "IPO/LTO not supported: <${error}>")
endif()

target_compile_definitions(CEmu PRIVATE $<$<CONFIG:Release,RelWithDebInfo>:QT_NO_DEBUG_OUTPUT>)

if(UNIX OR APPLE)
    target_sources(CEmu PUBLIC ../../core/os/os-linux.c)
endif()

if(APPLE)
    set(MACOSX_BUNDLE_ICON_FILE icon.icns)
    set(app_icon_macos "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/icon.icns")
    set_source_files_properties(${app_icon_macos} PROPERTIES MACOSX_PACKAGE_LOCATION "Resources")
    target_sources(CEmu PUBLIC
        ../../core/usb/physical_macos.c
        os/mac/kdmactouchbar.h os/mac/kdmactouchbar.mm
        os/mac/kdmactouchbar_global.h
        ${app_icon_macos}
    )
    target_link_libraries(CEmu PRIVATE
        "-framework Cocoa"
        "-framework CoreFoundation"
        "-framework IOKit"
    )
    set_target_properties(CEmu PROPERTIES
        MACOSX_FRAMEWORK_IDENTIFIER "com.adriweb.CEmu"
        MACOSX_BUNDLE_COPYRIGHT "CE-Programming team"
        MACOSX_BUNDLE_LONG_VERSION_STRING "${LONG_VERSION}"
    )
endif()

if(WIN32)
    target_sources(CEmu PUBLIC
        ../../core/os/os-win32.c
        win32-console.cpp
        resources/windows/cemu.rc
    )
    target_link_libraries(CEmu PRIVATE psapi)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        target_link_libraries(CEmu PRIVATE clang_rt.builtins-x86_64)
    endif()
endif()

install(TARGETS CEmu
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

message("Binary dir: ${CMAKE_CURRENT_BINARY_DIR}")

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    configure_file(resources/linux/cemu.desktop.in cemu.desktop @ONLY)
    install(FILES "${CMAKE_CURRENT_BINARY_DIR}/cemu.desktop"
        DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
    install(FILES resources/linux/cemu.xml
        DESTINATION "${CMAKE_INSTALL_DATADIR}/mime/packages")
    foreach(size IN ITEMS 16 20 22 24 32 36 40 42 48 64 72 96 128 160 192 256 512)
        install(FILES "resources/icons/linux/cemu-${size}x${size}.png"
            DESTINATION "${CMAKE_INSTALL_DATADIR}/icons/hicolor/${size}x${size}/apps"
            RENAME cemu.png)
    endforeach()
endif()

if(CEMU_DEPLOY_RUNTIME AND COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET CEmu
        OUTPUT_SCRIPT deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

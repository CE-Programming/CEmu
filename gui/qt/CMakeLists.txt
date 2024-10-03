cmake_minimum_required(VERSION 3.21.1)

option(USE_QT5 "Build with Qt5 instead of Qt6" OFF)

if(USE_QT5)
    list(APPEND VCPKG_MANIFEST_FEATURES "qt5")
    if(APPLE)
        set(ENV{MACOSX_DEPLOYMENT_TARGET} "10.13")
        set(ENV{VCPKG_KEEP_ENV_VARS} "MACOSX_DEPLOYMENT_TARGET")
    endif()
else()
    list(APPEND VCPKG_MANIFEST_FEATURES "qt6")
    if(APPLE)
        set(ENV{MACOSX_DEPLOYMENT_TARGET} "11.0")
        set(ENV{VCPKG_KEEP_ENV_VARS} "MACOSX_DEPLOYMENT_TARGET")
    endif()
endif()

option(DEPS_RELEASE_ONLY "Build only release versions of vcpkg dependencies" OFF)

if(DEPS_RELEASE_ONLY)
    if(NOT DEFINED VCPKG_TARGET_TRIPLET)
        message(FATAL_ERROR "Must provide a VCPKG_TARGET_TRIPLET to set as release only")
    endif()
    if(NOT VCPKG_TARGET_TRIPLET MATCHES "-release$")
        set(VCPKG_TARGET_TRIPLET "${VCPKG_TARGET_TRIPLET}-release")
        message("Updated VCPKG_TARGET_TRIPLET to ${VCPKG_TARGET_TRIPLET}")
    endif()
endif()

project(CEmu
        VERSION 2.0
        LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../core/debug/zdis/zdis.c" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tivars_lib_cpp/src/TIVarFile.cpp")
    message(FATAL_ERROR "Some files seem to be missing - you have to run 'git submodule init' and 'git submodule update' first.")
endif()

if(WIN32 AND VCPKG_TARGET_TRIPLET MATCHES "-static(-|$)" AND NOT VCPKG_TARGET_TRIPLET MATCHES "-md(-|$)")
    message("Using static MSVC runtime...")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_SKIP_RPATH TRUE)

set(USED_CMAKE_GENERATOR "${CMAKE_GENERATOR}" CACHE STRING "Expose CMAKE_GENERATOR" FORCE)
message(STATUS "Detected system: ${CMAKE_SYSTEM_NAME} - host processor: ${CMAKE_HOST_SYSTEM_PROCESSOR} - CXX_COMPILER: ${CMAKE_CXX_COMPILER_ID}")

# C11, and C++20 if supported otherwise C++14
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
include(CheckCXXCompilerFlag)
if(MSVC)
    CHECK_CXX_COMPILER_FLAG("/std:c++20" COMPILER_SUPPORTS_CXX20)
else()
    CHECK_CXX_COMPILER_FLAG("-std=c++2a" COMPILER_SUPPORTS_CXX20)
endif()
if(COMPILER_SUPPORTS_CXX20)
    set(CMAKE_CXX_STANDARD 20)
    add_definitions(-DTH_GDB_SUPPORT=1)
else()
    set(CMAKE_CXX_STANDARD 14)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
endif()

# Sane flags
if(MSVC)
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /utf-8 /D_CRT_SECURE_NO_WARNINGS")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /utf-8 /D_CRT_SECURE_NO_WARNINGS")
    if(MSVC_VERSION GREATER_EQUAL 1935 AND NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /experimental:c11atomics")
    endif()
elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # sane defaults + hardening
    set(GLOBAL_COMPILE_FLAGS "-W -Wall -Wextra -Wno-unused-parameter -Werror=write-strings -Wredundant-decls -Werror=date-time -Werror=return-type -Werror=pointer-arith")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GLOBAL_COMPILE_FLAGS} -Werror=implicit-function-declaration -Werror=missing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GLOBAL_COMPILE_FLAGS}")
    # useful flags for debugging
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
endif()

include(GNUInstallDirs)

if(USE_QT5)
    find_package(Qt5 REQUIRED COMPONENTS Core DBus Gui Network Widgets Svg)
else()
    find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
    find_package(Qt6 REQUIRED COMPONENTS DBus Gui Network Widgets Svg)
endif()

set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
endif()

if(APPLE)
    set(MAYBE_MACOSX_BUNDLE MACOSX_BUNDLE)
endif()

qt_add_resources(CEmu_Resources resources.qrc)

set(CEmu_Sources
    ../../core/asic.c ../../core/asic.h
    ../../core/atomics.h
    ../../core/backlight.c ../../core/backlight.h
    ../../core/bootver.c ../../core/bootver.h
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
    main.cpp
    calculatorwidget.cpp calculatorwidget.h
    capturewidget.cpp capturewidget.h
    consolewidget.cpp consolewidget.h
    corethread.cpp corethread.h
    corewindow.cpp corewindow.h
    dockedwidget.cpp dockedwidget.h
    keyhistorywidget.cpp keyhistorywidget.h
    overlaywidget.cpp overlaywidget.h
    romdialog.cpp romdialog.h
    screenshotwidget.cpp screenshotwidget.h
    screenwidget.cpp screenwidget.h
    settings.cpp settings.h
    settingsdialog.cpp settingsdialog.h
    statewidget.cpp statewidget.h
    tablewidget.cpp tablewidget.h
    util.cpp util.h
    variablewidget.cpp variablewidget.h

    developer/autotesterwidget.cpp developer/autotesterwidget.h
    developer/basicwidget.cpp developer/basicwidget.h
    developer/clockswidget.cpp developer/clockswidget.h
    developer/controlwidget.cpp developer/controlwidget.h
    developer/cpuwidget.cpp developer/cpuwidget.h
    developer/devmiscwidget.cpp developer/devmiscwidget.h
    developer/disassemblywidget.cpp developer/disassemblywidget.h
    developer/memorywidget.cpp developer/memorywidget.h
    developer/osstackswidget.cpp developer/osstackswidget.h
    developer/osvarswidget.cpp developer/osvarswidget.h
    developer/performancewidget.cpp developer/performancewidget.h
    developer/portmonitorwidget.cpp developer/portmonitorwidget.h
    developer/usbwidget.cpp developer/usbwidget.h
    developer/visualizerwidget.cpp developer/visualizerwidget.h
    developer/watchpointswidget.cpp developer/watchpointswidget.h
    developer/widgets/disassembler.cpp developer/widgets/disassembler.h
    developer/widgets/disassemblerwidget.cpp developer/widgets/disassemblerwidget.h
    developer/widgets/hexwidget.cpp developer/widgets/hexwidget.h
    developer/widgets/highlighteditwidget.cpp developer/widgets/highlighteditwidget.h
    developer/widgets/memwidget.cpp developer/widgets/memwidget.h
    developer/widgets/sliderpanner.cpp developer/widgets/sliderpanner.h
    developer/widgets/visualizerlcdwidget.cpp developer/widgets/visualizerlcdwidget.h
    keypad/alphakey.h
    keypad/arrowkey.cpp keypad/arrowkey.h
    keypad/graphkey.h
    keypad/keycode.h
    keypad/keyconfig.h
    keypad/key.h
    keypad/keymap.cpp keypad/keymap.h
    keypad/keypadwidget.cpp keypad/keypadwidget.h
    keypad/numkey.h
    keypad/operkey.h
    keypad/otherkey.h
    keypad/qtkeypadbridge.cpp keypad/qtkeypadbridge.h
    keypad/rectkey.cpp keypad/rectkey.h
    keypad/secondkey.h
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
    tivars_lib_cpp/src/TypeHandlers/TH_GDB.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericAppVar.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericComplex.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericList.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_GenericReal.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_Matrix.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_TempEqu.cpp
    tivars_lib_cpp/src/TypeHandlers/TH_Tokenized.cpp
    tivars_lib_cpp/src/TypeHandlers/TypeHandlers.h
    tivars_lib_cpp/src/tivarslib_utils.cpp tivars_lib_cpp/src/tivarslib_utils.h
    ${CEmu_Resources}
)

if(USE_QT5)
    add_executable(CEmu ${MAYBE_MACOSX_BUNDLE} ${CEmu_Sources})
else()
    qt_add_executable(CEmu ${MAYBE_MACOSX_BUNDLE} ${CEmu_Sources})
endif()

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
)

if(CYGWIN OR NOT WIN32)
    include(CheckSymbolExists)
    check_symbol_exists(glob "glob.h" HAVE_GLOB)
    if(HAVE_GLOB)
        target_compile_definitions(CEmu PRIVATE "GLOB_SUPPORT")
    else()
        message(WARNING "glob function not supported, AUTOTESTER_LIBS_DIR usage will not work!")
    endif()
endif()

find_package(LibArchive QUIET)
if(LibArchive_FOUND)
    target_compile_definitions(CEmu PRIVATE "LIB_ARCHIVE_SUPPORT")
    target_link_libraries(CEmu PRIVATE LibArchive::LibArchive)
else()
    message(WARNING "No LibArchive found! CE Bundle extraction/transfer will not be available")
endif()

find_package(PkgConfig REQUIRED)
if(PkgConfig_FOUND)
    pkg_check_modules(LibUsb libusb-1.0>=1.0.16)
    if(LibUsb_FOUND)
        target_compile_definitions(CEmu PRIVATE "LIBUSB_SUPPORT")
        target_include_directories(CEmu PRIVATE ${LibUsb_INCLUDE_DIRS})
        target_link_directories(CEmu PRIVATE ${LibUsb_LIBRARY_DIRS})
        target_link_libraries(CEmu PRIVATE ${LibUsb_LIBRARIES})
    else()
        message(WARNING "No LibUsb found! Some USB things will not be available")
    endif()
endif()

find_package(PNG QUIET)
if(PNG_FOUND)
    target_compile_definitions(CEmu PRIVATE "PNG_SUPPORT")
    target_link_libraries(CEmu PRIVATE PNG::PNG)
else()
    message(WARNING "No LibPNG found! APNG capture will not be available")
endif()

find_package(KDDockWidgets-qt6 CONFIG REQUIRED)
target_link_libraries(CEmu PRIVATE KDAB::kdbindings KDAB::kddockwidgets)

target_link_libraries(CEmu PRIVATE
    Qt::Core
    Qt::Gui
    Qt::DBus # needed at runtime by QtGui for some reason...?
    Qt::Network
    Qt::Widgets
    Qt::Svg
)

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
        os/mac/kdmactouchbar.h os/mac/kdmactouchbar.mm
        os/mac/kdmactouchbar_global.h
        ${app_icon_macos}
    )
    target_link_libraries(CEmu PRIVATE "-framework Cocoa")
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

# TODO: linux .desktop/.xml files

if(COMMAND qt_generate_deploy_app_script)
    qt_generate_deploy_app_script(
        TARGET CEmu
        FILENAME_VARIABLE deploy_script
        NO_UNSUPPORTED_PLATFORM_ERROR
    )
    install(SCRIPT ${deploy_script})
endif()

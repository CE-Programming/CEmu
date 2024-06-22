cmake_minimum_required(VERSION 3.21.1)

project(CEmu
        VERSION 2.0
        LANGUAGES C CXX)

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../core/debug/zdis/zdis.c" OR
   NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tivars_lib_cpp/src/TIVarFile.cpp")
    message(FATAL_ERROR "Some files seem to be missing - you have to run 'git submodule init' and 'git submodule update' first.")
endif()

option(TRY_STATIC_LIBS "Build using as many static libs as possible" OFF)
if (VCPKG_TARGET_TRIPLET MATCHES "-static(-md|-release)?$")
    if(NOT TRY_STATIC_LIBS)
        message(WARNING "Setting TRY_STATIC_LIBS to ON anyway due to static vcpkg target")
        set(TRY_STATIC_LIBS ON)
    endif()
endif()

if(TRY_STATIC_LIBS)
    include("${CMAKE_CURRENT_SOURCE_DIR}/.cmake/try_static_libs.cmake")
    message("Will try to build as statically as possible...")
    if(WIN32)
        set(CMAKE_FIND_LIBRARY_SUFFIXES .lib .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
        if (NOT VCPKG_TARGET_TRIPLET MATCHES "-static-md$")
            message("Using static MSVC runtime...")
            set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
        endif()
    else()
        set(CMAKE_FIND_LIBRARY_SUFFIXES .a ${CMAKE_FIND_LIBRARY_SUFFIXES})
    endif()
endif()

set(CMAKE_INCLUDE_CURRENT_DIR ON)

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
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    # sane defaults + hardening
    set(GLOBAL_COMPILE_FLAGS "-W -Wall -Wextra -Wno-unused-parameter -Werror=write-strings -Wredundant-decls -Werror=date-time -Werror=return-type -Werror=pointer-arith")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${GLOBAL_COMPILE_FLAGS} -Werror=implicit-function-declaration -Werror=missing-prototypes")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${GLOBAL_COMPILE_FLAGS}")
    # useful flags for debugging
    set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
    set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} -fno-omit-frame-pointer -fsanitize=address,bounds -fsanitize-undefined-trap-on-error ")
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    if(MSVC_VERSION GREATER_EQUAL 1935)
        set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /experimental:c11atomics")
    endif()
endif()

include(GNUInstallDirs)

set(CMAKE_AUTORCC ON)
find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(Qt6 REQUIRED COMPONENTS DBus Gui Network Widgets)
if(COMMAND qt_standard_project_setup)
    qt_standard_project_setup()
else()
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
endif()

if(APPLE)
    set(MAYBE_MACOSX_BUNDLE MACOSX_BUNDLE)
endif()

qt_add_executable(CEmu ${MAYBE_MACOSX_BUNDLE}
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
execute_process(
    COMMAND git rev-parse --abbrev-ref HEAD
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    OUTPUT_VARIABLE GIT_CURR_BRANCH
    OUTPUT_STRIP_TRAILING_WHITESPACE
)

# TODO: adjust these automatically (based on CI, branch, if it's a github release build...)
set(SHORT_VERSION "v2.0")
set(VERSION_SUFFIX "dev")
set(LONG_VERSION "${SHORT_VERSION}${VERSION_SUFFIX} (${GIT_CURR_BRANCH} - ${GIT_COMMIT_HASH})")

target_compile_definitions(CEmu PRIVATE
    CEMU_GIT_SHA=\"${GIT_COMMIT_HASH}\"
    CEMU_RELEASE=false
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

find_package(PkgConfig QUIET)

# Set manually when installed with Homebrew, see https://github.com/Homebrew/legacy-homebrew/issues/45891
# Note: we can't do the same for libiconv because it exports libiconv_* symbols and not iconv_*, so we fallback on the non-static system lib...
if(APPLE)
    set(BREW_LIB_PATHS "/opt/homebrew/lib" "/opt/homebrew/opt" "/usr/local/opt" "/usr/local/lib")
    foreach(LIB_DIR ${BREW_LIB_PATHS})
        if(EXISTS "${LIB_DIR}/libarchive/lib/pkgconfig")
            set(ENV{PKG_CONFIG_PATH} "${LIB_DIR}/libarchive/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
            break()
        endif()
    endforeach()
endif()

if(PkgConfig_FOUND AND TRY_STATIC_LIBS)
    pkg_check_modules(DEPS libarchive)
    if(DEPS_FOUND)
        try_static_libs_if_needed()
        if(TRY_STATIC_DEPS_LIBS)
            find_package(Iconv)
            if(Iconv_FOUND)
                if(NOT Iconv_IS_BUILT_IN)
                    target_include_directories(CEmu PRIVATE ${Iconv_INCLUDE_DIRS})
                    target_link_libraries(CEmu PRIVATE ${Iconv_LIBRARIES})
                endif()
                set(LibArchive_FOUND ON)
                set(LibArchive_INCLUDE_DIRS ${DEPS_STATIC_INCLUDE_DIRS})
                set(LibArchive_LIBRARY_DIRS ${TRY_STATIC_DEPS_LIBSDIRS})
                set(LibArchive_LIBRARIES ${TRY_STATIC_DEPS_LDFLAGS_OTHER} ${TRY_STATIC_DEPS_LIBS})
            endif()
        endif()
    endif()
    if(NOT LibArchive_FOUND)
        message(WARNING "Could not find all the deps required for static LibArchive, will use fallback")
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
    if(TRY_STATIC_DEPS_LDFLAGS MATCHES "libz.a") # already linked before
        message(TRACE "libpng-apng dependency 'zlib' already linked")
        set(ZLIB_FOUND ON)
    endif()
    if(PkgConfig_FOUND AND TRY_STATIC_LIBS AND NOT ZLIB_FOUND)
        pkg_check_modules(DEPS zlib)
        if(DEPS_FOUND)
            try_static_libs_if_needed()
            if(DEPS_STATIC_LIBRARY_DIRS AND TRY_STATIC_DEPS_LIBS)
                set(ZLIB_FOUND ON)
                set(ZLIB_INCLUDE_DIRS ${DEPS_STATIC_INCLUDE_DIRS})
                set(ZLIB_LIBRARY_DIRS ${TRY_STATIC_DEPS_LIBSDIRS})
                set(ZLIB_LIBRARIES ${TRY_STATIC_DEPS_LDFLAGS_OTHER} ${TRY_STATIC_DEPS_LIBS})
            endif()
        endif()
        if(NOT ZLIB_FOUND)
            message(WARNING "libpng-apng dependency 'zlib' cannot be found as a static lib, will try to use fallback")
        endif()
    endif()
    if(NOT ZLIB_FOUND)
        find_package(ZLIB QUIET)
        target_include_directories(CEmu PRIVATE ${ZLIB_INCLUDE_DIRS})
    endif()
    if(ZLIB_FOUND)
        target_compile_definitions(CEmu PRIVATE "PNG_SUPPORT")
        target_include_directories(CEmu PRIVATE ${ZLIB_INCLUDE_DIRS})
        target_include_directories(CEmu PRIVATE capture/libpng-apng)
        target_link_directories(CEmu PRIVATE ${ZLIB_LIBRARY_DIRS})
        target_link_libraries(CEmu PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/capture/libpng-apng/.libs/libpng16.a ${ZLIB_LIBRARIES})
    else()
        message(WARNING "libpng-apng dependency 'zlib' cannot be found, so libpng-apng features will not be available!")
    endif()
else()
    find_package(PNG QUIET)
    if (NOT PNG_FOUND AND PkgConfig_FOUND)
        pkg_check_modules(PNG libpng16)
    endif()
    if(PNG_FOUND)
        include(CheckSymbolExists)
        set(CMAKE_REQUIRED_INCLUDES ${PNG_INCLUDE_DIRS})
        check_symbol_exists(PNG_WRITE_APNG_SUPPORTED "png.h" HAVE_APNG)
        if(NOT HAVE_APNG)
            message(WARNING "External LibPNG does not have APNG support!")
        endif()
        target_compile_definitions(CEmu PRIVATE "PNG_SUPPORT")
        target_include_directories(CEmu PRIVATE ${PNG_INCLUDE_DIRS})
        target_link_directories(CEmu PRIVATE ${PNG_LIBRARY_DIRS})
        target_link_libraries(CEmu PRIVATE ${PNG_LIBRARIES})
    else()
        message(WARNING "No LibPNG found! APNG capture will not be available")
    endif()
endif()

# Bugfix for https://bugreports.qt.io/browse/QTBUG-100686
if(APPLE AND PkgConfig_FOUND)
    pkg_check_modules(Brotli libbrotlicommon)
    if(Brotli_FOUND)
        message("macdeployqt bug workaround: linking libbrotlicommon (dynamically) too...")
        target_link_directories(CEmu PRIVATE ${Brotli_LIBRARY_DIRS})
        target_link_libraries(CEmu PRIVATE ${Brotli_LIBRARIES})
    endif()
endif()

target_link_libraries(CEmu PRIVATE
    Qt::Core
    Qt::Gui
    Qt::DBus # needed at runtime by QtGui for some reason...?
    Qt::Network
    Qt::Widgets
)

qt_add_resources(CEmu "skins"
    PREFIX "/skin"
    FILES
        "resources/skin/ti83pce.png"
        "resources/skin/ti84pce.png"
        "resources/skin/ti83pce_ep.png"
        "resources/skin/ti84pce_py.png"
)
qt_add_resources(CEmu "setup"
    PREFIX "/setup"
    FILES
        "resources/setup/home.png"
)
qt_add_resources(CEmu "icons"
    PREFIX "/icons"
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
qt_add_resources(CEmu "custom_fonts"
    PREFIX "/fonts"
    FILES
        "resources/custom_fonts/LiberationSansNarrow-Bold.ttf"
        "resources/custom_fonts/TICELarge.ttf"
)
qt_add_resources(CEmu "other"
    PREFIX "/other"
    FILES
        "tivars_lib_cpp/programs_tokens.csv"
)
qt_add_resources(CEmu "i18n"
    PREFIX "/i18n"
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

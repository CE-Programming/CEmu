include_guard(GLOBAL)

set(CEMU_DEPENDENCY_PROVIDER cpm CACHE STRING "Default dependency provider: cpm or system")
set_property(CACHE CEMU_DEPENDENCY_PROVIDER PROPERTY STRINGS cpm system)
if(NOT CEMU_DEPENDENCY_PROVIDER MATCHES "^(cpm|system)$")
    message(FATAL_ERROR "CEMU_DEPENDENCY_PROVIDER must be cpm or system")
endif()
foreach(dependency IN ITEMS QT ZLIB LIBPNG LIBUSB LIBARCHIVE LUA SOL2)
    set(CEMU_${dependency}_PROVIDER "${CEMU_DEPENDENCY_PROVIDER}" CACHE STRING
        "${dependency} provider: cpm or system")
    set_property(CACHE CEMU_${dependency}_PROVIDER PROPERTY STRINGS cpm system)
    if(NOT CEMU_${dependency}_PROVIDER MATCHES "^(cpm|system)$")
        message(FATAL_ERROR "CEMU_${dependency}_PROVIDER must be cpm or system")
    endif()
    if(CEMU_${dependency}_PROVIDER STREQUAL "cpm")
        set(cemu_needs_cpm ON)
    endif()
endforeach()
if(cemu_needs_cpm)
    include("${CMAKE_CURRENT_LIST_DIR}/CPM.cmake")
endif()
if(CMAKE_BUILD_TYPE)
    set(qt_default_build_type "${CMAKE_BUILD_TYPE}")
else()
    set(qt_default_build_type Release)
endif()
set(CEMU_QT_BUILD_TYPE "${qt_default_build_type}" CACHE STRING "Configuration used to bootstrap Qt")

if(MSVC)
    # A Release-only Qt uses the release CRT even in a Debug application.
    # CEMU_QT_BUILD_TYPE=Debug selects a fully debug Qt/CRT instead.
    if(CEMU_QT_PROVIDER STREQUAL "cpm")
        set(CMAKE_MSVC_RUNTIME_LIBRARY MultiThreaded)
        if(CEMU_QT_BUILD_TYPE STREQUAL "Debug")
            string(APPEND CMAKE_MSVC_RUNTIME_LIBRARY Debug)
        endif()
        if(BUILD_SHARED_LIBS)
            string(APPEND CMAKE_MSVC_RUNTIME_LIBRARY DLL)
        endif()
    elseif(NOT BUILD_SHARED_LIBS)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    endif()
endif()

# Qt needs an installed prefix before qt_add_executable() can be used. Download
# it through CPM, then configure/build/install it in a separate CMake project.
function(cemu_add_qt)
    if(CEMU_QT_PROVIDER STREQUAL "system")
        return()
    elseif(NOT CEMU_QT_PROVIDER STREQUAL "cpm")
        message(FATAL_ERROR "CEMU_QT_PROVIDER must be cpm or system")
    endif()

    CPMAddPackage(
        NAME qtbase
        VERSION 6.12.0
        URL "https://download.qt.io/official_releases/qt/6.12/6.12.0/submodules/qtbase-everywhere-src-6.12.0.tar.xz;https://download.qt.io/archive/qt/6.12/6.12.0/submodules/qtbase-everywhere-src-6.12.0.tar.xz"
        URL_HASH SHA256=a951bd163c7b80fc6b8c88d7668fb56abf91c152373e13c10666763238131307
        DOWNLOAD_ONLY YES
    )

    set(qt_args
        -DQT_BUILD_EXAMPLES=OFF -DQT_BUILD_TESTS=OFF -DQT_BUILD_BENCHMARKS=OFF
        -DQT_USE_DEFAULT_CMAKE_OPTIMIZATION_FLAGS=ON
        -DQT_NO_FORCE_SET_CMAKE_BUILD_TYPE=ON
        -DFEATURE_relocatable=ON -DFEATURE_gui=ON -DFEATURE_widgets=ON
        -DFEATURE_network=ON -DFEATURE_png=ON -DFEATURE_sql=OFF
        -DFEATURE_testlib=OFF -DFEATURE_zstd=OFF
        -DINPUT_opengl=no -DFEATURE_opengl=OFF -DFEATURE_vulkan=OFF
    )
    if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
        list(APPEND qt_args
            -DFEATURE_fontconfig=ON -DFEATURE_xcb=ON -DINPUT_xcb=yes
            -DFEATURE_xkbcommon=ON -DFEATURE_xkbcommon_x11=ON
            -DFEATURE_dbus=ON -DINPUT_dbus=linked
        )
    else()
        list(APPEND qt_args -DFEATURE_dbus=OFF -DINPUT_dbus=no)
    endif()
    if(MSVC AND NOT BUILD_SHARED_LIBS)
        list(APPEND qt_args -DFEATURE_static_runtime=ON)
    endif()

    # Include all toolchain inputs in the prefix name; a stale install must not
    # mask changes of architecture, linkage, compiler or deployment target.
    set(toolchain_args)
    foreach(variable IN ITEMS
        CMAKE_C_COMPILER CMAKE_CXX_COMPILER CMAKE_TOOLCHAIN_FILE
        CMAKE_C_COMPILER_LAUNCHER CMAKE_CXX_COMPILER_LAUNCHER
        CMAKE_OSX_ARCHITECTURES CMAKE_OSX_DEPLOYMENT_TARGET CMAKE_OSX_SYSROOT
        CMAKE_MSVC_RUNTIME_LIBRARY
    )
        if(DEFINED ${variable} AND NOT "${${variable}}" STREQUAL "")
            string(REPLACE ";" "\\;" value "${${variable}}")
            list(APPEND toolchain_args "-D${variable}=${value}")
        endif()
    endforeach()
    string(SHA256 qt_key "6.12.0;${qtbase_SOURCE_DIR};${toolchain_args};${qt_args};${BUILD_SHARED_LIBS};${CEMU_QT_BUILD_TYPE};${CMAKE_GENERATOR};${CMAKE_GENERATOR_PLATFORM};${CMAKE_GENERATOR_TOOLSET}")
    string(SUBSTRING "${qt_key}" 0 16 qt_key)
    set(qt_prefix "${CMAKE_BINARY_DIR}/_deps/qt-${qt_key}")
    set(qt_build "${CMAKE_BINARY_DIR}/_deps/qt-build-${qt_key}")

    if(NOT EXISTS "${qt_prefix}/.cemu-qt-installed")
        set(generator_args -G "${CMAKE_GENERATOR}")
        if(CMAKE_GENERATOR_PLATFORM)
            list(APPEND generator_args -A "${CMAKE_GENERATOR_PLATFORM}")
        endif()
        if(CMAKE_GENERATOR_TOOLSET)
            list(APPEND generator_args -T "${CMAKE_GENERATOR_TOOLSET}")
        endif()
        message(STATUS "Bootstrapping Qt 6.12.0 into ${qt_prefix}")
        execute_process(
            COMMAND "${CMAKE_COMMAND}" -S "${qtbase_SOURCE_DIR}" -B "${qt_build}"
                ${generator_args} ${toolchain_args} ${qt_args}
                "-DCMAKE_BUILD_TYPE=${CEMU_QT_BUILD_TYPE}"
                "-DCMAKE_INSTALL_PREFIX=${qt_prefix}" -DCMAKE_INSTALL_LIBDIR=lib
                "-DBUILD_SHARED_LIBS=${BUILD_SHARED_LIBS}"
            RESULT_VARIABLE result
            COMMAND_ECHO STDOUT
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Qt configuration failed; see ${qt_build}")
        endif()
        execute_process(
            COMMAND "${CMAKE_COMMAND}" --build "${qt_build}" --target install
                --config "${CEMU_QT_BUILD_TYPE}" --parallel
            RESULT_VARIABLE result
            COMMAND_ECHO STDOUT
        )
        if(NOT result EQUAL 0)
            message(FATAL_ERROR "Qt build/install failed; see ${qt_build}")
        endif()
        file(WRITE "${qt_prefix}/.cemu-qt-installed" "${qt_key}\n")
    endif()

    # Avoid cached paths to a different system or bootstrapped Qt.
    get_cmake_property(cache_variables CACHE_VARIABLES)
    foreach(variable IN LISTS cache_variables)
        if(variable MATCHES "^(Qt6.*|QT)_DIR$")
            unset("${variable}" CACHE)
        endif()
    endforeach()
    list(PREPEND CMAKE_PREFIX_PATH "${qt_prefix}")
    set(CMAKE_PREFIX_PATH "${CMAKE_PREFIX_PATH}" PARENT_SCOPE)
    set(Qt6_DIR "${qt_prefix}/lib/cmake/Qt6" CACHE PATH "CPM Qt prefix" FORCE)
endfunction()

# CPM libraries stay static, including with shared Qt. Explicit system providers
# use the linkage supplied by the installed package.
function(cemu_add_zlib)
    if(CEMU_ZLIB_PROVIDER STREQUAL "system")
        find_package(ZLIB REQUIRED)
        return()
    endif()
    CPMAddPackage(
        NAME zlib VERSION 1.3.2
        URL https://codeload.github.com/madler/zlib/tar.gz/refs/tags/v1.3.2
        URL_HASH SHA512=16fea4df307a68cf0035858abe2fd550250618a97590e202037acd18a666f57afc10f8836cbbd472d54a0e76539d0e558cb26f059d53de52ff90634bbf4f47d4
        EXCLUDE_FROM_ALL YES SYSTEM YES
        OPTIONS "ZLIB_BUILD_SHARED OFF" "ZLIB_BUILD_STATIC ON"
                "ZLIB_BUILD_TESTING OFF" "ZLIB_INSTALL OFF"
    )
    add_library(ZLIB::ZLIB ALIAS zlibstatic)
    # libpng's generated-header step and libarchive also need the legacy
    # variables. Supply them through CPM's find_package redirect so a system
    # zlib cannot be mixed with the headers/target built here.
    file(WRITE "${CMAKE_FIND_PACKAGE_REDIRECTS_DIR}/zlib-extra.cmake"
        "set(ZLIB_INCLUDE_DIR [==[${zlib_SOURCE_DIR};${zlib_BINARY_DIR}]==])\n"
        "set(ZLIB_INCLUDE_DIRS \"\${ZLIB_INCLUDE_DIR}\")\n"
        "set(ZLIB_LIBRARIES ZLIB::ZLIB)\n"
        "set(ZLIB_VERSION 1.3.2)\nset(ZLIB_VERSION_STRING 1.3.2)\n"
    )
endfunction()

function(cemu_add_libpng)
    add_library(cemu_png INTERFACE)
    add_library(CEmu::PNG ALIAS cemu_png)
    if(CEMU_LIBPNG_PROVIDER STREQUAL "system")
        find_package(PNG REQUIRED)
        include(CheckCSourceCompiles)
        # A static try-compile checks headers/declarations without trying to
        # link a CPM zlib target that is not built until the main build step.
        set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)
        set(CMAKE_REQUIRED_INCLUDES "${PNG_INCLUDE_DIRS}")
        unset(CEMU_SYSTEM_PNG_HAS_APNG CACHE)
        check_c_source_compiles("
            #include <png.h>
            #if !defined(PNG_READ_APNG_SUPPORTED) || !defined(PNG_WRITE_APNG_SUPPORTED)
            #error libpng must support APNG read and write
            #endif
            int main(void) {
                (void)&png_set_acTL;
                (void)&png_write_frame_head;
                (void)&png_write_frame_tail;
                return 0;
            }" CEMU_SYSTEM_PNG_HAS_APNG)
        if(CEMU_SYSTEM_PNG_HAS_APNG)
            message(STATUS "APNG capture: system libpng with APNG support")
        else()
            message(WARNING "System libpng lacks APNG support. Animated capture is disabled; static screenshots remain available.")
        endif()
        target_link_libraries(cemu_png INTERFACE PNG::PNG)
        set_property(TARGET cemu_png PROPERTY CEMU_HAS_APNG "${CEMU_SYSTEM_PNG_HAS_APNG}")
        return()
    endif()
    find_package(Git REQUIRED)
    CPMAddPackage(
        NAME libpng VERSION 1.6.59
        URL https://codeload.github.com/pnggroup/libpng/tar.gz/refs/tags/v1.6.59
        URL_HASH SHA512=2d753e764846a0a75a10f2a00c6f4ae23d2ae37237c69d59d5e619da3f143d59f4ea9ce29c1fb39435df1cef77877c83b042a2275934c353a6603061da2d2dd9
        # Do not let git discover CEmu above a cache located inside its worktree.
        PATCH_COMMAND "${CMAKE_COMMAND}" -E env "GIT_CEILING_DIRECTORIES=<SOURCE_DIR>/.."
            "${GIT_EXECUTABLE}" -C "<SOURCE_DIR>" apply --ignore-space-change --whitespace=nowarn
            "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/patches/libpng-1.6.59-apng.patch"
        # Use normal include paths so CPATH (e.g. Homebrew on macOS runners)
        # cannot select an unpatched system png.h before this APNG build.
        EXCLUDE_FROM_ALL YES SYSTEM NO
        OPTIONS "PNG_SHARED OFF" "PNG_STATIC ON" "PNG_TESTS OFF"
                "PNG_TOOLS OFF" "PNG_FRAMEWORK OFF" "SKIP_INSTALL_ALL ON"
    )
    file(STRINGS "${libpng_SOURCE_DIR}/png.h" apng_define REGEX "^#define PNG_WRITE_APNG_SUPPORTED")
    if(NOT apng_define)
        message(FATAL_ERROR "CPM libpng must include the APNG patch (PNG_WRITE_APNG_SUPPORTED is missing)")
    endif()
    message(STATUS "APNG capture: pinned libpng 1.6.59 with APNG read/write support")

    target_link_libraries(cemu_png INTERFACE png_static)
    set_property(TARGET cemu_png PROPERTY CEMU_HAS_APNG TRUE)
endfunction()

function(cemu_add_libusb)
    if(CEMU_LIBUSB_PROVIDER STREQUAL "system")
        find_package(PkgConfig REQUIRED)
        pkg_check_modules(CEMU_SYSTEM_LIBUSB REQUIRED IMPORTED_TARGET libusb-1.0>=1.0.16)
        add_library(CEmu::LibUSB ALIAS PkgConfig::CEMU_SYSTEM_LIBUSB)
        return()
    endif()
    CPMAddPackage(
        NAME libusb VERSION 1.0.30
        GIT_REPOSITORY https://github.com/libusb/libusb-cmake.git
        GIT_TAG v1.0.30-0
        EXCLUDE_FROM_ALL YES SYSTEM YES
        OPTIONS "LIBUSB_BUILD_SHARED_LIBS OFF" "LIBUSB_BUILD_TESTING OFF"
                "LIBUSB_BUILD_EXAMPLES OFF" "LIBUSB_INSTALL_TARGETS OFF"
    )

    add_library(CEmu::LibUSB ALIAS usb-1.0)
endfunction()

function(cemu_add_libarchive)
    if(CEMU_LIBARCHIVE_PROVIDER STREQUAL "system")
        find_package(LibArchive 3.0 REQUIRED)
        add_library(CEmu::LibArchive ALIAS LibArchive::LibArchive)
        return()
    endif()
    if(WIN32 AND CEMU_ZLIB_PROVIDER STREQUAL "cpm")
        # The CPM zlib target is static, uses the C calling convention, and
        # has not been built yet during libarchive's configure-time probes.
        set(ZLIB_WINAPI OFF)
        set(ZLIB_DLL OFF)
        set(WITHOUT_ZLIB_DLL ON)
    endif()
    CPMAddPackage(
        NAME libarchive VERSION 3.8.11
        URL https://github.com/libarchive/libarchive/releases/download/v3.8.11/libarchive-3.8.11.tar.xz
        URL_HASH SHA256=a3dc0872b1b39da34dc18ef1b944f0f11ffd37425d70ae04be679279f1cf43d2
        EXCLUDE_FROM_ALL YES SYSTEM YES
        OPTIONS "ENABLE_TEST OFF" "ENABLE_INSTALL OFF" "ENABLE_WERROR OFF"
                "ENABLE_TAR OFF" "ENABLE_CPIO OFF" "ENABLE_CAT OFF" "ENABLE_UNZIP OFF"
                "ENABLE_OPENSSL OFF" "ENABLE_MBEDTLS OFF" "ENABLE_NETTLE OFF"
                "ENABLE_LIBB2 OFF" "ENABLE_LZ4 OFF" "ENABLE_LZO OFF" "ENABLE_LZMA OFF"
                "ENABLE_ZSTD OFF" "ENABLE_BZip2 OFF" "ENABLE_LIBXML2 OFF" "ENABLE_EXPAT OFF"
                "ENABLE_XATTR OFF" "ENABLE_ACL OFF" "ENABLE_ICONV OFF" "ENABLE_ZLIB ON"
    )
    target_compile_definitions(archive_static INTERFACE LIBARCHIVE_STATIC)

    add_library(CEmu::LibArchive ALIAS archive_static)
endfunction()

function(cemu_add_lua)
    if(CEMU_LUA_PROVIDER STREQUAL "system")
        find_package(Lua 5.4 REQUIRED)
        add_library(cemu_lua INTERFACE)
        target_link_libraries(cemu_lua INTERFACE ${LUA_LIBRARIES})
        set(lua_include_dir "${LUA_INCLUDE_DIR}")
    else()
        # Lua does not ship a CMake project. Build its library sources directly,
        # excluding the lua/luac command-line programs.
        CPMAddPackage(
            NAME lua VERSION 5.5.1
            URL https://www.lua.org/ftp/lua-5.5.1.tar.gz
            URL_HASH SHA512=1c8cb3d7194732d7685f80a30e0804cb24dc6aee1f8d1383bb9fd5d0699b84c70b823e686555dfa8fd61a32f32c0c687a8419ff4aa285f39d4f3bf3d5cd635c0
            DOWNLOAD_ONLY YES
        )
        file(GLOB lua_sources CONFIGURE_DEPENDS "${lua_SOURCE_DIR}/src/*.c")
        list(REMOVE_ITEM lua_sources "${lua_SOURCE_DIR}/src/lua.c" "${lua_SOURCE_DIR}/src/luac.c")
        add_library(cemu_lua STATIC ${lua_sources})
        target_include_directories(cemu_lua PRIVATE "${lua_SOURCE_DIR}/src")
        if(APPLE)
            target_compile_definitions(cemu_lua PRIVATE LUA_USE_MACOSX)
        elseif(UNIX)
            target_compile_definitions(cemu_lua PRIVATE LUA_USE_LINUX)
            target_link_libraries(cemu_lua PUBLIC ${CMAKE_DL_LIBS} m)
        endif()
        set(lua_include_dir "${lua_SOURCE_DIR}/src")
    endif()
    # sol2 prefers <lua/lua.h> whenever that spelling exists anywhere on the
    # include path. Provide it ourselves so a system Lua cannot win that probe.
    foreach(header IN ITEMS lua.h luaconf.h lauxlib.h lualib.h)
        configure_file("${lua_include_dir}/${header}"
            "${CMAKE_CURRENT_BINARY_DIR}/lua-include/lua/${header}" COPYONLY)
    endforeach()
    if(EXISTS "${lua_include_dir}/lua.hpp")
        configure_file("${lua_include_dir}/lua.hpp"
            "${CMAKE_CURRENT_BINARY_DIR}/lua-include/lua/lua.hpp" COPYONLY)
    endif()
    target_include_directories(cemu_lua SYSTEM BEFORE INTERFACE
        "${CMAKE_CURRENT_BINARY_DIR}/lua-include" "${lua_include_dir}")
    add_library(CEmu::Lua ALIAS cemu_lua)
endfunction()

function(cemu_add_sol2)
    if(CEMU_SOL2_PROVIDER STREQUAL "system")
        find_package(sol2 3.5 CONFIG REQUIRED)
    else()
        find_package(Git REQUIRED)
        set(sol2_patch "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/patches/sol2-3.5.0-lua-5.5.patch")
        file(SHA256 "${sol2_patch}" sol2_patch_hash)
        CPMAddPackage(
            NAME sol2 VERSION 3.5.0
            # Invalidate entries populated by the old patch command, which could skip files.
            CUSTOM_CACHE_KEY "3.5.0-${sol2_patch_hash}-apply2"
            URL https://codeload.github.com/ThePhD/sol2/tar.gz/refs/tags/v3.5.0
            URL_HASH SHA512=5a6ec7e16dae05ad6abea02842f62db8f64935eda438d67b2c264cbee80cee6d82200bd060387c6df837fe9f212dbe22b2772af34df1ce8bd43296dd9429558d
            # Lua 5.5 compatibility, including khaneliman's seed and GC fixes in PR #1723.
            PATCH_COMMAND "${CMAKE_COMMAND}" -E env "GIT_CEILING_DIRECTORIES=<SOURCE_DIR>/.."
                "${GIT_EXECUTABLE}" -C "<SOURCE_DIR>" apply --ignore-space-change --whitespace=nowarn
                "${sol2_patch}"
            EXCLUDE_FROM_ALL YES SYSTEM YES
            OPTIONS "SOL2_BUILD_LUA OFF" "SOL2_ENABLE_INSTALL OFF"
        )
        file(STRINGS "${sol2_SOURCE_DIR}/include/sol/compatibility/compat-5.3.h" sol2_lua55_guard
            REGEX "LUA_VERSION_NUM > 505")
        file(STRINGS "${sol2_SOURCE_DIR}/include/sol/state.hpp" sol2_lua55_seed
            REGEX "luaL_makeseed")
        file(STRINGS "${sol2_SOURCE_DIR}/include/sol/state_view.hpp" sol2_lua55_gc
            REGEX "LUA_GCPARAM")
        if(NOT sol2_lua55_guard OR NOT sol2_lua55_seed OR NOT sol2_lua55_gc)
            message(FATAL_ERROR "CPM sol2 is missing the Lua 5.5 compatibility patch. Remove its cached source and reconfigure.")
        endif()
    endif()
    add_library(cemu_sol2 INTERFACE)
    target_link_libraries(cemu_sol2 INTERFACE sol2::sol2)
    add_library(CEmu::Sol2 ALIAS cemu_sol2)
endfunction()

function(cemu_add_dependencies)
    set(BUILD_SHARED_LIBS OFF)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    cemu_add_zlib()
    cemu_add_libpng()
    cemu_add_libusb()
    cemu_add_libarchive()
    cemu_add_lua()
    cemu_add_sol2()
endfunction()

cemu_add_dependencies()
cemu_add_qt()

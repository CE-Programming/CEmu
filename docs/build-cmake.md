# Building with CMake and CPM

The Qt frontend requires CMake 3.24 or newer, Git (including the zdis
submodule), a C11/C++20 compiler, and a build tool such as Ninja. CPM.cmake
0.43.2 is vendored in `gui/qt/cmake/CPM.cmake`; no package-manager toolchain is
needed. The first configure downloads sources and builds/installs Qt into the
build directory, so it takes considerably longer than subsequent configures.

```sh
git submodule update --init --recursive
cmake -S gui/qt -B gui/qt/CEmu.build/local -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build gui/qt/CEmu.build/local --parallel
```

CPM builds Qt 6.12.0, zlib 1.3.2, libpng 1.6.59 with the matching APNG patch,
libusb 1.0.30, libarchive 3.8.11, Lua 5.5.1, and sol2 3.5.0. The auxiliary
CPM libraries are static; `BUILD_SHARED_LIBS=ON` selects a shared Qt build.
System dependencies keep the linkage supplied by their installed packages. Lua
scripting, USB passthrough, ZIP/CE bundle extraction, and APNG recording are
enabled by the CMake build. These pins match the latest stable upstream
releases/tags checked on 2026-10-30. Sol2 3.5.0 receives a Lua 5.5 compatibility
patch based on [sol2 PR #1723](https://github.com/ThePhD/sol2/pull/1723), including
khaneliman's [allocator seed fix](https://github.com/ThePhD/sol2/pull/1723#discussion_r4239188783)
and [GC wrapper fixes](https://github.com/khaneliman/sol2/commit/df8744a81f855c07a5bfeaceb5aa2288b29d8030).
Custom allocators use Lua's default seed generator on 5.5; explicit seeds
remain unchanged. The GC wrappers use Lua 5.5's parameter API and byte-sized
step budgets while preserving sol2's kilobyte and logarithmic step-size units.
Zero tuning parameters retain their existing values; negative GC step budgets
are unsupported on Lua 5.5. The Lua 5.4 code paths are preserved. The sol2 CPM
source cache key includes the patch content, so editing it refreshes cached
sources.

The CPM build applies the vendored APNG patch. With system libpng, CMake
checks APNG read/write macros and declarations. If they are missing, it warns
and disables animated capture; static screenshots remain available through
Qt. An APNG-enabled system libpng enables both capture modes. The older qmake
build and its libpng helper script are independent of this CMake setup.

## Platform prerequisites and presets

- **Windows:** Visual Studio 2022 or newer with C++ and the Windows SDK, Git,
  CMake, Ninja, and Perl. Run from an MSVC developer shell for the desired
  architecture. Use `Win-x64` or `Win-arm64`.
- **macOS:** Xcode command-line tools, CMake, Ninja, and pkg-config. The
  source-built Qt defaults to macOS 13.0 or newer. Use `Mac-x64` or
  `Mac-arm64`; these explicitly set `CMAKE_OSX_ARCHITECTURES`. Building on a
  matching host architecture is recommended.
- **Linux:** a C/C++ toolchain, CMake, Ninja, Git, Perl, pkg-config, and the
  development headers for DBus, Fontconfig/FreeType, X11/XCB, xkbcommon, and
  udev. The complete Ubuntu package list is in
  `.github/workflows/build.linux.workflow.yml`. Use `Linux-x64` or
  `Linux-x64-Dynamic` (shared Qt).

Run presets from `gui/qt`, for example:

```sh
cd gui/qt
cmake --preset Mac-arm64
cmake --build --preset Mac-arm64-Release --parallel
```

The existing preset names and Debug/Release/RelWithDebInfo build presets are
retained. CPM presets use `CEmu.build/cpm/<preset>` to avoid reusing old
CMake caches. Switch an existing manual build to a new directory too; an old
`CMAKE_TOOLCHAIN_FILE` remains cached even after changing the source files.

Qt is bootstrapped in Release for multi-config generators by default.
`CEMU_QT_BUILD_TYPE=Debug` selects a debug Qt instead. On MSVC, CEmu and all
CPM libraries use the CRT matching the selected Qt configuration and linkage.
Installed system libraries must also use a compatible CRT.
For a single-config generator, the initial `CMAKE_BUILD_TYPE` also supplies
the default Qt build type. Set `CEMU_QT_BUILD_TYPE` explicitly when changing
it later in an existing build directory.

## Use an installed Qt

For faster development builds, keep Qt external while CPM builds all other
libraries (including APNG libpng):

```sh
cmake -S gui/qt -B gui/qt/CEmu.build/system-qt -G Ninja \
    -DCEMU_QT_PROVIDER=system -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_PREFIX_PATH=/path/to/Qt/6.x/platform
cmake --build gui/qt/CEmu.build/system-qt --parallel
```

With system Qt on MSVC, set `BUILD_SHARED_LIBS=ON` for the usual shared Qt/CRT
installation, or leave it off for a static Qt installation. Qt Core, Gui,
Network, and Widgets are required; DBus is used when available on Unix.

## Use system dependencies

`CEMU_DEPENDENCY_PROVIDER` sets the default provider for a new build directory.
It defaults to `cpm`; set it to `system` for a fully system build. Each
library also has an independent override:

| Cache variable | System lookup |
| --- | --- |
| `CEMU_QT_PROVIDER` | Qt6 Core, Gui, Network, Widgets; optional DBus |
| `CEMU_ZLIB_PROVIDER` | CMake `ZLIB` package |
| `CEMU_LIBPNG_PROVIDER` | CMake `PNG` package; APNG support enables animated capture |
| `CEMU_LIBUSB_PROVIDER` | pkg-config `libusb-1.0 >= 1.0.16` |
| `CEMU_LIBARCHIVE_PROVIDER` | CMake `LibArchive >= 3.0` package |
| `CEMU_LUA_PROVIDER` | CMake `Lua >= 5.4` package |
| `CEMU_SOL2_PROVIDER` | CMake `sol2 >= 3.5` config package |

All values are `cpm` or `system`. An explicitly selected system package is
required: CMake does not silently download a replacement. A fully system
configuration does not load CPM or download dependencies. Standard discovery
hints such as `CMAKE_PREFIX_PATH`, `ZLIB_ROOT`, `PNG_ROOT`, `Lua_ROOT`,
`sol2_DIR`, and `PKG_CONFIG_PATH` can select non-default installations.

For example, use installed libusb/libarchive while CPM supplies the other
auxiliary libraries, including the patched libpng:

```sh
cmake -S gui/qt -B gui/qt/CEmu.build/mixed -G Ninja \
    -DCEMU_QT_PROVIDER=system \
    -DCEMU_LIBUSB_PROVIDER=system -DCEMU_LIBARCHIVE_PROVIDER=system
cmake --build gui/qt/CEmu.build/mixed --parallel
```

Or use system packages everywhere:

```sh
cmake -S gui/qt -B gui/qt/CEmu.build/distro -G Ninja \
    -DCEMU_DEPENDENCY_PROVIDER=system \
    -DCMAKE_PREFIX_PATH="/path/to/Qt"
cmake --build gui/qt/CEmu.build/distro --parallel
```

If the distro supplies only ordinary libpng, static screenshots still work.
For animated capture, provide an APNG-enabled system libpng or add
`-DCEMU_LIBPNG_PROVIDER=cpm` to build the patched dependency. For Lua 5.5,
system sol2 must contain the compatibility fixes described above; use
`-DCEMU_SOL2_PROVIDER=cpm` if its installed package lacks them. System Lua's
headers and libraries must come from the same version.

Provider choices are cached independently. To change the global default
later, use a fresh build directory or explicitly set each existing provider
variable. Presets may also set individual providers explicitly.

## Caching, installation, and dependency updates

Set `CPM_SOURCE_CACHE` in the environment or as a CMake cache variable to
share downloaded sources between build directories. The CI workflows cache
these sources plus the installed Qt sub-build. Changes to compiler,
architecture, deployment target, Qt options, or linkage select a new Qt
prefix. A failed Qt configure/build does not leave a successful install stamp.

```sh
cmake --install gui/qt/CEmu.build/local --prefix /path/to/stage
```

Qt's deployment helper runs during installation where supported, including
copying shared Qt frameworks/plugins into macOS bundles. Build outputs and
staged files remain local unless explicitly published.

Dependency versions and archive hashes live in
`gui/qt/cmake/CEmuDependencies.cmake`. Update the CPM libpng together with its APNG
patch and run capture tests against that exact library; do not substitute an
unpatched libpng. The patch is from
<https://downloads.sourceforge.net/project/libpng-apng/libpng16/1.6.59/libpng-1.6.59-apng.patch.gz>.
Its downloaded gzip SHA512 is
`a27ed2769b3c8d1703267b807663aa8e543b62355237e223d7731662034dd126951b88117c1038dc976750cf515d7e0e26e4a9278bf951f96ec4ed8f74105db7`,
and the vendored uncompressed patch retains libpng's licensing terms in the
patched sources.

#!/bin/bash
# CEmu libpng-apng Build Script
# 
# Prerequisites:
#   - BASH to run this script. (If you can run it, chances are that
#     you have it!)
#   - curl to download packages.
#   - Basic UNIX utilities - sed, grep, sort, tar, rm
#   - Clang/GCC + Make installed. For Mac OS X, you may get this
#     installed via Xcode - see here:
#     https://stackoverflow.com/a/9353468/1094484
#   - Possibly root access via sudo. For Mac OS X, see here for more
#     details: https://support.apple.com/en-us/HT204012
#     
#     ** Note that you may be able to just point the compiler to the
#        libpng-apng folder to compile with, hence not requiring any
#        root access to install anything at all. If this is the case,
#        simply press CTRL-C when prompted for a password and you're
#        good to go.
# 
# How to run:
#   1. Download the raw version of this script and save it to
#      somewhere you know.
#   2. Open Terminal, and then cd to your folder.
#   3. chmod +x ./get_libpng-apng.sh
#   4. ./get_libpng-apng.sh
# 
becho() { echo -e "\033[1m$@\033[0m"; }
cerr() { becho "An error occurred. Please see output above for details." && exit 1; }

LIBPNG_VERSION_CONSTANT="1.6.34"

becho " ** Detecting latest version of libpng..."

LIBPNG_VERSION=$(curl -L -s 'https://sourceforge.net/p/libpng/code/ref/master/tags/' |
                     sed -n 's,.*<a[^>]*>v\([0-9][^<]*\)<.*,\1,p' |
                     grep -v alpha | grep -v beta | grep -v rc | 
                     sort -t. -s -k 1,1nr -k 2,2nr -k 3,3nr -k 4,4nr | head -n1 | tail -1)

([ ! "$?" = "0" ] || [ -z "$LIBPNG_VERSION" ]) && becho "Couldn't autodetect latest libpng, using v$LIBPNG_VERSION_CONSTANT." && LIBPNG_VERSION="$LIBPNG_VERSION_CONSTANT"

becho "Will install: libpng-apng v${LIBPNG_VERSION}"
becho "Starting in 3s (press CTRL-C to abort)"
sleep 3

# Download libpng
becho " ** Downloading libpng..."
curl -s -L -O "https://downloads.sourceforge.net/sourceforge/libpng/libpng-${LIBPNG_VERSION}.tar.xz" || cerr

becho " ** Extracting libpng..."
tar -xJf libpng-${LIBPNG_VERSION}.tar.xz || cerr

becho " ** Cleaning up libpng archive..."
rm -f libpng-${LIBPNG_VERSION}.tar.xz || cerr

# Download apng patch
becho " ** Downloading libpng-apng patch..."
curl -s -L -O "https://downloads.sourceforge.net/sourceforge/libpng-apng/libpng-${LIBPNG_VERSION}-apng.patch.gz" || cerr

becho " ** Decompressing libpng-apng patch..."
gzip -d libpng-${LIBPNG_VERSION}-apng.patch.gz || cerr

# Apply apng patch
becho " ** Applying libpng-apng patch..."
cd libpng-${LIBPNG_VERSION} || cerr
patch -Np1 -i ../libpng-${LIBPNG_VERSION}-apng.patch || cerr

becho " ** Cleaning up libpng-apng patch..."
rm -f ../libpng-${LIBPNG_VERSION}-apng.patch || cerr

# Build libpng-apng
becho " ** Configuring libpng-apng..."
./configure --with-libpng-prefix=a --enable-static --disable-shared CFLAGS="-O2 -fPIC" || cerr

becho " ** Building libpng-apng..."
make -j2 || cerr

cd ..

becho " ** Renaming folder to libpng-apng"
mv "libpng-${LIBPNG_VERSION}" "libpng-apng"

becho " ** All done!"


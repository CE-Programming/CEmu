#!/bin/bash

LIBPNG_VERSION=1.6.34

# Download libpng
wget "https://downloads.sourceforge.net/sourceforge/libpng/libpng-${LIBPNG_VERSION}.tar.xz"
tar -xJf libpng-${LIBPNG_VERSION}.tar.xz
rm -f libpng-${LIBPNG_VERSION}.tar.xz

# Download apng patch
wget "https://downloads.sourceforge.net/sourceforge/libpng-apng/libpng-${LIBPNG_VERSION}-apng.patch.gz"
gzip -d libpng-${LIBPNG_VERSION}-apng.patch.gz

# Apply apng patch
cd libpng-${LIBPNG_VERSION}
patch -Np1 -i ../libpng-${LIBPNG_VERSION}-apng.patch
rm -f ../libpng-${LIBPNG_VERSION}-apng.patch

# Change libname to avoid conflicts
sed -i.bak -e "s/LIBNAME\=\'PNG\@PNGLIB/LIBNAME\=\'APNG\@PNGLIB/" Makefile.am
rm -f Makefile.am.bak

# Build libpng-apng
CFLAGS="-O2 -fPIC" ./configure --enable-static --disable-shared
make -j2
mv .libs/libpng*.a .libs/libpngapng.a

cd ..

# CEmu will expect this name
mv libpng-${LIBPNG_VERSION} libpng-apng

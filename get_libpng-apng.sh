#!/bin/bash

LIBPNG_VERSION=1.6.34
SF_DIR=libpng16

# Download libpng
wget -O libpng-${LIBPNG_VERSION}.tar.xz "http://prdownloads.sourceforge.net/libpng/libpng-${LIBPNG_VERSION}.tar.xz?download"
tar -xJf libpng-${LIBPNG_VERSION}.tar.xz
rm -f libpng-${LIBPNG_VERSION}.tar.xz

# The patch will need that
mv libpng-${LIBPNG_VERSION} libpng-${LIBPNG_VERSION}.org

# Download the APNG patch
wget -O libpng-${LIBPNG_VERSION}-apng.patch.gz https://sourceforge.net/projects/libpng-apng/files/${SF_DIR}/${LIBPNG_VERSION}/libpng-${LIBPNG_VERSION}-apng.patch.gz/download
gzip -d libpng-${LIBPNG_VERSION}-apng.patch.gz
patch -p0 < ./libpng-${LIBPNG_VERSION}-apng.patch
rm -f libpng-${LIBPNG_VERSION}-apng.patch

# CEmu will expect this
mv libpng-${LIBPNG_VERSION}.org libpng-apng

cd libpng-apng

# Change libname to avoid conflicts
sed -i.bak -e "s/LIBNAME\=\'PNG\@PNGLIB/LIBNAME\=\'APNG\@PNGLIB/" Makefile.am
rm -f Makefile.am.bak

# Build libpng-apng
autoreconf -vif
CFLAGS="-O2" ./configure
make -j2
mv .libs/libpng*.a .libs/libpngapng.a

cd ..

# Move the dir to the expected location
mv libpng-apng gui/qt/capture/

#!/bin/bash

LIBPNG_VERSION=$(wget -q -O- 'https://sourceforge.net/p/libpng/code/ref/master/tags/' |
                     sed -n 's,.*<a[^>]*>v\([0-9][^<]*\)<.*,\1,p' |
                     grep -v alpha | grep -v beta | grep -v rc | sort -V | tail -1)

# Download libpng
wget -q "https://downloads.sourceforge.net/sourceforge/libpng/libpng-${LIBPNG_VERSION}.tar.xz"
tar -xJf libpng-${LIBPNG_VERSION}.tar.xz
rm -f libpng-${LIBPNG_VERSION}.tar.xz

# Download apng patch
wget -q "https://downloads.sourceforge.net/sourceforge/libpng-apng/libpng-${LIBPNG_VERSION}-apng.patch.gz"
gzip -d libpng-${LIBPNG_VERSION}-apng.patch.gz

# Apply apng patch
cd libpng-${LIBPNG_VERSION}
patch -Np1 -i ../libpng-${LIBPNG_VERSION}-apng.patch
rm -f ../libpng-${LIBPNG_VERSION}-apng.patch

# Build libpng-apng
CFLAGS="-O2 -fPIC" ./configure --enable-static --disable-shared
make -j2
sudo make install

cd ..

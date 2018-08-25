#
# spec file for package CEmu
#
# Copyright (c) 2017 SUSE LINUX GmbH, Nuernberg, Germany.
#
# All modifications and additions to the file contributed by third parties
# remain the property of their copyright owners, unless otherwise agreed
# upon. The license for this file, and modifications and additions to the
# file, is the same license as for the pristine package itself (unless the
# license for the pristine package is not an Open Source License, in which
# case the license is the MIT License). An "Open Source License" is a
# license that conforms to the Open Source Definition (Version 1.9)
# published by the Open Source Initiative.

# Please submit bugfixes or comments via http://bugs.opensuse.org/
#


%global debug_package %{nil}
Name:           cemu
Version:        1.1
Release:        0
Summary:        TI-84 Plus CE / TI-83 Premium CE emulator
License:        GPL-3.0
Group:          Amusements/Teaching/Other
Url:            https://github.com/CE-Programming/CEmu
Source:         %{name}-%{version}.tar.xz

BuildRequires:  git
BuildRequires:  pkgconfig

%if 0%{?suse_version} > 1320
BuildRequires:  gcc-c++
%endif
%if 0%{?suse_version} == 1320
BuildRequires:  gcc49-c++
#!BuildIgnore: libgcc_s1
%endif

%if 0%{?sle_version} == 120100
# Leap 42.1 / SLE12SP2
BuildRequires:  gcc5-c++
%endif

%if 0%{?sle_version} == 120200
# Leap 42.2+ / SLE12SP2Backports
%if 0%{?is_opensuse}
BuildRequires:  gcc6-c++
#!BuildIgnore:  libasan3
%else
BuildRequires:  gcc5-c++
%endif
%endif

%if 0%{?sle_version} == 120300
# Leap 42.3+ / SLE12SP2Backports
%if 0%{?is_opensuse}
BuildRequires:  gcc6-c++
%else
BuildRequires:  gcc5-c++
%endif
%endif

%if 0%{?suse_version} == 1315
#!Buildignore:  libgcc_s1
%endif

BuildRequires:  zlib-devel
BuildRequires:  libarchive-devel
BuildRequires:  hicolor-icon-theme
Requires:       hicolor-icon-theme
%if  0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires:  qt5-qtbase-devel >= 5.5
BuildRequires:  qt5-qtdeclarative-devel
%else
BuildRequires:  libqt5-qtbase-devel >= 5.5
BuildRequires:  libqt5-qtdeclarative-devel
BuildRequires:  update-desktop-files
%endif

%description

%prep
%setup -q

%build
export CC=gcc
export CXX=g++
test -x "$(type -p gcc-4.9)" && export CC=gcc-4.9
test -x "$(type -p g++-4.9)" && export CXX=g++-4.9
test -x "$(type -p gcc-5)"   && export CC=gcc-5
test -x "$(type -p g++-5)"   && export CXX=g++-5
test -x "$(type -p gcc-6)"   && export CC=gcc-6
test -x "$(type -p g++-6)"   && export CXX=g++-6
test -x "$(type -p gcc-7)"   && export CC=gcc-7
test -x "$(type -p g++-7)"   && export CXX=g++-7
cd gui/qt/capture/libpng-apng && ./configure --with-libpng-prefix=a --enable-static --disable-shared CFLAGS="-O2 -fPIC" && make libpng16.la && cd ../../../..
qmake-qt5 QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" gui/qt/CEmu.pro CEMU_VERSION=v1.1 USE_LIBPNG=internal TARGET_NAME=cemu
make %{?_smp_mflags}

%install
make %{?_smp_mflags} DESTDIR=%{buildroot} install INSTALL_ROOT=%{buildroot}

%files
%{_bindir}/cemu

%changelog

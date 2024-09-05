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
Version:        2.0
Release:        0
Summary:        TI-84 Plus CE / TI-83 Premium CE emulator
License:        GPL-3.0
Group:          Amusements/Teaching/Other
Url:            https://github.com/CE-Programming/CEmu
Source:         %{name}-%{version}.tar.xz

BuildRequires:  git
BuildRequires:  pkgconfig

%if 0%{?suse_version} > 1600
BuildRequires:  gcc14-c++
%else
%if 0%{?sle_version} == 150600
BuildRequires:  gcc13-c++
%else
%if 0%{?sle_version} == 150500
BuildRequires:  gcc12-c++
%else
BuildRequires:  gcc-c++
%endif
%endif
%endif

BuildRequires:  autoconf
BuildRequires:  automake
BuildRequires:  libtool
BuildRequires:  zlib-devel
BuildRequires:  libarchive-devel
BuildRequires:  hicolor-icon-theme
Requires:       hicolor-icon-theme
%if  0%{?fedora_version}
BuildRequires:  openssl-devel
BuildRequires:  llvm-libs
%endif
%if  0%{?fedora_version} || 0%{?rhel_version} || 0%{?centos_version}
BuildRequires:  qt5-qtbase-devel >= 5.7
BuildRequires:  qt5-qtdeclarative-devel
%else
BuildRequires:  libqt5-qtbase-devel >= 5.7
BuildRequires:  libqt5-qtdeclarative-devel
BuildRequires:  update-desktop-files
%endif

%description
Third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator,
focused on developer features.

%prep
%setup -q

%build
export CC=gcc
export CXX=g++
test -x "$(type -p gcc-8)"   && export CC=gcc-8
test -x "$(type -p g++-8)"   && export CXX=g++-8
test -x "$(type -p gcc-9)"   && export CC=gcc-9
test -x "$(type -p g++-9)"   && export CXX=g++-9
test -x "$(type -p gcc-10)"   && export CC=gcc-10
test -x "$(type -p g++-10)"   && export CXX=g++-10
test -x "$(type -p gcc-11)"   && export CC=gcc-11
test -x "$(type -p g++-11)"   && export CXX=g++-11
test -x "$(type -p gcc-12)"   && export CC=gcc-12
test -x "$(type -p g++-12)"   && export CXX=g++-12
test -x "$(type -p gcc-13)"   && export CC=gcc-13
test -x "$(type -p g++-13)"   && export CXX=g++-13
test -x "$(type -p gcc-14)"   && export CC=gcc-14
test -x "$(type -p g++-14)"   && export CXX=g++-14
cd gui/qt/capture/libpng-apng && autoreconf -vif && ./configure --with-libpng-prefix=a --enable-static --disable-shared CFLAGS="-O2 -fPIC" && make libpng16.la && cd ../../../..
qmake-qt5 QMAKE_CC="$CC" QMAKE_CXX="$CXX" QMAKE_LINK="$CXX" gui/qt/CEmu.pro CEMU_VERSION=v2.0 USE_LIBPNG=internal TARGET_NAME=cemu
make %{?_smp_mflags}

%install
make %{?_smp_mflags} DESTDIR=%{buildroot} install INSTALL_ROOT=%{buildroot}

%files
%{_bindir}/cemu

%changelog

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
Version:        3.0
Release:        0
Summary:        TI-84 Plus CE / TI-83 Premium CE emulator
License:        GPL-3.0-or-later
Group:          Amusements/Teaching/Other
Url:            https://github.com/CE-Programming/CEmu
Source:         %{name}-%{version}.tar.xz

BuildRequires:  gcc-c++
BuildRequires:  git
BuildRequires:  cmake >= 3.24
BuildRequires:  pkgconfig
BuildRequires:  pkgconfig(Qt6Widgets) >= 6.2
BuildRequires:  pkgconfig(Qt6Network) >= 6.2
BuildRequires:  pkgconfig(zlib)
BuildRequires:  pkgconfig(libpng)
BuildRequires:  pkgconfig(libarchive)
BuildRequires:  pkgconfig(libusb-1.0)
BuildRequires:  cmake(sol2) >= 3.5
%if 0%{?suse_version}
BuildRequires:  ninja
BuildRequires:  lua54-devel
%else
BuildRequires:  ninja-build
BuildRequires:  lua-devel >= 5.4
BuildRequires:  lua-devel < 5.5
%endif
Requires:       hicolor-icon-theme
Requires:       shared-mime-info

%description
Third-party TI-84 Plus CE / TI-83 Premium CE calculator emulator,
focused on developer features.

%prep
%setup -q

%build
cmake -S gui/qt -B build -G Ninja \
    -DCMAKE_BUILD_TYPE=RelWithDebInfo \
    -DCMAKE_C_FLAGS="%{optflags}" -DCMAKE_CXX_FLAGS="%{optflags}" \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} -DCMAKE_INSTALL_LIBDIR=%{_lib} \
    -DCEMU_DEPENDENCY_PROVIDER=system -DBUILD_SHARED_LIBS=ON \
    -DCEMU_DEPLOY_RUNTIME=OFF -DCEMU_EXECUTABLE_NAME=cemu \
    -DSHORT_VERSION=v%{version}
cmake --build build -- %{?_smp_mflags}

%install
DESTDIR=%{buildroot} cmake --install build

%files
%license LICENSE
%{_bindir}/cemu
%{_datadir}/applications/cemu.desktop
%{_datadir}/mime/packages/cemu.xml
%{_datadir}/icons/hicolor/*/apps/cemu.png

%changelog

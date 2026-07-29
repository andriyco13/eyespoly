Name:           eyespoly
Version:        1.2.0
Release:        1%{?dist}
Summary:        Eye-care break reminder for Linux

License:        MIT
URL:            https://github.com/andriyco13/eyespoly
Source0:        %{url}/archive/refs/tags/v%{version}/eyespoly-%{version}.tar.gz

BuildRequires:  cmake
BuildRequires:  gcc-c++
BuildRequires:  qt6-qtbase-devel
BuildRequires:  qt6-qtdeclarative-devel
BuildRequires:  qt6-qtmultimedia-devel
BuildRequires:  qt6-qttools-devel
BuildRequires:  desktop-file-utils
BuildRequires:  libappstream-glib

Requires:       qt6-qtmultimedia
Requires:       qt6-qtdeclarative

%description
Eyespoly reminds you to take regular breaks to protect your eyesight
while working at the computer, following the well-known 20-20-20
method. It automatically pauses the timer when you step away from
the keyboard (idle detection), shows short guided eye-relaxation
exercises during each break, and keeps simple screen-time statistics.

%prep
%autosetup -n eyespoly-%{version}

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install

desktop-file-validate %{buildroot}%{_datadir}/applications/eyespoly.desktop

%files
%license LICENSE
%doc README.md
%{_bindir}/eyespoly
%{_datadir}/applications/eyespoly.desktop
%{_datadir}/icons/hicolor/256x256/apps/eyespoly.png
%{_datadir}/metainfo/eyespoly.metainfo.xml

%changelog
* Sun Jul 26 2026 andriyco13 <andriyco13@gmail.com> - 0.1.0-1
- Initial release

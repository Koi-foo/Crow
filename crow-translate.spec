Name: crow-translate
Version: 2.8.4
Release: alt1

License: GPL3
Group: System/Internationalization
Url: https://crow-translate.github.io
Packager: Evgeny Chuck <koi at altlinux.org>

Source: %name-%version.tar.gz
# https://github.com/crow-translate/crow-translate/archive/refs/tags/v%version.tar.gz

# Automatically added by buildreq on Tue Jun 01 2021 (-bi)
# optimized out: ca-trust cmake cmake-modules elfutils fontconfig-devel gcc-c++ glibc-kernheaders-generic glibc-kernheaders-x86 gtk-update-icon-cache leptonica libICE-devel libSM-devel libX11-devel libXScrnSaver-devel libXau-devel libXcomposite-devel libXcursor-devel libXdamage-devel libXdmcp-devel libXext-devel libXfixes-devel libXft-devel libXi-devel libXinerama-devel libXmu-devel libXpm-devel libXrandr-devel libXrender-devel libXt-devel libXtst-devel libXv-devel libXxf86misc-devel libXxf86vm-devel libfreetype-devel libglvnd-devel libgpg-error libqt5-concurrent libqt5-core libqt5-dbus libqt5-gui libqt5-multimedia libqt5-network libqt5-widgets libqt5-x11extras libqt5-xml libsasl2-3 libstdc++-devel libxcb-devel libxkbfile-devel pkg-config python-base python-modules qt5-base-devel qt5-tools sh4 tesseract xorg-proto-devel xorg-xf86miscproto-devel

BuildRequires: extra-cmake-modules
# To get source code from git repos
BuildRequires: git-core

BuildRequires: libleptonica-devel
BuildRequires: qt5-multimedia-devel
BuildRequires: qt5-tools-devel
BuildRequires: qt5-x11extras-devel
BuildRequires: tesseract-devel
# For tools downloaded from git
BuildRequires: zlib-devel
BuildRequires: bzlib-devel
BuildRequires: libpng-devel
BuildRequires: libpcre-devel
BuildRequires: libuuid-devel
BuildRequires: libexpat-devel

BuildRequires: libqt5-network
BuildRequires: libqt5-concurrent
BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: libqt5-widgets
BuildRequires: libqt5-dbus
BuildRequires(pre): rpm-macros-cmake

Requires: tesseract >= 4.0.0

Summary: A Qt GUI for Google, Yandex and Bing translators
Summary(ru_RU.UTF-8): Графический интерфейс Qt для переводчиков Google, Yandex и Bing

%description
A simple and lightweight translator that allows to translate and speak
text using Google, Yandex and Bing written with Qt5.

%description -l ru_RU.UTF-8
Простой и легкий переводчик, который позволяет переводить и произносить
текст с помощью Google, Yandex и Bing, написанный с помощью Qt5.

%prep
%setup

%build
%cmake
%cmake_build

%install
%cmakeinstall_std

%files
%doc README.md COPYING
%_bindir/crow
%_desktopdir/io.crow_translate.CrowTranslate.desktop
%_datadir/Crow*/*
%_iconsdir/hicolor/*/*/crow-translate*

%changelog
* Sat Jul 03 2021 Evgeny Chuck <koi@altlinux.org> 2.8.4-alt1
- initial build for ALT Sisyphus

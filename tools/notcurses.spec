Name: notcurses
Version: 1.2.5
Release: 1
Summary: Character graphics and TUI library
License: ASL 2.0
Source0: https://github.com/dankamongmen/%{name}/archive/v%{version}.tar.gz
Source1: https://github.com/dankamongmen/%{name}/releases/v%{version}/v%{version}.tar.gz.asc
Source2: https://dank.qemfd.net/dankamongmen.gpg
BuildRequires: gnupg2 cmake make gcc-c++ ncurses-devel pandoc python3-devel

%description
notcurses facilitates the creation of modern TUI programs,
making full use of Unicode and 24-bit direct color. It presents
an API similar to that of Curses, and rides atop Terminfo.

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%setup

%build
%cmake -DUSE_FFMPEG=off -DUSE_TEST=off .
%make_build

%install
%make_install

%files
/usr/bin/notcurses-demo
/usr/bin/notcurses-input
/usr/bin/notcurses-ncreel
/usr/bin/notcurses-pydemo
/usr/bin/notcurses-tester
/usr/bin/notcurses-tetris
/usr/bin/notcurses-view
/usr/include/notcurses/nckeys.h
/usr/include/notcurses/notcurses.h
/usr/lib/x86_64-linux-gnu/libnotcurses.so.{version}
/usr/lib/x86_64-linux-gnu/libnotcurses.so.1
/usr/lib/x86_64-linux-gnu/libnotcurses.so
/usr/lib/x86_64-linux-gnu/cmake/notcurses/notcursesConfig.cmake
/usr/lib/x86_64-linux-gnu/cmake/notcurses/notcursesConfigVersion.cmake
/usr/lib/x86_64-linux-gnu/libnotcurses.a
/usr/lib/x86_64-linux-gnu/pkgconfig
/usr/lib/x86_64-linux-gnu/pkgconfig/notcurses.pc
%license LICENSE

%changelog
* Tue, Apr 07 2020 Nick Black <dankamongmen@gmail.com> - 1.2.5-1
- Initial Fedora packaging

Name:          notcurses
Version:       1.2.8
Release:       1%{?dist}
Summary:       Character graphics and TUI library
License:       ASL 2.0
URL:           https://nick-black.com/dankwiki/index.php/Notcurses
Source0:       https://github.com/dankamongmen/%{name}/archive/v%{version}.tar.gz
Source1:       https://github.com/dankamongmen/%{name}/releases/download/v%{version}/v%{version}.tar.gz.asc
Source2:       https://dank.qemfd.net/dankamongmen.gpg

BuildRequires: gnupg2
BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: pandoc
BuildRequires: python3-devel
BuildRequires: python3-setuptools
BuildRequires: pkgconfig(ncurses)

%description
notcurses facilitates the creation of modern TUI programs,
making full use of Unicode and 24-bit TrueColor. It presents
an API similar to that of Curses, and rides atop Terminfo.

%package devel
Summary:       Development files for the notcurses library
License:       ASL 2.0
Requires:      "%{name}%{?_isa} = %{version}-%{release}"

%description devel
Development files for the notcurses library.

%package static
Summary:       Static library for the notcurses library
License:       ASL 2.0
Requires:      %{name}-devel = %{version}-%{release}

%description static
A statically-linked version of the notcurses library.

%package -n python3-%{srcname}
Summary:       Python wrappers for notcurses
License:       ASL 2.0
Requires:      "%{name}%{?_isa} = %{version}-%{release}"
%{?python_provide:%python_provide python3-%{srcname}}

%description -n python3-%{srcname}
Python wrappers and a demonstration script for the notcurses library.

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%autosetup

# Tests have been disabled due to absence of doctest in Fedora (as of F32)
%build
%cmake -DUSE_FFMPEG=off -DUSE_TESTS=off .
%make_build

%install
%make_install

%files
%doc CHANGELOG.md README.md
%license COPYRIGHT LICENSE
%{_libdir}/libnotcurses.so.%{version}
%{_libdir}/libnotcurses.so.1
%{_libdir}/libnotcurses++.so.1
%{_libdir}/libnotcurses++.so.%{version}
%{_bindir}/notcurses-demo
%{_bindir}/notcurses-input
%{_bindir}/notcurses-ncreel
%{_bindir}/notcurses-tetris
%{_mandir}/man1/notcurses-demo.1.gz
%{_mandir}/man1/notcurses-input.1.gz
%{_mandir}/man1/notcurses-ncreel.1.gz
%{_mandir}/man1/notcurses-tester.1.gz
%{_mandir}/man1/notcurses-tetris.1.gz
%{_mandir}/man1/notcurses-view.1.gz

%files devel
%{_includedir}/notcurses/nckeys.h
%{_includedir}/notcurses/notcurses.h
%{_includedir}/ncpp/Cell.hh
%{_includedir}/ncpp/CellStyle.hh
%{_includedir}/ncpp/Direct.hh
%{_includedir}/ncpp/Menu.hh
%{_includedir}/ncpp/MultiSelector.hh
%{_includedir}/ncpp/NCAlign.hh
%{_includedir}/ncpp/NCBox.hh
%{_includedir}/ncpp/NCKey.hh
%{_includedir}/ncpp/NCLogLevel.hh
%{_includedir}/ncpp/NCScale.hh
%{_includedir}/ncpp/NotCurses.hh
%{_includedir}/ncpp/Palette256.hh
%{_includedir}/ncpp/Plane.hh
%{_includedir}/ncpp/Plot.hh
%{_includedir}/ncpp/Reel.hh
%{_includedir}/ncpp/Root.hh
%{_includedir}/ncpp/Selector.hh
%{_includedir}/ncpp/Tablet.hh
%{_includedir}/ncpp/TabletCallback.hh
%{_includedir}/ncpp/Visual.hh
%{_includedir}/ncpp/_exceptions.hh
%{_includedir}/ncpp/_flag_enum_operator_helpers.hh
%{_includedir}/ncpp/_helpers.hh
%{_includedir}/ncpp/internal
%{_includedir}/ncpp/internal/Helpers.hh
%{_includedir}/ncpp/ncpp.hh
%{_libdir}/libnotcurses.so
%{_libdir}/libnotcurses++.so
%{_libdir}/cmake/notcurses
%{_libdir}/pkgconfig/notcurses.pc
%{_libdir}/pkgconfig/notcurses++.pc
%{_mandir}/man3/notcurses.3.gz
%{_mandir}/man3/notcurses_cell.3.gz
%{_mandir}/man3/notcurses_channels.3.gz
%{_mandir}/man3/notcurses_directmode.3.gz
%{_mandir}/man3/notcurses_fade.3.gz
%{_mandir}/man3/notcurses_init.3.gz
%{_mandir}/man3/notcurses_input.3.gz
%{_mandir}/man3/notcurses_lines.3.gz
%{_mandir}/man3/notcurses_menu.3.gz
%{_mandir}/man3/notcurses_multiselector.3.gz
%{_mandir}/man3/notcurses_ncplane.3.gz
%{_mandir}/man3/notcurses_ncvisual.3.gz
%{_mandir}/man3/notcurses_output.3.gz
%{_mandir}/man3/notcurses_palette.3.gz
%{_mandir}/man3/notcurses_plot.3.gz
%{_mandir}/man3/notcurses_reel.3.gz
%{_mandir}/man3/notcurses_refresh.3.gz
%{_mandir}/man3/notcurses_render.3.gz
%{_mandir}/man3/notcurses_selector.3.gz
%{_mandir}/man3/notcurses_stats.3.gz
%{_mandir}/man3/notcurses_stdplane.3.gz
%{_mandir}/man3/notcurses_stop.3.gz

%files static
%{_libdir}/libnotcurses.a
%{_libdir}/libnotcurses++.a

%files -n python3-%{srcname}
%{_bindir}/notcurses-pydemo
%{_mandir}/man1/notcurses-pydemo.1.gz
%{python3_sitelib}/*egg-info/

%changelog
* Tue Apr 07 2020 Nick Black <dankamongmen@gmail.com> - 1.2.8-1
- Initial Fedora packaging

Name:          notcurses
Version:       1.3.3
Release:       1%{?dist}
Summary:       Character graphics and TUI library
License:       ASL 2.0
URL:           https://nick-black.com/dankwiki/index.php/Notcurses
Source0:       https://github.com/dankamongmen/notcurses/releases/download/v%{version}/notcurses_%{version}+dfsg.1.orig.tar.xz
Source1:       https://github.com/dankamongmen/%{name}/releases/download/v%{version}/notcurses_%{version}+dfsg.1.orig.tar.xz.asc
Source2:       https://nick-black.com/dankamongmen.gpg

BuildRequires: gnupg2
BuildRequires: cmake
BuildRequires: gcc-c++
BuildRequires: libqrcodegen-devel
BuildRequires: OpenEXR-devel
BuildRequires: OpenImageIO-devel
BuildRequires: pandoc
BuildRequires: python3-devel
BuildRequires: python3-cffi
BuildRequires: pkgconfig(ncurses)

%description
Notcurses facilitates the creation of modern TUI programs,
making full use of Unicode and 24-bit TrueColor. It presents
an API similar to that of Curses, and rides atop Terminfo.
This package includes C and C++ shared libraries.

%package devel
Summary:       Development files for the Notcurses library
License:       ASL 2.0
Requires:      %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for the notcurses library.

%package static
Summary:       Static library for the Notcurses library
License:       ASL 2.0
Requires:      %{name}%{?_isa} = %{version}-%{release}

%description static
A statically-linked version of the notcurses library.

%package utils
Summary:       Binaries from the Notcurses project
License:       ASL 2.0
Requires:      %{name}%{?_isa} = %{version}-%{release}

%description utils
Binaries from Notcurses, and multimedia content used thereby.

%package -n python3-%{name}
Summary:       Python wrappers for notcurses
License:       ASL 2.0
Requires:      %{name}%{?_isa} = %{version}-%{release}

%description -n python3-%{name}
Python wrappers and a demonstration script for the notcurses library.

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%autosetup

# Tests have been disabled due to absence of doctest in Fedora (as of F32)
%build
mkdir build
cd build
%cmake -DUSE_MULTIMEDIA=oiio -DUSE_TESTS=off -DDFSG_BUILD=on ..
%make_build
cd python
%py3_build

%install
cd build
%make_install
cd python
%py3_install

%files
%doc CHANGELOG.md OTHERS.md README.md USAGE.md
%license COPYRIGHT LICENSE
%{_libdir}/libnotcurses.so.%{version}
%{_libdir}/libnotcurses.so.1
%{_libdir}/libnotcurses++.so.1
%{_libdir}/libnotcurses++.so.%{version}

%files devel
%{_includedir}/notcurses/
%{_includedir}/ncpp/
%{_libdir}/libnotcurses.so
%{_libdir}/libnotcurses++.so
%{_libdir}/cmake/notcurses
%{_libdir}/pkgconfig/notcurses.pc
%{_libdir}/pkgconfig/notcurses++.pc
%{_mandir}/man3/*.3*

%files static
%{_libdir}/libnotcurses.a
%{_libdir}/libnotcurses++.a

%files utils
# Don't use a wildcard, lest we pull in notcurses-pydemo.1. We install the man
# pages for notcurses-tester, which we're not yet installing, because we intend
# to install it Real Soon and it's IMHO not worth mucking with the CMake in the
# meantime FIXME.
%{_bindir}/notcurses-demo
%{_bindir}/notcurses-input
%{_bindir}/notcurses-ncreel
%{_bindir}/notcurses-tetris
%{_bindir}/notcurses-view
%{_mandir}/man1/notcurses-demo.1*
%{_mandir}/man1/notcurses-input.1*
%{_mandir}/man1/notcurses-ncreel.1*
%{_mandir}/man1/notcurses-tester.1*
%{_mandir}/man1/notcurses-tetris.1*
%{_mandir}/man1/notcurses-view.1*
%{_datadir}/%{name}

%files -n python3-%{name}
%{_bindir}/notcurses-pydemo
%{_mandir}/man1/notcurses-pydemo.1*
%{python3_sitearch}/*egg-info/
%{python3_sitearch}/notcurses/
%attr(0755, -, -) %{python3_sitearch}/notcurses/notcurses.py
%{python3_sitearch}/*.so

%changelog
* Sat Apr 25 2020 Nick Black <dankamongmen@gmail.com> - 1.3.3-1
- New upstream version, incorporate review feedback
- Build against OpenImageIO, install notcurses-view and data.

* Tue Apr 07 2020 Nick Black <dankamongmen@gmail.com> - 1.3.3-1
- Initial Fedora packaging

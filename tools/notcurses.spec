Name:          notcurses
Version:       1.3.1
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
Requires:      %{name}%{?_isa} = %{version}-%{release}

%description devel
Development files for the notcurses library.

%package static
Summary:       Static library for the notcurses library
License:       ASL 2.0
Requires:      %{name}-devel = %{version}-%{release}

%description static
A statically-linked version of the notcurses library.

%package -n python3-%{name}
Summary:       Python wrappers for notcurses
License:       ASL 2.0
Requires:      %{name}%{?_isa} = %{version}-%{release}
%{?python_provide:%python_provide python3-%{name}}

%description -n python3-%{name}
Python wrappers and a demonstration script for the notcurses library.

%prep
%{gpgverify} --keyring='%{SOURCE2}' --signature='%{SOURCE1}' --data='%{SOURCE0}'
%autosetup

# Tests have been disabled due to absence of doctest in Fedora (as of F32)
%build
mkdir build
cd build
%cmake -DUSE_FFMPEG=off -DUSE_TESTS=off ..
%make_build

%install
cd build
%make_install
cd python
python setup.py install --root=%{buildroot} --optimize=1

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
# Don't use a wildcard, lest we pull in notcurses-pydemo.1. We install the man
# pages for notcurses-tester and notcurses-view, binaries we're not yet
# installing, because we intend to install the binaries Real Soon and it's
# IMHO not worth mucking with the CMake in the meantime FIXME.
%{_mandir}/man1/notcurses-demo.1*
%{_mandir}/man1/notcurses-input.1*
%{_mandir}/man1/notcurses-ncreel.1*
%{_mandir}/man1/notcurses-tester.1*
%{_mandir}/man1/notcurses-tetris.1*
%{_mandir}/man1/notcurses-view.1*

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

%files -n python3-%{name}
%{_bindir}/notcurses-pydemo
%{_mandir}/man1/notcurses-pydemo.1*
%{python3_sitearch}/*egg-info/
%{python3_sitearch}/notcurses/
%{python3_sitearch}/*.so

%changelog
* Tue Apr 07 2020 Nick Black <dankamongmen@gmail.com> - 1.3.1-1
- Initial Fedora packaging

# Building and installation

You are generally recommended to use your distro's package manager to install
Notcurses; it is [available](https://repology.org/project/notcurses/versions)
prepackaged on many distributions. If you wish to build from source, read on.

## Prerequisites for building

Acquire the current source via

`git clone https://github.com/dankamongmen/notcurses.git`

There are no submodules. Dependencies are fairly minimal.

### APT

Install build dependencies:

`apt-get install build-essential cmake doctest-dev zlib1g-dev libavformat-dev libavutil-dev libdeflate-dev libgpm-dev libncurses-dev libqrcodegen-dev libswscale-dev libunistring-dev pandoc pkg-config`

If you only intend to build core Notcurses (without multimedia support), you
can omit `libavformat-dev`, `libavutil-dev`, and `libswscale-dev` from this
list. If you do not want to deflate Kitty graphics, you can omit
'libdeflate-dev'. If you don't want to generate QR codes, you can omit
'libqrcodegen-dev'.

If you want to build the Python wrappers, you'll also need:

`apt-get install python3-cffi python3-dev python3-pypandoc python3-setuptools`

### RPM

Install build dependencies:

`dnf install cmake doctest-devel zlib-devel ncurses-devel gpm-devel libqrcodegen-devel libunistring-devel OpenImageIO-devel pandoc`

If you only intend to build core Notcurses (without multimedia support), you
can omit `OpenImageIO-devel`. If you're building outside Fedora Core (e.g. with
RPM Fusion), you might want to use FFmpeg rather than OpenImageIO. If you don't
want to generate QR codes, you can omit 'libqrcodegen-devel'.

### FreeBSD / DragonFly BSD

Install build dependencies:

`pkg install archivers/libdeflate devel/ncurses multimedia/ffmpeg graphics/qr-code-generator devel/libunistring`

If you only intend to build core Notcurses (without multimedia support), you
can omit `multimedia/ffmpeg`. If you do not want to deflate Kitty graphics,
you can omit 'archivers/libdeflate'. If you don't want to generate QR codes,
you can omit 'graphics/qr-code-generator'.

### Microsoft Windows

Building on Windows requires [MSYS2](https://www.msys2.org/) in its
64-bit Universal C Runtime (UCRT) incarnation. This builds native Windows DLLs
and EXEs, though it does not use Visual Studio. Install build dependencies:

`pacman -S mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-libdeflate mingw-w64-ucrt-x86_64-libunistring mingw-w64-ucrt-x86_64-ncurses mingw-w64-ucrt-x86_64-ninja mingw-w64-ucrt-x86_64-openimageio mingw-w64-ucrt-x86_64-toolchain`

Note that on Windows, OpenImageIO is (at the moment) recommended over FFmpeg.

If you only intend to build core Notcurses (without multimedia support), you
can omit `mingw-w64-ucrt-x86_64-openimageio`. If you do not want to deflate Kitty
graphics, you can omit 'mingw-w64-ucrt-x86_64-libdeflate'.

You'll want to add `-DUSE_DOCTEST=off -DUSE_PANDOC=off` to your `cmake` invocation.

## Building

* Create a subdirectory, traditionally `build` (this is not strictly necessary,
  but it keeps your source tree clean). Enter the directory.
* `cmake ..`
  * You might want to set e.g. `CMAKE_BUILD_TYPE`. Use `-DVAR=val`.
  * To build without multimedia support, use `-DUSE_MULTIMEDIA=none`.
* `make`
* `make test`
* `make install`
* `sudo ldconfig`

The default multimedia engine is FFmpeg. You can select a different engine
using `USE_MULTIMEDIA`. Valid values are `ffmpeg`, `oiio` (for OpenImageIO),
or `none`. Without a multimedia engine, Notcurses will be unable to decode
images and videos.

To get mouse events in the Linux console, you'll need the GPM daemon running,
and you'll need run `cmake` with `-DUSE_GPM=on`.

Run unit tests with `make test` following a successful build. If you have unit
test failures, *please* file a bug including the output of

`./notcurses-tester -p ../data`

(`make test` also runs `notcurses-tester`, but hides important output).

To watch the bitchin' demo, run `make demo` (or `./notcurses-demo -p ../data`).
More details can be found on the `notcurses-demo(1)` man page.

Install with `make install` following a successful build. This installs the C
core library, the C headers, the C++ library, and the C++ headers (note that
the C headers are C++-safe). It does not install the Python wrappers. To
install the Python wrappers (after installing the core library), run:

```
cd cffi
python setup.py build
python setup.py install
```

The Python wrappers are also available from [PyPi](https://pypi.org/project/notcurses/).

### Build options

To set the C compiler, export `CC`. To set the C++ compiler, export `CXX`. The
`CMAKE_BUILD_TYPE` CMake variable can be defined to any of its standard values,
but must be `Debug` for use of `USE_COVERAGE`.

* `DFSG_BUILD`: leave out all content considered non-free under the Debian Free
                Software Guidelines
* `BUILD_TESTING`: build test targets
* `USE_ASAN`: build with AddressSanitizer
* `USE_CPP`: build C++ code (requires a C++ compiler)
* `USE_COVERAGE`: build coverage support (for developers, requires use of Clang)
* `USE_DOCTEST`: build `notcurses-tester` with Doctest, requires `BUILD_TESTING`
  * `USE_DOCTEST=on` requires `USE_CPP=off`
* `USE_DOXYGEN`: build interlinked HTML documentation with Doxygen
* `USE_GPM`: build GPM console mouse support via libgpm
* `USE_MULTIMEDIA`: `ffmpeg` for FFmpeg, `oiio` for OpenImageIO, `none` for none
  * `oiio` cannot be used with `USE_CPP=off`
* `USE_PANDOC`: build man pages with pandoc
* `USE_POC`: build small, uninstalled proof-of-concept binaries
* `USE_QRCODEGEN`: build qrcode support via libqrcodegen
* `USE_STATIC`: build static libraries (in addition to shared ones)

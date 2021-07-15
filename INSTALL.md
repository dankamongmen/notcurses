# Building and installation

You are generally recommended to use your distro's package manager to install
Notcurses; it is [available](https://repology.org/project/notcurses/versions)
prepackaged on many distributions. Otherwise, acquire the current source via

`git clone https://github.com/dankamongmen/notcurses.git`

## Prerequisites

### APT

On an APT-based distribution, run:

`apt-get install build-essential cmake doctest-dev zlib1g-dev libavformat-dev libavutil-dev libncurses-dev libreadline-dev libqrcodegen-dev libswscale-dev libunistring-dev pandoc pkg-config`

If you only intend to build core Notcurses (without multimedia support), run:

`apt-get install build-essential cmake doctest-dev zlib1g-dev libncurses-dev libreadline-dev libqrcodegen-dev libunistring-dev pandoc pkg-config`

If you want to build the Python wrappers, you'll also need:

`apt-get install python3-cffi python3-dev python3-pypandoc python3-setuptools`

If you want to build the Rust wrappers, you'll also need:

`apt-get install cargo bindgen`

### RPM

`dnf install cmake doctest-devel zlib-devel ncurses-devel readline-devel libqrcodegen-devel libunistring-devel OpenImageIO-devel pandoc`

## Building

* Create a subdirectory, traditionally `build` (this is not strictly necessary,
  but it keeps your source tree clean). Enter the directory.
* `cmake ..`
** You might want to set e.g. `CMAKE_BUILD_TYPE`. Use `-DVAR=val`.
** To build without multimedia support, use `-DUSE_MULTIMEDIA=none`.
* `make`
* `make test`
* `make install`
* `sudo ldconfig`

The default multimedia engine is FFmpeg. You can select a different engine
using `USE_MULTIMEDIA`. Valid values are `ffmpeg`, `oiio` (for OpenImageIO),
or `none`. Without a multimedia engine, Notcurses will be unable to decode
images and videos.

Run unit tests with `make test` following a successful build. If you have unit
test failures, *please* file a bug including the output of

`./notcurses-tester -p ../data`

(`make test` also runs `notcurses-tester`, but hides important output).

To watch the bitchin' demo, run `make demo`. More details can
be found on the `notcurses-demo(1)` man page.

Install with `make install` following a successful build. This installs the C
core library, the C headers, the C++ library, and the C++ headers (note that
the C headers are C++-safe). It does not install the Python or Rust wrappers.
To install the Python wrappers (after installing the core library), run:

```
cd cffi
python setup.py build
python setup.py install
```

The Python wrappers are also available from [PyPi](https://pypi.org/project/notcurses/). To install the low-level Rust
wrappers (`libnotcurses-sys`), run:

```
cd rust
cargo build
cargo install
```

The Rust wrappers are also available from [crates.io](https://crates.io/crates/libnotcurses-sys/).

### Build options

To set the C compiler, export `CC`. To set the C++ compiler, export `CXX`. The
`CMAKE_BUILD_TYPE` CMake variable can be defined to any of its standard values,
but must be `Debug` for use of `USE_COVERAGE`.

* `DFSG_BUILD`: leave out all content considered non-free under the Debian Free
                Software Guidelines
* `BUILD_TESTING`: build test targets
* `USE_COVERAGE`: build coverage support (for developers, requires use of Clang)
* `USE_DOCTEST`: build `notcurses-tester` with Doctest, requires `BUILD_TESTING`
* `USE_DOXYGEN`: build interlinked HTML documentation with Doxygen
* `USE_MULTIMEDIA`: `ffmpeg` for FFmpeg, `oiio` for OpenImageIO, `none` for none
* `USE_PANDOC`: build man pages with pandoc
* `USE_POC`: build small, uninstalled proof-of-concept binaries
* `USE_QRCODEGEN`: build qrcode support via libqrcodegen
* `USE_STATIC`: build static libraries (in addition to shared ones)

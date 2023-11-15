# 3.14.0 introduced NAME_WLE
cmake_minimum_required(VERSION 3.14.0)
project(notcurses VERSION 3.0.9
  DESCRIPTION "Blingful UI for modern terminal emulators"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/notcurses"
  LANGUAGES C)
include(CTest)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(FeatureSummary)

###################### USER-SELECTABLE OPTIONS ###########################
# BUILD_TESTING is defined by CTest
option(DFSG_BUILD "DFSG build (no non-free media/code)" OFF)
option(USE_ASAN "Build with AddressSanitizer" OFF)
option(USE_COVERAGE "Assess code coverage with llvm-cov/lcov" OFF)
option(USE_CXX "Build C++ code" ON)
cmake_dependent_option(
  USE_DOCTEST "Build notcurses-tester with doctest" ON
  "BUILD_TESTING;USE_CXX" OFF
)
option(USE_DEFLATE "Use libdeflate instead of libz" ON)
option(USE_DOXYGEN "Build HTML cross reference with doxygen" OFF)
option(USE_GPM "Enable libgpm console mouse support" OFF)
option(USE_PANDOC "Build man pages and HTML reference with pandoc" ON)
option(BUILD_EXECUTABLES "Build executables" ON)
option(BUILD_FFI_LIBRARY "Build ffi library (containing all symbols which are static inline)" ON)
option(USE_POC "Build small, uninstalled proof-of-concept binaries" ON)
option(USE_QRCODEGEN "Enable libqrcodegen QR code support" OFF)
option(USE_STATIC "Build static libraries (in addition to shared)" ON)
set(USE_MULTIMEDIA "ffmpeg" CACHE STRING "Multimedia engine, one of 'ffmpeg', 'oiio', or 'none'")
set_property(CACHE USE_MULTIMEDIA PROPERTY STRINGS ffmpeg oiio none)
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE
    STRING "Choose the build mode." FORCE)
endif()
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo Coverage)
############## END (additional) USER-SELECTABLE OPTIONS ##################
set(USE_FFMPEG OFF)
set(USE_OIIO OFF)
if(${USE_MULTIMEDIA} STREQUAL "ffmpeg")
  set(USE_FFMPEG ON)
elseif(${USE_MULTIMEDIA} STREQUAL "oiio")
  if(NOT ${USE_CXX})
    message(FATAL_ERROR "USE_CXX must be on to use OpenImageIO.")
  endif()
  set(USE_OIIO ON)
elseif(NOT ${USE_MULTIMEDIA} STREQUAL "none")
  message(FATAL_ERROR "USE_MULTIMEDIA must be one of 'oiio', 'ffmpeg', 'none' (was '${USE_MULTIMEDIA}').")
endif()
if (NOT BUILD_EXECUTABLES AND USE_POC)
  message(WARNING "Disabling USE_POC since BUILD_EXECUTABLES=OFF")
  set(USE_POC OFF)
endif()

if(${USE_CXX})
  enable_language(CXX)
endif()
if(${USE_CXX})
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -Og")
endif()
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)

message(STATUS "Requested multimedia engine: ${USE_MULTIMEDIA}")
message(STATUS "Requested build mode: ${CMAKE_BUILD_TYPE}")

string(APPEND CMAKE_C_FLAGS_DEBUG " -Og")
if(${USE_COVERAGE})
  if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message(FATAL_ERROR "USE_COVERAGE was on but CC isn't clang")
  endif()
  # FIXME requires clang11+
  string(APPEND CMAKE_C_FLAGS_DEBUG " --coverage -fprofile-instr-generate -fcoverage-mapping")
  if(${USE_CXX})
    if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
      message(FATAL_ERROR "USE_COVERAGE was on but CXX isn't clang++")
    endif()
    string(APPEND CMAKE_CXX_FLAGS_DEBUG " --coverage -fprofile-instr-generate -fcoverage-mapping")
  endif()
endif()

# under msys2 (and all other operating systems) we want pkgconfig. when
# building with visual studio, don't require it.
if(NOT MSVC)
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
set(PKGCONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
elseif(APPLE)
# surely there's a better way to do this? alas, seems necessary to pull the
# pkg-config files out of Homebrew.
if(NOT DEFINED ENV{PKG_CONFIG_PATH})
set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/ncurses/lib/pkgconfig")
endif()
set(PKGCONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
else()
set(PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/libdata/pkgconfig")
endif()
find_package(PkgConfig REQUIRED)
# some distros (<cough>motherfucking alpine</cough> subsume terminfo directly
# into ncurses. accept either, and may god have mercy on our souls.
pkg_search_module(TERMINFO REQUIRED tinfo>=6.1 ncursesw>=6.1)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND terminfo)
set_package_properties(terminfo PROPERTIES TYPE REQUIRED)
set(PKGCONF_REQ_PRIV "${TERMINFO_LIBRARIES}")
if(${USE_FFMPEG})
pkg_check_modules(AVCODEC REQUIRED libavcodec>=57.0)
pkg_check_modules(AVDEVICE REQUIRED libavdevice>=57.0)
pkg_check_modules(AVFORMAT REQUIRED libavformat>=57.0)
pkg_check_modules(AVUTIL REQUIRED libavutil>=56.0)
pkg_check_modules(SWSCALE REQUIRED libswscale>=5.0)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND FFMpeg)
elseif(${USE_OIIO})
pkg_check_modules(OPENIMAGEIO REQUIRED OpenImageIO>=2.1)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND OpenImageIO)
endif()
endif()

# global compiler flags
if(NOT WIN32)
# MSYS2 needs -lssp and -fstack-protector for _FORTIFY_SOURCE
# macOS with ASAN can't handle _FORTIFY_SOURCE != 1
if(NOT ${USE_ASAN})
add_compile_definitions(_FORTIFY_SOURCE=3)
endif()
add_compile_options(-Wformat -Werror=format-security)
endif()
if(MSVC)
add_compile_options(/W4)
else()
add_compile_options(-Wall -Wextra -W -Wshadow -Wvla -Wstrict-aliasing=2)
# -ffast-math dies on NaNs we draw from libav (by -ffinite-math-only)
add_compile_options(-fno-signed-zeros -fno-trapping-math -fassociative-math)
add_compile_options(-fno-math-errno -freciprocal-math -funsafe-math-optimizations)
add_compile_options(-fexceptions -fstrict-aliasing)
if(${USE_ASAN})
add_compile_options(-fsanitize=address)
add_link_options(-fsanitize=address)
endif()
endif()

# don't use REQUIRED with subsequent find_package() operations; we use
# feature_summary + set_package_properties to fail in one fell swoop.
find_package(Threads)
set_package_properties(Threads PROPERTIES TYPE REQUIRED)
# platform-specific logics
if(WIN32)
  set(LIBRT wsock32 ws2_32 secur32)
elseif(NOT APPLE)
  find_library(LIBM m REQUIRED)
  find_library(LIBRT rt REQUIRED)
endif()
if(NOT ${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  set(CMAKE_REQUIRED_INCLUDES /usr/local/include)
endif()

if(${USE_DOCTEST})
find_package(doctest 2.3.5)
set_package_properties(doctest PROPERTIES TYPE REQUIRED)
endif()

# don't cache these, or installing them requires clearing the cache to be found.
# this is going to be true for anything lacking pkg-config/CMake support.
# unigbrk.h was introduced in libunistring 0.9.4, 2010-02-14.
unset(HAVE_UNISTRING_H CACHE)
find_path(UNISTRING_INCLUDE unigbrk.h)
set(CMAKE_REQUIRED_INCLUDES ${UNISTRING_INCLUDE})
check_include_file("unigbrk.h" HAVE_UNISTRING_H)
if(NOT "${HAVE_UNISTRING_H}")
  message(FATAL_ERROR "Couldn't find unigbrk.h from GNU libunistring")
endif()
find_library(unistring unistring REQUIRED)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND libunistring)
set_package_properties(libunistring PROPERTIES TYPE REQUIRED)

# optional dependencies lacking pkg-config support
if(${USE_DEFLATE})
unset(HAVE_DEFLATE_H CACHE)
check_include_file("libdeflate.h" HAVE_DEFLATE_H)
if(NOT "${HAVE_DEFLATE_H}")
  message(FATAL_ERROR "Couldn't find libdeflate.h")
endif()
find_library(libdeflate deflate REQUIRED)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND libdeflate)
set_package_properties(libdeflate PROPERTIES TYPE REQUIRED)
else()
find_package(ZLIB)
set_package_properties(ZLIB PROPERTIES TYPE REQUIRED)
endif()

if(${USE_GPM}) # no pkgconfig from gpm
unset(HAVE_GPM_H CACHE)
check_include_file("gpm.h" HAVE_GPM_H)
if(NOT "${HAVE_GPM_H}")
  message(FATAL_ERROR "Couldn't find gpm.h from libgpm")
endif()
find_library(gpm gpm REQUIRED)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND libgpm)
set_package_properties(libgpm PROPERTIES TYPE REQUIRED)
endif()

if("${USE_QRCODEGEN}")
unset(HAVE_QRCODEGEN_H CACHE)
check_include_file("qrcodegen/qrcodegen.h" HAVE_QRCODEGEN_H)
if(NOT "${HAVE_QRCODEGEN_H}")
  message(FATAL_ERROR "USE_QRCODEGEN is active, but couldn't find qrcodegen.h")
endif()
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND qrcodegen)
endif()

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

file(GLOB COMPATSRC CONFIGURE_DEPENDS src/compat/*.c)

############################################################################
# libnotcurses-core (core shared library, core static library)
file(GLOB NCCORESRCS CONFIGURE_DEPENDS src/lib/*.c)
add_library(notcurses-core SHARED ${NCCORESRCS} ${COMPATSRC})
if(${USE_STATIC})
add_library(notcurses-core-static STATIC ${NCCORESRCS} ${COMPATSRC})
else()
add_library(notcurses-core-static STATIC EXCLUDE_FROM_ALL ${NCCORESRCS} ${COMPATSRC})
endif()
# don't want these on freebsd/dragonfly/osx
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_compile_definitions(notcurses-core
  PUBLIC
   _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
target_compile_definitions(notcurses-core-static
  PUBLIC
   _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
endif()
set_target_properties(notcurses-core PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
set_target_properties(notcurses-core-static PROPERTIES
  VERSION ${PROJECT_VERSION}
  OUTPUT_NAME notcurses-core
)
target_include_directories(notcurses-core
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
    "${libdeflate_INCLUDE_DIRS}"
    "${ZLIB_INCLUDE_DIRS}"
)
target_include_directories(notcurses-core-static
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_STATIC_INCLUDE_DIRS}"
    "${libdeflate_STATIC_INCLUDE_DIRS}"
    "${ZLIB_STATIC_INCLUDE_DIRS}"
)
target_link_libraries(notcurses-core
  PRIVATE
    "${libdeflate}"
    "${ZLIB_LIBRARIES}"
    "${TERMINFO_LIBRARIES}"
    "${LIBM}"
    "${unistring}"
    "${gpm}"
  PUBLIC
    Threads::Threads
    "${LIBRT}"
)
target_link_libraries(notcurses-core-static
  PRIVATE
    "${libdeflate_STATIC_LIBRARIES}"
    "${ZLIB_STATIC_LIBRARIES}"
    "${TERMINFO_STATIC_LIBRARIES}"
    "${LIBM}"
    "${unistring}"
    "${gpm}"
    Threads::Threads
    "${LIBRT}"
)
target_link_directories(notcurses-core
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
    "${libdeflate_LIBRARY_DIRS}"
    "${ZLIB_LIBRARY_DIRS}"
)
target_link_directories(notcurses-core-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARY_DIRS}"
    "${libdeflate_STATIC_LIBRARY_DIRS}"
    "${ZLIB_STATIC_LIBRARY_DIRS}"
)
if(${USE_QRCODEGEN})
target_link_libraries(notcurses-core PRIVATE qrcodegen)
target_link_libraries(notcurses-core-static PRIVATE qrcodegen)
endif()

############################################################################
# libnotcurses (multimedia shared library+static library)
file(GLOB NCSRCS CONFIGURE_DEPENDS src/media/*.c src/media/*.cpp)
add_library(notcurses SHARED ${NCSRCS} ${COMPATSRC})
if(${USE_STATIC})
add_library(notcurses-static STATIC ${NCSRCS} ${COMPATSRC})
else()
add_library(notcurses-static STATIC EXCLUDE_FROM_ALL ${NCSRCS} ${COMPATSRC})
endif()
set_target_properties(notcurses PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
set_target_properties(notcurses-static PROPERTIES
  VERSION ${PROJECT_VERSION}
  OUTPUT_NAME notcurses
)
target_include_directories(notcurses
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
)
target_include_directories(notcurses-static
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
)
target_compile_definitions(notcurses
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
target_compile_definitions(notcurses-static
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
target_link_libraries(notcurses
  PUBLIC
    notcurses-core
)
target_link_libraries(notcurses-static
  PUBLIC
    notcurses-core-static
)
if(${USE_FFMPEG})
target_include_directories(notcurses
  PRIVATE
    "${AVCODEC_INCLUDE_DIRS}"
    "${AVDEVICE_INCLUDE_DIRS}"
    "${AVFORMAT_INCLUDE_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_INCLUDE_DIRS}"
)
target_include_directories(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_INCLUDE_DIRS}"
    "${AVDEVICE_STATIC_INCLUDE_DIRS}"
    "${AVFORMAT_STATIC_INCLUDE_DIRS}"
    "${AVUTIL_STATIC_INCLUDE_DIRS}"
    "${SWSCALE_STATIC_INCLUDE_DIRS}"
)
target_link_libraries(notcurses
  PRIVATE
    "${AVCODEC_LIBRARIES}"
    "${AVDEVICE_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
    "${AVUTIL_LIBRARIES}"
)
target_link_libraries(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_LIBRARIES}"
    "${AVDEVICE_STATIC_LIBRARIES}"
    "${AVFORMAT_STATIC_LIBRARIES}"
    "${SWSCALE_STATIC_LIBRARIES}"
    "${AVUTIL_STATIC_LIBRARIES}"
)
target_link_directories(notcurses
  PRIVATE
    "${AVCODEC_LIBRARY_DIRS}"
    "${AVDEVICE_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${SWSCALE_LIBRARY_DIRS}"
    "${AVUTIL_LIBRARY_DIRS}"
)
target_link_directories(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_LIBRARY_DIRS}"
    "${AVDEVICE_STATIC_LIBRARY_DIRS}"
    "${AVFORMAT_STATIC_LIBRARY_DIRS}"
    "${SWSCALE_STATIC_LIBRARY_DIRS}"
    "${AVUTIL_STATIC_LIBRARY_DIRS}"
)
elseif(${USE_OIIO})
target_include_directories(notcurses PUBLIC "${OIIO_INCLUDE_DIRS}")
target_include_directories(notcurses-static PUBLIC "${OIIO_STATIC_INCLUDE_DIRS}")
target_link_libraries(notcurses PRIVATE OpenImageIO)
target_link_libraries(notcurses-static PRIVATE ${OIIO_STATIC_LIBRARIES})
target_link_directories(notcurses PRIVATE ${OIIO_LIBRARY_DIRS})
target_link_directories(notcurses-static PRIVATE ${OIIO_STATIC_LIBRARY_DIRS})
endif()

#######################################
# libnotcurses-ffi (ffi shared library)
if(${BUILD_FFI_LIBRARY})
file(GLOB NCFFISRCS CONFIGURE_DEPENDS src/libffi/*.c src/libffi/*.cpp)
add_library(notcurses-ffi SHARED ${NCFFISRCS})
target_compile_options(notcurses-ffi PUBLIC -fkeep-inline-functions)
target_compile_definitions(notcurses-ffi PUBLIC NOTCURSES_FFI)
# don't want these on freebsd/dragonfly/osx
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_compile_definitions(notcurses-ffi
  PUBLIC
   _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
endif()
set_target_properties(notcurses-ffi PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(notcurses-ffi
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
    "${libdeflate_INCLUDE_DIRS}"
    "${ZLIB_INCLUDE_DIRS}"
)

target_link_libraries(notcurses-ffi
  PRIVATE
    "${libdeflate}"
    "${ZLIB_LIBRARIES}"
    "${TERMINFO_LIBRARIES}"
    "${LIBM}"
    "${unistring}"
    "${gpm}"
    "notcurses-core"
  PUBLIC
    Threads::Threads
    "${LIBRT}"
)

target_link_directories(notcurses-ffi
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
    "${libdeflate_LIBRARY_DIRS}"
    "${ZLIB_LIBRARY_DIRS}"
)
endif()

############################################################################
if(${USE_CXX})
# libnotcurses++ (C++ wrappers)
set(NCPP_SOURCES
  src/libcpp/FDPlane.cc
  src/libcpp/Menu.cc
  src/libcpp/MultiSelector.cc
  src/libcpp/NotCurses.cc
  src/libcpp/Plane.cc
  src/libcpp/Plot.cc
  src/libcpp/Reel.cc
  src/libcpp/Root.cc
  src/libcpp/Selector.cc
  src/libcpp/Subproc.cc
  src/libcpp/Tablet.cc
  src/libcpp/Utilities.cc
  )

add_library(notcurses++ SHARED ${NCPP_SOURCES})
if(${USE_STATIC})
add_library(notcurses++-static STATIC ${NCPP_SOURCES})
else()
add_library(notcurses++-static STATIC EXCLUDE_FROM_ALL ${NCPP_SOURCES})
endif()
set_target_properties(
  notcurses++-static PROPERTIES
  OUTPUT_NAME notcurses++
  )

set_target_properties(
  notcurses++ PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  OUTPUT_NAME "notcurses++")

set(NCPP_INCLUDE_DIRS
    "include"
    "src"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
  )

target_include_directories(notcurses++
  BEFORE
  PRIVATE ${NCPP_INCLUDE_DIRS}
  )

target_include_directories(notcurses++-static
  BEFORE
  PRIVATE ${NCPP_INCLUDE_DIRS}
  )

target_link_libraries(notcurses++
  PUBLIC
  notcurses)

set(NCPP_COMPILE_OPTIONS
  -Wnull-dereference
  -Wunused
  -Wno-c99-extensions
  -fno-strict-aliasing
  -ffunction-sections
  -fno-rtti
  )

set(NCPP_COMPILE_DEFINITIONS_PUBLIC
  _GNU_SOURCE _DEFAULT_SOURCE
  )

target_compile_options(notcurses++
  PRIVATE
  ${NCPP_COMPILE_OPTIONS}
  -fPIC
  )

target_compile_options(notcurses-core
  PRIVATE
    -fPIC
)

target_compile_options(notcurses-core-static
  PRIVATE
    -fPIE
)

target_compile_options(notcurses
  PRIVATE
    -fPIC
)

target_compile_options(notcurses-static
  PRIVATE
    -fPIE
)

target_compile_options(notcurses++-static
  PRIVATE
  ${NCPP_COMPILE_OPTIONS}
  -fPIE
)

target_compile_definitions(notcurses++
  PUBLIC
    ${NCPP_COMPILE_DEFINITIONS_PUBLIC}
)

target_compile_definitions(notcurses++-static
  PUBLIC
    ${NCPP_COMPILE_DEFINITIONS_PUBLIC}
)

file(GLOB NOTCURSES_HEADERS
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  ${PROJECT_SOURCE_DIR}/include/notcurses/*.h
  ${CMAKE_CURRENT_BINARY_DIR}/include/version.h)

file(GLOB NCPP_HEADERS
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  ${PROJECT_SOURCE_DIR}/include/ncpp/*.hh)

file(GLOB NCPP_INTERNAL_HEADERS
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  ${PROJECT_SOURCE_DIR}/include/ncpp/internal/*.hh)
endif()

export(PACKAGE notcurses)

install(FILES ${NOTCURSES_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/notcurses)
install(FILES ${NCPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp)
install(FILES ${NCPP_INTERNAL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp/internal)

# tiny proofs of concept, one binary per source file
if(USE_POC)
file(GLOB POCSRCS CONFIGURE_DEPENDS src/poc/*.c)
foreach(f ${POCSRCS})
  get_filename_component(fe "${f}" NAME_WE)
  add_executable(${fe} ${f})
  target_include_directories(${fe}
    BEFORE
    PRIVATE include src "${TERMINFO_INCLUDE_DIRS}"
    "${PROJECT_BINARY_DIR}/include"
  )
  target_link_libraries(${fe}
    PRIVATE notcurses "${TERMINFO_LIBRARIES}" "${LIBM}" "${LIBRT}"
  )
  target_link_directories(${fe}
    PRIVATE "${TERMINFO_LIBRARY_DIRS}"
  )
endforeach()
if(${USE_CXX})
  file(GLOB POCPPSRCS CONFIGURE_DEPENDS src/pocpp/*.cpp)
  foreach(f ${POCPPSRCS})
    get_filename_component(fe "${f}" NAME_WE)
    add_executable(${fe} ${f})
    target_include_directories(${fe}
      BEFORE
      PRIVATE include src "${TERMINFO_INCLUDE_DIRS}"
      "${PROJECT_BINARY_DIR}/include"
    )
    target_link_libraries(${fe}
      PRIVATE notcurses++ "${TERMINFO_LIBRARIES}" "${LIBM}" "${LIBRT}"
    )
    target_link_directories(${fe}
      PRIVATE "${TERMINFO_LIBRARY_DIRS}"
    )
  endforeach()
endif()
endif()

# documentation source, processed by pandoc into XHTML and man pages. declare
# them here so that we can filter out man pages for binaries which aren't
# going to be installed.
file(GLOB MANSOURCE1 CONFIGURE_DEPENDS doc/man/man1/*.md)
file(GLOB MANSOURCE3 CONFIGURE_DEPENDS doc/man/man3/*.md)

if(BUILD_EXECUTABLES)
############################################################################
# notcurses-demo
file(GLOB DEMOSRCS CONFIGURE_DEPENDS src/demo/*.c)
add_executable(notcurses-demo ${DEMOSRCS} ${COMPATSRC})
target_compile_definitions(notcurses-demo
  PRIVATE
    _GNU_SOURCE
)
target_include_directories(notcurses-demo
  BEFORE
  PRIVATE
    include
    src
    "${TERMINFO_INCLUDE_DIRS}"
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-demo
  PRIVATE
    notcurses
    ${LIBM}
    ${LIBRT}
    Threads::Threads
)

############################################################################
# notcurses-info
file(GLOB INFOSRCS CONFIGURE_DEPENDS src/info/*.c)
add_executable(notcurses-info ${INFOSRCS} ${COMPATSRC})
target_compile_definitions(notcurses-info
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
target_include_directories(notcurses-info
  BEFORE
  PRIVATE
    src
    include
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
)
target_link_libraries(notcurses-info
  PRIVATE
    notcurses
    "${LIBRT}"
)

############################################################################
# notcurses-input
if(${USE_CXX})
file(GLOB INPUTSRCS CONFIGURE_DEPENDS src/input/input.cpp)
add_executable(notcurses-input ${INPUTSRCS})
target_include_directories(notcurses-input
  BEFORE
  PRIVATE
    include
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-input
  PRIVATE
    notcurses++
    "${LIBRT}"
)

############################################################################
# nctetris
file(GLOB TETRISSRC CONFIGURE_DEPENDS src/tetris/*.cpp src/compat/*.c)
add_executable(nctetris ${TETRISSRC})
target_include_directories(nctetris
  BEFORE
  PRIVATE
    src
    include
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(nctetris
  PRIVATE
    notcurses++
    "${LIBRT}"
)
endif()

############################################################################
# tfman
if(NOT WIN32)
file(GLOB TFMANSRCS CONFIGURE_DEPENDS src/man/*.c)
add_executable(tfman ${TFMANSRCS} ${COMPATSRC})
target_compile_definitions(tfman
  PRIVATE
    _GNU_SOURCE
)
target_include_directories(tfman
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${libdeflate_INCLUDE_DIRS}"
    "${ZLIB_INCLUDE_DIRS}"
)
target_link_libraries(tfman
  PRIVATE
    notcurses-core
    "${libdeflate}"
    "${ZLIB_LIBRARIES}"
)
endif()

############################################################################
# ncneofetch
file(GLOB FETCHSRCS CONFIGURE_DEPENDS src/fetch/*.c src/compat/*.c)
add_executable(ncneofetch ${FETCHSRCS} ${COMPATSRC})
target_include_directories(ncneofetch
  BEFORE
  PRIVATE
    include
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    src
)
target_link_libraries(ncneofetch
  PRIVATE
    notcurses
    "${LIBRT}"
)


# all further binaries require multimedia and C++ support
if(${USE_CXX})
if(${USE_MULTIMEDIA} STREQUAL "none")
  list(FILTER MANSOURCE1 EXCLUDE REGEX "ncls.1.md")
  list(FILTER MANSOURCE1 EXCLUDE REGEX "ncplayer.1.md")
else()
############################################################################
# ncls
file(GLOB LSSRC CONFIGURE_DEPENDS src/ls/*.cpp)
add_executable(ncls ${LSSRC})
target_include_directories(ncls
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(ncls
  PRIVATE
    notcurses++
)

############################################################################
# ncplayer
file(GLOB PLAYERSRCS CONFIGURE_DEPENDS src/player/*.cpp)
add_executable(ncplayer ${PLAYERSRCS} ${COMPATSRC})
target_include_directories(ncplayer
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(ncplayer
  PRIVATE
    notcurses++
)
endif()
endif()
else()
set(MANSOURCE1 "") # no executables were built
endif() # BUILD_EXECUTABLES

############################################################################
# testing
if(${BUILD_TESTING})
if(${USE_CXX})
#set(CMAKE_CTEST_ARGUMENTS "-V")
if(${USE_DOCTEST})
file(GLOB TESTSRCS CONFIGURE_DEPENDS src/tests/*.cpp src/compat/*.c)
add_executable(notcurses-tester ${TESTSRCS})
target_include_directories(notcurses-tester
  BEFORE
  PRIVATE
    include
    src
    "${CMAKE_REQUIRED_INCLUDES}"
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
)
target_link_libraries(notcurses-tester
  PRIVATE
    notcurses++
    "${unistring}"
    "${TERMINFO_LIBRARIES}"
)
target_link_directories(notcurses-tester
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
)
add_test(
  NAME notcurses-tester
  COMMAND notcurses-tester -p ${CMAKE_CURRENT_SOURCE_DIR}/data --abort-after=1
)
set_tests_properties(notcurses-tester PROPERTIES RUN_SERIAL TRUE)
install(TARGETS notcurses-tester DESTINATION bin)
else()
  list(FILTER MANSOURCE1 EXCLUDE REGEX "notcurses-tester.1.md")
endif()
endif()
enable_testing()
# the accursed Ubuntu buildd sets "TERM=unknown" for unfathomable reasons
if(DEFINED ENV{TERM} AND NOT $ENV{TERM} STREQUAL "unknown" AND USE_POC)
add_test(
  NAME notcurses-info
  COMMAND notcurses-info
)
if(${USE_CXX})
add_test(
  NAME ncpp_build
  COMMAND ncpp_build
)
add_test(
  NAME ncpp_build_exceptions
  COMMAND ncpp_build_exceptions
)
# provide an empty source
add_test(
  NAME input-devnull
  COMMAND sh -c "./notcurses-input -v < /dev/null"
)
# provide an ASCII file
add_test(
  NAME input-text
  COMMAND sh -c "./notcurses-input < ${CMAKE_SOURCE_DIR}/COPYRIGHT"
)
add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND} DEPENDS notcurses-input)
LIST(APPEND TESTBINS ncpp_build ncpp_build_exceptions input-devnull input-text)
endif()
add_test(
  NAME sgr-direct
  COMMAND sgr-direct
)
add_test(
  NAME sgr-full
  COMMAND sgr-full
)
add_test(
  NAME rgb
  COMMAND rgb
)
add_test(
  NAME rgbbg
  COMMAND rgbbg
)
LIST(APPEND TESTBINS notcurses-info sgr-direct sgr-full rgb rgbbg)
if(${USE_QRCODEGEN})
add_test(
  NAME qrcode
  COMMAND qrcode
)
endif()
set_tests_properties(
  ${TESTBINS} PROPERTIES RUN_SERIAL TRUE
)
endif()
else()
  list(FILTER MANSOURCE1 EXCLUDE REGEX "notcurses-tester.1.md")
endif()

# Pandoc documentation (man pages, HTML reference)
if(USE_PANDOC)
  find_program(PANDOC pandoc)
  if(NOT PANDOC)
    message(FATAL_ERROR "pandoc not found. USE_PANDOC=OFF to disable.")
  else()
    foreach(m ${MANSOURCE3} ${MANSOURCE1})
      get_filename_component(me ${m} NAME_WLE)
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${me}
        DEPENDS ${m}
        COMMAND ${PANDOC}
        ARGS --to man --standalone --from markdown-smart ${m} > ${CMAKE_CURRENT_BINARY_DIR}/${me}
        COMMENT "Building man page ${me}"
      )
      add_custom_target(${me}.man
        ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${me}
      )
      file(GLOB ANALHTML doc/analytics-header.html)
      add_custom_command(
        OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${me}.html
        DEPENDS ${m} ${ANALHTML}
        COMMAND ${PANDOC}
        ARGS -H ${ANALHTML} --to html --standalone --from markdown-smart ${m} > ${CMAKE_CURRENT_BINARY_DIR}/${me}.html
        COMMENT "Building HTML5 ${me}.html"
      )
      add_custom_target(${me}.html5
        ALL
        DEPENDS ${CMAKE_CURRENT_BINARY_DIR}/${me}.html
      )
    endforeach()
    foreach(m ${MANSOURCE3})
      get_filename_component(me ${m} NAME_WLE)
      LIST(APPEND MANPAGES3 ${CMAKE_CURRENT_BINARY_DIR}/${me})
    endforeach()
    foreach(m ${MANSOURCE1})
      get_filename_component(me ${m} NAME_WLE)
      LIST(APPEND MANPAGES1 ${CMAKE_CURRENT_BINARY_DIR}/${me})
    endforeach()
  endif()
endif()

# Doxygen / diagrams
if(USE_DOXYGEN)
  find_package(Doxygen REQUIRED dot dia)
  if(NOT ${DOXYGEN_FOUND})
    message(FATAL_ERROR "doxygen not found. USE_DOXYGEN=OFF to disable.")
  else()
    set(DOXYFILE ${CMAKE_CURRENT_SOURCE_DIR}/doc/Doxyfile)
    # FIXME should dep on all source, i suppose, yuck
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
      DEPENDS ${DOXYFILE}
      COMMAND Doxygen::doxygen
      ARGS ${DOXYFILE}
      COMMENT "Running doxygen"
    )
    add_custom_target(doxygen
      ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
    )
    set(MODELDOT ${CMAKE_CURRENT_SOURCE_DIR}/doc/model.dot)
    add_custom_command(
      OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/model.png"
      DEPENDS ${MODELDOT}
      COMMAND Doxygen::dot
      ARGS -Tpng ${MODELDOT} -o "${CMAKE_CURRENT_BINARY_DIR}/model.png"
      COMMENT "Running dot"
    )
    add_custom_target(dot
      ALL DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/model.png"
    )
  endif()
endif()

add_custom_target(demo
  COMMAND ./notcurses-demo -p ${CMAKE_CURRENT_SOURCE_DIR}/data -c
  DEPENDS notcurses-demo
)

# pkg-config support
configure_file(tools/notcurses-core.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses-core.pc
  @ONLY
)
configure_file(tools/notcurses.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  @ONLY
)

if(${BUILD_FFI_LIBRARY})
configure_file(tools/notcurses-ffi.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses-ffi.pc
  @ONLY
)
endif()
configure_file(tools/notcurses++.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses++.pc
  @ONLY
)

include(CMakePackageConfigHelpers)
configure_file(tools/version.h.in include/version.h)
configure_file(tools/builddef.h.in include/builddef.h)

configure_package_config_file(tools/NotcursesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Notcurses
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesCoreConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(tools/NotcursesCoreConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesCoreConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/NotcursesCore
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

configure_package_config_file(tools/Notcurses++Config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/Notcurses++Config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/Notcurses++
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/Notcurses++ConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

# Installation
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesCoreConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesCoreConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/NotcursesCore"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Notcurses"
)

install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/Notcurses++Config.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/Notcurses++ConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Notcurses++"
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses-core.pc
  DESTINATION ${PKGCONFIG_DIR}
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  DESTINATION ${PKGCONFIG_DIR}
)

if(${BUILD_FFI_LIBRARY})
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses-ffi.pc
  DESTINATION ${PKGCONFIG_DIR}
)
endif()

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses++.pc
  DESTINATION ${PKGCONFIG_DIR}
)

if(NOT ${USE_MULTIMEDIA} STREQUAL "none")
file(GLOB TESTDATA CONFIGURE_DEPENDS data/*)
# Don't install source materia for self-originated multimedia
list(FILTER TESTDATA EXCLUDE REGEX ".*xcf$")
list(FILTER TESTDATA EXCLUDE REGEX ".*osp$")
install(FILES ${TESTDATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/notcurses)
endif()

install(FILES ${MANPAGES1} DESTINATION ${CMAKE_INSTALL_DATADIR}/man/man1)
install(FILES ${MANPAGES3} DESTINATION ${CMAKE_INSTALL_DATADIR}/man/man3)
file(GLOB MARKDOWN CONFIGURE_DEPENDS *.md)
list(FILTER MARKDOWN EXCLUDE REGEX "INSTALL.md")
install(FILES ${MARKDOWN} DESTINATION ${CMAKE_INSTALL_DOCDIR})

if(BUILD_EXECUTABLES)
install(TARGETS notcurses-demo DESTINATION bin)
install(TARGETS notcurses-info DESTINATION bin)
install(TARGETS ncneofetch DESTINATION bin)
if(NOT WIN32)
install(TARGETS tfman DESTINATION bin)
endif()
if(${USE_CXX})
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS nctetris DESTINATION bin)
if(NOT ${USE_MULTIMEDIA} STREQUAL "none")
install(TARGETS ncls DESTINATION bin)
install(TARGETS ncplayer DESTINATION bin)
endif()
endif()
endif() # BUILD_EXECUTABLES

if(${BUILD_FFI_LIBRARY})
LIST(APPEND INSTLIBS notcurses-ffi)
endif()
LIST(APPEND INSTLIBS notcurses-core notcurses)
if(${USE_STATIC})
LIST(APPEND INSTLIBS notcurses-core-static notcurses-static)
endif()
if(${USE_CXX})
LIST(APPEND INSTLIBS notcurses++)
if(${USE_STATIC})
LIST(APPEND INSTLIBS notcurses++-static)
endif()
endif()

install(TARGETS ${INSTLIBS}
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
)

option(DUMMY_PYTHON "Build dummy python module used for compile check and Clangd" OFF)
if(${DUMMY_PYTHON})
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
add_subdirectory("./python")
endif()

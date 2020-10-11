cmake_minimum_required(VERSION 3.14.0)
project(notcurses VERSION 1.7.6
  DESCRIPTION "Blingful UI for modern terminal emulators"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/notcurses"
  LANGUAGES C CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(CTest)
include(GNUInstallDirs)
include(CMakeDependentOption)
include(FeatureSummary)

###################### USER-SELECTABLE OPTIONS ###########################
# BUILD_TESTING is defined by CTest
option(DFSG_BUILD "DFSG build (no non-free media/code)" OFF)
option(USE_COVERAGE "Assess code coverage with llvm-cov/lcov" OFF)
cmake_dependent_option(
  USE_DOCTEST "Build notcurses-tester with doctest" ON
  "${BUILD_TESTING}" OFF
)
option(USE_DOXYGEN "Build HTML cross reference with doxygen" OFF)
option(USE_PANDOC "Build man pages and HTML reference with pandoc" ON)
option(USE_POC "Build small, uninstalled proof-of-concept binaries" ON)
option(USE_QRCODEGEN "Disable libqrcodegen QR code support" ON)
option(USE_STATIC "Build static libraries (in addition to shared)" ON)
set(USE_MULTIMEDIA "ffmpeg" CACHE STRING "Multimedia engine, one of 'ffmpeg', 'oiio', or 'none'")
set_property(CACHE USE_MULTIMEDIA PROPERTY STRINGS ffmpeg oiio none)
set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS Debug Release MinSizeRel RelWithDebInfo Coverage)
############## END (additional) USER-SELECTABLE OPTIONS ##################
set(USE_FFMPEG OFF)
set(USE_OIIO OFF)
if(${USE_MULTIMEDIA} STREQUAL "ffmpeg")
  set(USE_FFMPEG ON)
elseif(${USE_MULTIMEDIA} STREQUAL "oiio")
  set(USE_OIIO ON)
elseif(NOT ${USE_MULTIMEDIA} STREQUAL "none")
  message(FATAL_ERROR "USE_MULTIMEDIA must be one of 'oiio', 'ffmpeg', 'none' (was '${USE_MULTIMEDIA}')")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE
    STRING "Choose the build mode." FORCE)
endif()
string(APPEND CMAKE_C_FLAGS_DEBUG " -O0")
string(APPEND CMAKE_CXX_FLAGS_DEBUG " -O0")
if("${USE_COVERAGE}")
  if(NOT "${CMAKE_C_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message(FATAL_ERROR "USE_COVERAGE was on but CC isn't clang")
  endif()
  if(NOT "${CMAKE_CXX_COMPILER_ID}" MATCHES "(Apple)?[Cc]lang")
    message(FATAL_ERROR "USE_COVERAGE was on but CXX isn't clang++")
  endif()
  # FIXME requires clang11+
  string(APPEND CMAKE_C_FLAGS_DEBUG " --coverage -fprofile-instr-generate -fcoverage-mapping")
  string(APPEND CMAKE_CXX_FLAGS_DEBUG " --coverage -fprofile-instr-generate -fcoverage-mapping")
endif()

# global compiler flags
add_compile_definitions(FORTIFY_SOURCE=2)
add_compile_options(-Wall -Wextra -W -Wshadow -Wformat -fexceptions)

message(STATUS "Requested multimedia engine: ${USE_MULTIMEDIA}")
message(STATUS "Requested build mode: ${CMAKE_BUILD_TYPE}")

find_package(PkgConfig REQUIRED)
# don't use REQUIRED with subsequent find_package() operations; we use
# feature_summary + set_package_properties to fail in one fell swoop.
find_package(Threads)
set_package_properties(Threads PROPERTIES TYPE REQUIRED)
# some distros (<cough>motherfucking alpine</cough> subsume terminfo directly
# into ncurses. accept either, and may god have mercy on our souls.
pkg_search_module(TERMINFO REQUIRED tinfo>=6.1 ncursesw>=6.1)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND terminfo)
set_package_properties(terminfo PROPERTIES TYPE REQUIRED)
if(${USE_FFMPEG})
pkg_check_modules(AVCODEC REQUIRED libavcodec>=57.0)
pkg_check_modules(AVFORMAT REQUIRED libavformat>=57.0)
pkg_check_modules(AVUTIL REQUIRED libavutil>=56.0)
pkg_check_modules(SWSCALE REQUIRED libswscale>=5.0)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND FFMpeg)
elseif(${USE_OIIO})
pkg_check_modules(OIIO REQUIRED OpenImageIO>=2.1)
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND OpenImageIO)
endif()
find_library(MATH_LIBRARIES m)
if(${USE_DOCTEST})
find_package(doctest 2.3.5)
set_package_properties(doctest PROPERTIES TYPE REQUIRED)
endif()

# don't cache these, or installing them requires clearing the cache to be found.
# this is going to be true for anything lacking pkg-config/CMake support.
unset(HAVE_UNISTRING_H CACHE)
check_include_file("uniwbrk.h" HAVE_UNISTRING_H)
if(NOT "${HAVE_UNISTRING_H}")
  message(FATAL_ERROR "Couldn't find uniwbrk.h from GNU libunistring")
endif()
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND libunistring)
set_package_properties(libunistring PROPERTIES TYPE REQUIRED)
unset(HAVE_QRCODEGEN_H CACHE)
if("${USE_QRCODEGEN}")
check_include_file("qrcodegen/qrcodegen.h" HAVE_QRCODEGEN_H)
if(NOT "${HAVE_QRCODEGEN_H}")
  message(FATAL_ERROR "USE_QRCODEGEN is active, but couldn't find qrcodegen.h")
endif()
set_property(GLOBAL APPEND PROPERTY PACKAGES_FOUND qrcodegen)
endif()
find_library(LIBRT rt)

# libnotcurses (core shared library, core static library)
file(GLOB NCSRCS CONFIGURE_DEPENDS src/lib/*.c src/lib/*.cpp)
add_library(notcurses SHARED ${NCSRCS})
if(${USE_STATIC})
add_library(notcurses-static STATIC ${NCSRCS})
else()
add_library(notcurses-static STATIC EXCLUDE_FROM_ALL ${NCSRCS})
endif()
set_target_properties(
  notcurses-static PROPERTIES
  OUTPUT_NAME notcurses
)

feature_summary(WHAT ALL FATAL_ON_MISSING_REQUIRED_PACKAGES)

set_target_properties(notcurses PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
set_target_properties(notcurses-static PROPERTIES
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
target_include_directories(notcurses
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
)
target_include_directories(notcurses-static
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_STATIC_INCLUDE_DIRS}"
)
target_link_libraries(notcurses
  PRIVATE
    "${TERMINFO_LIBRARIES}"
    "${LIBRT}"
    unistring
  PUBLIC
    Threads::Threads
)
target_link_libraries(notcurses-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARIES}"
    "${LIBRT}"
    unistring
  PUBLIC
    Threads::Threads
)
target_link_directories(notcurses
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
)
target_link_directories(notcurses-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARY_DIRS}"
)

if(${USE_QRCODEGEN})
target_link_libraries(notcurses PRIVATE qrcodegen)
target_link_libraries(notcurses-static PRIVATE qrcodegen)
endif()

if(${USE_FFMPEG})
target_include_directories(notcurses
  PUBLIC
    "${AVCODEC_INCLUDE_DIRS}"
    "${AVFORMAT_INCLUDE_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_INCLUDE_DIRS}"
)
target_include_directories(notcurses-static
  PUBLIC
    "${AVCODEC_STATIC_INCLUDE_DIRS}"
    "${AVFORMAT_STATIC_INCLUDE_DIRS}"
    "${AVUTIL_STATIC_INCLUDE_DIRS}"
    "${SWSCALE_STATIC_INCLUDE_DIRS}"
)
target_link_libraries(notcurses
  PRIVATE
    "${AVCODEC_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
  PUBLIC
    "${AVUTIL_LIBRARIES}"
)
target_link_libraries(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_LIBRARIES}"
    "${AVFORMAT_STATIC_LIBRARIES}"
    "${SWSCALE_STATIC_LIBRARIES}"
  PUBLIC
    "${AVUTIL_STATIC_LIBRARIES}"
)
target_link_directories(notcurses
  PRIVATE
    "${AVCODEC_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${SWSCALE_LIBRARY_DIRS}"
  PUBLIC
    "${AVUTIL_LIBRARY_DIRS}"
)
target_link_directories(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_LIBRARY_DIRS}"
    "${AVFORMAT_STATIC_LIBRARY_DIRS}"
    "${SWSCALE_STATIC_LIBRARY_DIRS}"
  PUBLIC
    "${AVUTIL_STATIC_LIBRARY_DIRS}"
)
elseif(${USE_OIIO})
target_include_directories(notcurses PUBLIC "${OIIO_INCLUDE_DIRS}")
target_include_directories(notcurses-static PUBLIC "${OIIO_STATIC_INCLUDE_DIRS}")
target_link_libraries(notcurses PRIVATE ${OIIO_LIBRARIES})
target_link_libraries(notcurses-static PRIVATE ${OIIO_STATIC_LIBRARIES})
target_link_directories(notcurses PRIVATE ${OIIO_LIBRARY_DIRS})
target_link_directories(notcurses-static PRIVATE ${OIIO_STATIC_LIBRARY_DIRS})
endif()

# don't want these on freebsd
if(${CMAKE_SYSTEM_NAME} STREQUAL "Linux")
target_compile_definitions(notcurses
  PUBLIC
   _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
target_compile_definitions(notcurses-static
  PUBLIC
   _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
   _GNU_SOURCE _DEFAULT_SOURCE
)
set(PKGCONFIG_DIR "${CMAKE_INSTALL_LIBDIR}/pkgconfig")
elseif(${CMAKE_SYSTEM_NAME} STREQUAL "FreeBSD")
set(PKGCONFIG_DIR "${CMAKE_INSTALL_PREFIX}/libdata/pkgconfig")
endif()

# libnotcurses++
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
  include
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIRS}"
  )

target_include_directories(notcurses++
  PRIVATE ${NCPP_INCLUDE_DIRS}
  )

target_include_directories(notcurses++-static
  PRIVATE ${NCPP_INCLUDE_DIRS}
  )

target_link_libraries(notcurses++
  PUBLIC
  notcurses)

set(NCPP_COMPILE_OPTIONS
  -Werror=format-security
  -Wnull-dereference
  -Wunused
  -Wno-c99-extensions
  -fno-strict-aliasing
  -ffunction-sections
  -fstack-protector
  -fno-rtti
  -fpic
  )

set(NCPP_COMPILE_DEFINITIONS_PUBLIC
  _GNU_SOURCE _DEFAULT_SOURCE
  )

target_compile_options(notcurses++
  PRIVATE
  ${NCPP_COMPILE_OPTIONS}
  )

target_compile_options(notcurses++-static
  PRIVATE
  ${NCPP_COMPILE_OPTIONS}
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

export(PACKAGE notcurses)

install(FILES ${NOTCURSES_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/notcurses)
install(FILES ${NCPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp)
install(FILES ${NCPP_INTERNAL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp/internal)

# notcurses-demo
file(GLOB DEMOSRCS CONFIGURE_DEPENDS src/demo/*.c)
add_executable(notcurses-demo ${DEMOSRCS})
target_compile_definitions(notcurses-demo
  PRIVATE
    _GNU_SOURCE
)
target_include_directories(notcurses-demo
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
  PUBLIC
    "${AVCODEC_INCLUDE_DIRS}"
    "${AVFORMAT_INCLUDE_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_INCLUDE_DIRS}"
)
target_link_libraries(notcurses-demo
  PRIVATE
    notcurses
    unistring
    ${MATH_LIBRARIES}
)

# tiny proofs of concept, one binary per source file
if(USE_POC)
file(GLOB POCSRCS CONFIGURE_DEPENDS src/poc/*.c src/poc/*.cpp)
foreach(f ${POCSRCS})
  get_filename_component(fe "${f}" NAME_WE)
  add_executable(${fe} ${f})
  target_include_directories(${fe}
    PRIVATE include "${TERMINFO_INCLUDE_DIRS}"
    "${PROJECT_BINARY_DIR}/include"
  )
  target_link_libraries(${fe}
    PRIVATE notcurses++ "${TERMINFO_LIBRARIES}"
  )
  target_link_directories(${fe}
    PRIVATE "${TERMINFO_LIBRARY_DIRS}"
  )
endforeach()
endif()

# Pandoc documentation (man pages, HTML reference)
if(USE_PANDOC)
  file(GLOB MANSOURCE1 CONFIGURE_DEPENDS doc/man/man1/*.md)
  file(GLOB MANSOURCE3 CONFIGURE_DEPENDS doc/man/man3/*.md)
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
        ARGS --to man --standalone ${m} > ${CMAKE_CURRENT_BINARY_DIR}/${me}
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
        ARGS -H ${ANALHTML} --to html --standalone ${m} > ${CMAKE_CURRENT_BINARY_DIR}/${me}.html
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

# Doxygen
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
      ALL
      DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/html/index.html"
    )
  endif()
endif()

# ncneofetch
file(GLOB FETCHSRCS CONFIGURE_DEPENDS src/fetch/*.c)
add_executable(ncneofetch ${FETCHSRCS})
target_include_directories(ncneofetch
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(ncneofetch
  PRIVATE
    notcurses
)

# notcurses-input
file(GLOB INPUTSRCS CONFIGURE_DEPENDS src/input/input.cpp)
add_executable(notcurses-input ${INPUTSRCS})
target_include_directories(notcurses-input
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-input
  PRIVATE
    notcurses++
)

# notcurses-tetris
file(GLOB TETRISSRC CONFIGURE_DEPENDS src/tetris/*.cpp)
add_executable(notcurses-tetris ${TETRISSRC})
target_include_directories(notcurses-tetris
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-tetris
  PRIVATE
    notcurses++
)

# notcurses-view
if(${USE_FFMPEG} OR ${USE_OIIO})
file(GLOB VIEWSRCS CONFIGURE_DEPENDS src/view/*.cpp)
add_executable(notcurses-view ${VIEWSRCS})
target_include_directories(notcurses-view
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-view
  PRIVATE
    notcurses++
)
endif()

# Testing
if(${BUILD_TESTING})
# sadly, this doesn't take effect until CMake 3.17...
set(CMAKE_CTEST_ARGUMENTS "-V")
enable_testing()
# the accursed Ubuntu buildd sets "TERM=unknown" for unfathomable reasons
if(DEFINED ENV{TERM} AND NOT $ENV{TERM} STREQUAL "unknown" AND USE_POC)
add_test(
  NAME ncpp_build
  COMMAND ncpp_build
)
add_test(
  NAME ncpp_build_exceptions
  COMMAND ncpp_build_exceptions
)
add_test(
  NAME rgb
  COMMAND rgb
)
add_test(
  NAME rgbbg
  COMMAND rgbbg
)
set_tests_properties(ncpp_build ncpp_build_exceptions rgb rgbbg
  PROPERTIES RUN_SERIAL TRUE
)
endif()
if(${USE_DOCTEST})
file(GLOB TESTSRCS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(notcurses-tester ${TESTSRCS})
target_include_directories(notcurses-tester
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
    src/lib
)
target_link_libraries(notcurses-tester
  PRIVATE
    unistring
    notcurses++
    "${TERMINFO_LIBRARIES}"
)
add_test(
  NAME notcurses-tester
  COMMAND notcurses-tester -p ${CMAKE_CURRENT_SOURCE_DIR}/data
)
set_tests_properties(notcurses-tester PROPERTIES RUN_SERIAL TRUE)
install(TARGETS notcurses-tester DESTINATION bin)
endif()
endif()

# pkg-config support
configure_file(tools/notcurses.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  @ONLY
)

configure_file(tools/notcurses++.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses++.pc
  @ONLY
)
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses++.pc
  DESTINATION ${PKGCONFIG_DIR}
)

include(CMakePackageConfigHelpers)
configure_file(tools/version.h.in include/version.h)
configure_file(tools/builddef.h.in include/builddef.h)

configure_package_config_file(tools/NotcursesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/notcurses/cmake
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

# Installation
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/NotcursesConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Notcurses"
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  DESTINATION ${PKGCONFIG_DIR}
)

if(${USE_FFMPEG} OR ${USE_OIIO})
file(GLOB TESTDATA CONFIGURE_DEPENDS data/*)
# Don't install source materia for self-originated multimedia
list(FILTER TESTDATA EXCLUDE REGEX ".*xcf$")
list(FILTER TESTDATA EXCLUDE REGEX ".*osp$")
install(FILES ${TESTDATA} DESTINATION ${CMAKE_INSTALL_DATADIR}/notcurses)
endif()

install(FILES ${MANPAGES1} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1)
install(FILES ${MANPAGES3} DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3)
file(GLOB MARKDOWN CONFIGURE_DEPENDS *.md)
install(FILES ${MARKDOWN} DESTINATION ${CMAKE_INSTALL_DOCDIR})

install(TARGETS notcurses-demo DESTINATION bin)
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS ncneofetch DESTINATION bin)
install(TARGETS notcurses-tetris DESTINATION bin)
if(${USE_FFMPEG} OR ${USE_OIIO})
install(TARGETS notcurses-view DESTINATION bin)
endif()

install(TARGETS notcurses notcurses++
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
)
if(${USE_STATIC})
install(
  TARGETS notcurses-static notcurses++-static
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
)
endif()

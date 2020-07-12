cmake_minimum_required(VERSION 3.14)
project(notcurses VERSION 1.6.1
  DESCRIPTION "UI for modern terminal emulators"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/notcurses"
  LANGUAGES C CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(GNUInstallDirs)

###################### USER-SELECTABLE OPTIONS ###########################
# BUILD_TESTING is defined by CTest
option(DFSG_BUILD "DFSG build (no non-free media/code)" OFF)
option(USE_COVERAGE "Assess code coverage with llvm-cov/lcov" OFF)
option(USE_DOXYGEN "Build HTML cross reference with doxygen" OFF)
option(USE_NETWORK "Allow cargo to use the network" OFF)
option(USE_PANDOC "Build man pages and HTML reference with pandoc" ON)
option(USE_PYTHON "Build Python wrappers" ON)
option(USE_QRCODEGEN "Disable libqrcodegen QR code support" ON)
option(USE_RUST "Build Rust wrappers (experimental)" OFF)
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
find_package(Threads REQUIRED)
# some distros (<cough>motherfucking alpine</cough> subsume terminfo directly
# into ncurses. accept either, and may god have mercy on our souls.
pkg_search_module(TERMINFO REQUIRED tinfo>=6.1 ncursesw>=6.1)
if(${USE_FFMPEG})
pkg_check_modules(AVCODEC REQUIRED libavcodec>=57.0)
pkg_check_modules(AVFORMAT REQUIRED libavformat>=57.0)
pkg_check_modules(AVUTIL REQUIRED libavutil>=56.0)
pkg_check_modules(SWSCALE REQUIRED libswscale>=5.0)
elseif(${USE_OIIO})
pkg_check_modules(OIIO REQUIRED OpenImageIO>=2.1)
endif()
find_library(MATH_LIBRARIES m)
# don't cache this, or installing it requires clearing the cache to be found
unset(HAVE_QRCODEGEN_H CACHE)
check_include_file("qrcodegen/qrcodegen.h" HAVE_QRCODEGEN_H)
if("${USE_QRCODEGEN}")
if(NOT "${HAVE_QRCODEGEN_H}")
  message(FATAL_ERROR "USE_QRCODEGEN is active, but couldn't find qrcodegen.h")
endif()
endif()
find_library(LIBRT rt)
if(${BUILD_TESTING})
find_package(doctest 2.3.5 REQUIRED)
endif()

# libnotcurses (core shared library and static library)
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
  PUBLIC
    Threads::Threads
)
target_link_libraries(notcurses-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARIES}"
    "${LIBRT}"
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
  -Wmisleading-indentation
  -Wunused
  -Wsuggest-override
  -Wno-c99-extensions
  -fno-strict-aliasing
  -ffunction-sections
  -funswitch-loops
  -finline-limit=300
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
  ${PROJECT_SOURCE_DIR}/include/notcurses/*.h)

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
    ${MATH_LIBRARIES}
)

# tiny proofs of concept, one binary per source file
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

# notcurses-ncreel
file(GLOB NCREELSRCS CONFIGURE_DEPENDS src/ncreel/*.cpp)
add_executable(notcurses-ncreel ${NCREELSRCS})
target_include_directories(notcurses-ncreel
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-ncreel
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
    "${AVCODEC_INCLUDE_DIRS}"
    "${AVFORMAT_INCLUDE_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
)
target_link_directories(notcurses-view
  PRIVATE
    "${AVCODEC_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${AVUTIL_LIBRARY_DIRS}"
)
target_link_libraries(notcurses-view
  PRIVATE
    notcurses++
  PRIVATE
    "${AVCODEC_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${AVUTIL_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
)
endif()

# Testing
include(CTest)
if(${BUILD_TESTING})
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
    notcurses++
    "${TERMINFO_LIBRARIES}"
)
# sadly, this doesn't take effect until CMake 3.17...
set(CMAKE_CTEST_ARGUMENTS "-V")
enable_testing()
add_test(
  NAME ncpp_build
  COMMAND ncpp_build
)
add_test(
  NAME ncpp_build_exceptions
  COMMAND ncpp_build_exceptions
)
add_test(
  NAME dirgb
  COMMAND dirgb
)
add_test(
  NAME sgr
  COMMAND sgr
)
add_test(
  NAME rgb
  COMMAND rgb
)
add_test(
  NAME rgbbg
  COMMAND rgbbg
)
add_test(
  NAME notcurses-tester
  COMMAND notcurses-tester -p ${CMAKE_CURRENT_SOURCE_DIR}/data
)
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
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
  )

include(CMakePackageConfigHelpers)
configure_file(tools/version.h.in include/version.h)

configure_package_config_file(tools/notcursesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/notcurses/cmake
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

# Python bindings. When we're building debs, we use pybuild to directly call
# into setup.py, so none of this applies---debian/rules sets USE_PYTHON=off.
# arch also doesn't use it. fedora does.
if(${USE_PYTHON})
  find_package(Python3 COMPONENTS Development Interpreter REQUIRED)
  file(GLOB PYSRC CONFIGURE_DEPENDS python/src/notcurses/*.py)
  file(COPY python/src/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python/src)
  file(COPY python/README.md DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python)
  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/tools/setup.py.in")
  set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/python/setup.py")
  set(SETUP_CFG_IN "${CMAKE_CURRENT_SOURCE_DIR}/tools/setup.cfg.in")
  set(SETUP_CFG "${CMAKE_CURRENT_BINARY_DIR}/python/setup.cfg")
  configure_file(${SETUP_PY_IN} ${SETUP_PY})
  configure_file(${SETUP_CFG_IN} ${SETUP_CFG})
  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/build/pytimestamp"
    COMMAND
      env LDFLAGS=\"-Wl,-soname,_notcurses.abi3.so.1 -L${CMAKE_CURRENT_BINARY_DIR}\" "${Python3_EXECUTABLE}" ${SETUP_PY} build &&
      "${Python3_EXECUTABLE}" ${SETUP_PY} egg_info
    DEPENDS
      ${PYSRC} ${SETUP_PY} ${SETUP_CFG} notcurses
    COMMENT "Building Python wrappers"
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/python
  )
  # build/pytimestamp isn't actually generated, and thus this runs each time.
  # python does its own dep tracking, so it "works" out like recursive make.
  add_custom_target(pymod ALL
    DEPENDS
      "${CMAKE_CURRENT_BINARY_DIR}/build/pytimestamp"
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_BINARY_DIR}/python
  )
  install(CODE "execute_process(COMMAND ${Python3_EXECUTABLE} setup.py install --prefix=${CMAKE_INSTALL_PREFIX} WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/python)")
endif()

# Rust bindings
if(${USE_RUST})
  file(GLOB LIBNCRSSRC CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/rust/libnotcurses-sys/src/*.rs ${CMAKE_CURRENT_SOURCE_DIR}/rust/libnotcurses-sys/src/*.rs)
  set(LIBNOTCURSESSYS ${CMAKE_CURRENT_BINARY_DIR}/rust/debug/liblibnotcurses_sys.rlib)
  set(CRATENOTCURSES ${CMAKE_CURRENT_BINARY_DIR}/rust/debug/libnotcurses.rlib)
  find_program(CARGO cargo REQUIRED)
  # might need --locked here
  set(CARGO_ARGS --verbose --release)
  if(NOT ${USE_NETWORK})
    set(CARGO_ARGS "${CARGO_ARGS}" --offline) # might want --frozen here
  endif()
  add_custom_command(
    OUTPUT
      ${LIBNOTCURSESSYS}
    COMMAND
      PKG_CONFIG_PATH=${CMAKE_CURRENT_BINARY_DIR} CARGO_HOME=${CMAKE_CURRENT_BINARY_DIR}/rust CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/rust ${CARGO} build ${CARGO_ARGS}
    DEPENDS
      ${LIBNCRSSRC}
    COMMENT "Building rust crate libnotcurses-sys"
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_SOURCE_DIR}/rust/libnotcurses-sys
  )
  add_custom_target(libnotcurses-sys ALL
    DEPENDS
      ${LIBNOTCURSESSYS}
  )
  add_custom_command(
    OUTPUT
      ${CRATENOTCURSES}
    COMMAND
      CARGO_HOME=${CMAKE_CURRENT_BINARY_DIR}/rust CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/rust ${CARGO} build ${CARGO_ARGS}
    DEPENDS
      ${CRATENOTCURSESSRC} ${LIBNOTCURSESSYS}
    COMMENT "Building rust crate notcurses"
    WORKING_DIRECTORY
      ${CMAKE_CURRENT_SOURCE_DIR}/rust/notcurses
  )
  add_custom_target(cratenotcurses ALL
    DEPENDS
      ${CRATENOTCURSES}
  )
endif()

# Installation
install(FILES
  "${CMAKE_CURRENT_BINARY_DIR}/notcursesConfig.cmake"
  "${CMAKE_CURRENT_BINARY_DIR}/notcursesConfigVersion.cmake"
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/notcurses"
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
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

install(PROGRAMS src/pydemo/notcurses-pydemo DESTINATION bin)
install(TARGETS notcurses-demo DESTINATION bin)
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS notcurses-ncreel DESTINATION bin)
install(TARGETS ncneofetch DESTINATION bin)
if(${BUILD_TESTING})
install(TARGETS notcurses-tester DESTINATION bin)
endif()
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

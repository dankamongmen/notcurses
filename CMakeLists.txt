cmake_minimum_required(VERSION 3.14)
project(notcurses VERSION 1.2.1
  DESCRIPTION "UI for modern terminal emulators"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/notcurses"
  LANGUAGES C CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

include(GNUInstallDirs)
set(NOTCURSES_SHARE ${CMAKE_INSTALL_PREFIX}/share/notcurses)

###################### USER-SELECTABLE OPTIONS ###########################
option(BUILD_PYTHON "Build Python wrappers" ON)
option(BUILD_RUST "Build Rust wrappers/colloquy (experimental)" OFF)
option(DFSG_BUILD "DFSG build (no non-free media/code)" OFF)
option(DISABLE_FFMPEG "Disable FFmpeg image/video support, not recommended" OFF)
option(USE_PANDOC "Build man pages and HTML reference with pandoc" ON)
option(USE_DOXYGEN "Build HTML cross reference with doxygen" ON)
option(USE_NETWORK "Allow cargo to use the network" OFF)
############## END (additional) USER-SELECTABLE OPTIONS ##################

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(TERMINFO REQUIRED tinfo>=6.1)
if(NOT "${DISABLE_FFMPEG}")
pkg_check_modules(AVCODEC REQUIRED libavcodec>=57.0)
pkg_check_modules(AVFORMAT REQUIRED libavformat>=57.0)
pkg_check_modules(AVUTIL REQUIRED libavutil>=56.0)
pkg_check_modules(SWSCALE REQUIRED libswscale>=5.0)
endif()
find_library(LIBRT rt)
find_package(doctest 2.3.6 REQUIRED)

# libnotcurses
file(GLOB NCSRCS CONFIGURE_DEPENDS src/lib/*.c)
add_library(notcurses SHARED ${NCSRCS})
add_library(notcurses-static STATIC ${NCSRCS})
set_target_properties(
  notcurses-static PROPERTIES
  OUTPUT_NAME notcurses
)

set_target_properties(notcurses PROPERTIES
  PUBLIC_HEADER "include/notcurses.h"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)
set_target_properties(notcurses-static PROPERTIES
  PUBLIC_HEADER "include/notcurses.h"
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
)
target_link_libraries(notcurses-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARIES}"
    "${LIBRT}"
)
target_link_directories(notcurses
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
)
target_link_directories(notcurses-static
  PRIVATE
    "${TERMINFO_STATIC_LIBRARY_DIRS}"
)

if(NOT "${DISABLE_FFMPEG}")
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
    "${AVUTIL_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
    Threads::Threads
)
target_link_libraries(notcurses-static
  PRIVATE
    "${AVCODEC_STATIC_LIBRARIES}"
    "${AVFORMAT_STATIC_LIBRARIES}"
    "${AVUTIL_STATIC_LIBRARIES}"
    "${SWSCALE_STATIC_LIBRARIES}"
    Threads::Threads
)
target_link_directories(notcurses
  PUBLIC
    "${AVCODEC_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${AVUTIL_LIBRARY_DIRS}"
    "${SWSCALE_LIBRARY_DIRS}"
)
target_link_directories(notcurses-static
  PUBLIC
    "${AVCODEC_STATIC_LIBRARY_DIRS}"
    "${AVFORMAT_STATIC_LIBRARY_DIRS}"
    "${AVUTIL_STATIC_LIBRARY_DIRS}"
    "${SWSCALE_STATIC_LIBRARY_DIRS}"
)
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
  set(DEBUG_OPTIONS -g -O0)
else()
  set(DEBUG_OPTIONS -O2)
endif()

target_compile_options(notcurses
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_options(notcurses-static
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses
  PUBLIC
    _XOPEN_SOURCE # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
    FORTIFY_SOURCE=2 _GNU_SOURCE SOURCE_DEFAULT
)
target_compile_definitions(notcurses-static
  PUBLIC
    _XOPEN_SOURCE # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
    FORTIFY_SOURCE=2 _GNU_SOURCE SOURCE_DEFAULT
)

# libnotcurses++
set(NCPP_SOURCES
  src/libcpp/Menu.cc
  src/libcpp/NotCurses.cc
  src/libcpp/Plane.cc
  src/libcpp/Reel.cc
  src/libcpp/Root.cc
  src/libcpp/Selector.cc
  src/libcpp/Tablet.cc
  src/libcpp/Visual.cc
  )

add_library(
  notcurses++
  SHARED
  ${NCPP_SOURCES}
  )

add_library(
  notcurses++-static
  STATIC
  ${NCPP_SOURCES}
  )
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
  PRIVATE
  notcurses)

set(NCPP_COMPILE_OPTIONS
  -Wall
  -Wextra
  -W
  -Wshadow
  -Wformat
  -Werror=format-security
  -Wnull-dereference
  -Wmisleading-indentation
  -Wunused
  -Wpedantic
  -Wsuggest-override
  -Wno-c99-extensions
  -fno-strict-aliasing
  -ffunction-sections
  -funswitch-loops
  -finline-limit=300
  -fstack-protector
  -fno-rtti
  -fno-exceptions
  -fpic
  ${DEBUG_OPTIONS}
  )

set(NCPP_COMPILE_DEFINITIONS_PUBLIC
  FORTIFY_SOURCE=2 _GNU_SOURCE SOURCE_DEFAULT
  )
set(NCPP_COMPILE_DEFINITIONS_PRIVATE
  _XOPEN_SOURCE # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  )

target_compile_options(notcurses++
  PRIVATE
  ${NCPP_COMPILLE_OPTIONS}
  )

target_compile_options(notcurses++-static
  PRIVATE
  ${NCPP_COMPILLE_OPTIONS}
  )

target_compile_definitions(notcurses++
  PUBLIC
    ${NCPP_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
    ${NCPP_COMPILE_DEFINITIONS_PRIVATE}
  )

target_compile_definitions(notcurses++-static
  PUBLIC
    ${NCPP_COMPILE_DEFINITIONS_PUBLIC}
  PRIVATE
    ${NCPP_COMPILE_DEFINITIONS_PRIVATE}
  )

file(GLOB NCPP_HEADERS
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  ${CMAKE_SOURCE_DIR}/include/ncpp/*.hh)

file(GLOB NCPP_INTERNAL_HEADERS
  CONFIGURE_DEPENDS
  LIST_DIRECTORIES false
  ${CMAKE_SOURCE_DIR}/include/ncpp/internal/*.hh)

install(FILES ${NCPP_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp)
install(FILES ${NCPP_INTERNAL_HEADERS} DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ncpp/internal)

install(
  TARGETS notcurses++
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
)

install(
  TARGETS notcurses++-static
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
	)

# notcurses-demo
file(GLOB DEMOSRCS CONFIGURE_DEPENDS src/demo/*.c)
add_executable(notcurses-demo ${DEMOSRCS})
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
    Threads::Threads
)
target_compile_options(notcurses-demo
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses-demo
  PRIVATE
  FORTIFY_SOURCE=2 _GNU_SOURCE
)

# tiny proofs of concept, one binary per source file
file(GLOB POCSRCS CONFIGURE_DEPENDS src/poc/*.c src/poc/*.cpp)
foreach(f ${POCSRCS})
  get_filename_component(fe "${f}" NAME_WE)
  add_executable(${fe} ${f})
  target_include_directories(${fe}
    PRIVATE include "${TERMINFO_INCLUDE_DIRS}"
  )
  target_link_libraries(${fe}
    PRIVATE notcurses notcurses++ "${TERMINFO_LIBRARIES}"
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
      set(MANPAGES3 ${MANPAGES3} ${CMAKE_CURRENT_BINARY_DIR}/${me})
    endforeach()
    foreach(m ${MANSOURCE1})
      get_filename_component(me ${m} NAME_WLE)
      set(MANPAGES1 ${MANPAGES1} ${CMAKE_CURRENT_BINARY_DIR}/${me})
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

# notcurses-input
file(GLOB INPUTSRCS CONFIGURE_DEPENDS src/input/*.cpp)
add_executable(notcurses-input ${INPUTSRCS})
target_include_directories(notcurses-input
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-input
  PRIVATE
    notcurses notcurses++
)
target_compile_options(notcurses-input
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses-input
  PRIVATE
    FORTIFY_SOURCE=2
)

# notcurses-ncreel
file(GLOB ncreelSRCS CONFIGURE_DEPENDS src/ncreel/*.cpp)
add_executable(notcurses-ncreel ${ncreelSRCS})
target_include_directories(notcurses-ncreel
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-ncreel
  PRIVATE
    notcurses notcurses++
)
target_compile_options(notcurses-ncreel
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses-ncreel
  PRIVATE
    FORTIFY_SOURCE=2
)

# notcurses-view
file(GLOB VIEWSRCS CONFIGURE_DEPENDS src/view/*.cpp)
if(NOT "${DISABLE_FFMPEG}")
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
    notcurses notcurses++
  PRIVATE
    "${AVCODEC_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${AVUTIL_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
)
target_compile_options(notcurses-view
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses-view
  PRIVATE
    FORTIFY_SOURCE=2
)
endif()

# Testing
if(CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME)
  include(CTest)
endif()
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
    notcurses
)
target_compile_options(notcurses-tester
  PRIVATE
    -Wall -Wextra -W -Wshadow ${DEBUG_OPTIONS}
)
target_compile_definitions(notcurses-tester
  PRIVATE
    FORTIFY_SOURCE=2
)

enable_testing()
add_test(
  NAME notcurses-tester
  COMMAND notcurses-tester -p ${CMAKE_CURRENT_SOURCE_DIR}/data
)

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

# Python bindings
if(${BUILD_PYTHON})
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
      "${Python3_EXECUTABLE}" ${SETUP_PY} build_ext -L ${CMAKE_CURRENT_BINARY_DIR} -b ${CMAKE_CURRENT_BINARY_DIR}/python -t${CMAKE_CURRENT_BINARY_DIR}/python &&
      "${Python3_EXECUTABLE}" ${SETUP_PY} build_py
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
endif()

# Rust bindings
if(${BUILD_RUST})
  file(GLOB RUSTSRC CONFIGURE_DEPENDS src/colloquy/*.rs)
  set(CARGO_ARGS "--verbose")
  if(NOT ${USE_NETWORK})
    set(CARGO_ARGS "${CARGO} --frozen")
  endif()
  add_custom_target(colloquy ALL
    COMMAND CARGO_HOME=${CMAKE_CURRENT_BINARY_DIR}/rust CARGO_TARGET_DIR=${CMAKE_CURRENT_BINARY_DIR}/rust cargo build ${CARGO_ARGS}
    DEPENDS
      ${COLLOQUYSRC}
    WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/src/colloquy
  )
  set_target_properties(colloquy
    PROPERTIES LOCATION ${CMAKE_CURRENT_BINARY_DIR}
  )
endif()

# Installation
install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notcurses
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

file(GLOB TESTDATA CONFIGURE_DEPENDS data/*)
install(FILES
  ${TESTDATA}
    DESTINATION ${NOTCURSES_SHARE}
)

install(FILES
  ${MANPAGES1}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man1
)
install(FILES
  ${MANPAGES3}
    DESTINATION ${CMAKE_INSTALL_PREFIX}/share/man/man3
)

install(TARGETS notcurses-demo DESTINATION bin)
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS notcurses-ncreel DESTINATION bin)
install(TARGETS notcurses-tester DESTINATION bin)
if(NOT "${DISABLE_FFMPEG}")
install(TARGETS notcurses-view DESTINATION bin)
endif()
install(TARGETS notcurses notcurses-static
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/notcurses
    COMPONENT Development
)
if(${BUILD_PYTHON})
  # if DFSG_BUILD has been selected, assume we're building debs. debian/rules
  # has its own python install logic after many hours wasted, so we don't
  # install from here for a deb build.
  if(NOT "${DFSG_BUILD}")
    install(
      CODE "execute_process(
        COMMAND
          ${Python3_EXECUTABLE} ${SETUP_PY} install --prefix=${CMAKE_INSTALL_PREFIX} --skip-build
        WORKING_DIRECTORY
          ${CMAKE_CURRENT_BINARY_DIR}/python
      )"
    )
  endif()
endif()

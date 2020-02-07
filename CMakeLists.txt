cmake_minimum_required(VERSION 3.14)
project(notcurses VERSION 1.1.6
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

option(DISABLE_FFMPEG "Disable FFmpeg image/video support" OFF)
option(BUILD_PYTHON "Build Python wrappers" ON)
option(DFSG_BUILD "DFSG build (no non-free media)" OFF)

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

# libnotcurses
file(GLOB NCSRCS CONFIGURE_DEPENDS src/lib/*.c)
add_library(notcurses SHARED ${NCSRCS})
set_target_properties(notcurses PROPERTIES
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
target_link_libraries(notcurses
  PRIVATE
    "${TERMINFO_LIBRARIES}"
    "${LIBRT}"
)
target_link_directories(notcurses
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
)

if(NOT "${DISABLE_FFMPEG}")
target_include_directories(notcurses
  PUBLIC
    "${AVCODEC_INCLUDE_DIRS}"
    "${AVFORMAT_INCLUDE_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_INCLUDE_DIRS}"
)
target_link_libraries(notcurses
  PRIVATE
    "${AVCODEC_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_LIBRARIES}"
)
target_link_directories(notcurses
  PRIVATE
    "${AVCODEC_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${AVUTIL_INCLUDE_DIRS}"
    "${SWSCALE_LIBRARY_DIRS}"
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
target_compile_definitions(notcurses
  PUBLIC
    _XOPEN_SOURCE # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
    FORTIFY_SOURCE=2 _GNU_SOURCE SOURCE_DEFAULT
)

# libnotcurses++
set(NCPP_SOURCES
  src/libcpp/NotCurses.cc
  src/libcpp/Plane.cc
  src/libcpp/Reel.cc
  src/libcpp/Root.cc
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

# Documentation
file(GLOB MANSOURCE1 CONFIGURE_DEPENDS doc/man/man1/*.md)
file(GLOB MANSOURCE3 CONFIGURE_DEPENDS doc/man/man3/*.md)
FIND_PROGRAM(PANDOC pandoc)
iF(NOT PANDOC)
  message(WARNING "pandoc not found, won't generate documentation")
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
    "${AVUTIL_INCLUDE_DIRS}"
)
target_link_directories(notcurses-view
  PRIVATE
    "${AVUTIL_LIBRARY_DIRS}"
)
target_link_libraries(notcurses-view
  PRIVATE
    notcurses notcurses++
  PRIVATE
    "${AVUTIL_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
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
  file(COPY python/src/ DESTINATION ${CMAKE_CURRENT_BINARY_DIR}/python/)
  set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/python/setup.py.in")
  set(SETUP_PY "${CMAKE_CURRENT_BINARY_DIR}/python/setup.py")
  set(SETUP_CFG_IN "${CMAKE_CURRENT_SOURCE_DIR}/python/setup.cfg.in")
  set(SETUP_CFG "${CMAKE_CURRENT_BINARY_DIR}/setup.cfg")
  configure_file(${SETUP_PY_IN} ${SETUP_PY})
  configure_file(${SETUP_CFG_IN} ${SETUP_CFG})
  add_custom_command(
    OUTPUT
      "${CMAKE_CURRENT_BINARY_DIR}/build/pytimestamp"
    COMMAND
      "${Python3_EXECUTABLE}" ${SETUP_PY} build_ext -R${CMAKE_INSTALL_LIBDIR} -L ${CMAKE_CURRENT_BINARY_DIR} -b ${CMAKE_CURRENT_BINARY_DIR}/python &&
       "${Python3_EXECUTABLE}" ${SETUP_PY} build_py &&
       "${Python3_EXECUTABLE}" ${SETUP_PY} egg_info -b ${CMAKE_CURRENT_BINARY_DIR}/python
    DEPENDS
      ${PYSRC} ${SETUP_PY} ${SETUP_CFG} notcurses
    COMMENT "Building Python wrappers"
  )
  # build/pytimestamp isn't actually generated, and thus this runs each time.
  # python does its own dep tracking, so it "works" out like recursive make.
  add_custom_target(pymod
    ALL
    DEPENDS "${CMAKE_CURRENT_BINARY_DIR}/build/pytimestamp"
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
if(NOT "${DISABLE_FFMPEG}")
install(TARGETS notcurses-view DESTINATION bin)
endif()
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS notcurses-ncreel DESTINATION bin)
install(TARGETS notcurses
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
)
if(${BUILD_PYTHON})
  # if DFSG_BUILD has been selected, assume we're building debs
  if("${DFSG_BUILD}")
    install(
      CODE
        "execute_process(COMMAND ${Python3_EXECUTABLE} ${SETUP_PY} install --root=${CMAKE_SOURCE_DIR}/debian/python3-notcurses --install-layout=deb --prefix=${CMAKE_INSTALL_PREFIX} --skip-build
      WORKING_DIRECTORY
        ../python)"
    )
  else()
    install(
      CODE
      "execute_process(COMMAND ${Python3_EXECUTABLE} ${SETUP_PY} install --prefix=${CMAKE_INSTALL_PREFIX})"
    )
  endif()
endif()

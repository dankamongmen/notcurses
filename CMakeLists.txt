cmake_minimum_required(VERSION 3.13)
project(notcurses VERSION 1.0.0
  DESCRIPTION "UI for modern terminal emulators"
  HOMEPAGE_URL "https://nick-black.com/dankwiki/index.php/notcurses"
  LANGUAGES C CXX)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_VISIBILITY_PRESET hidden)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

configure_file(tools/version.h.in include/version.h)

include(GNUInstallDirs)

find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
pkg_check_modules(TERMINFO REQUIRED tinfo>=6.1)
pkg_check_modules(AVUTIL REQUIRED libavutil>=56.0)
pkg_check_modules(AVFORMAT REQUIRED libavformat>=57.0)
pkg_check_modules(SWSCALE REQUIRED libswscale>=5.0)
find_library(LIBRT rt)

file(GLOB LIBSRCS CONFIGURE_DEPENDS src/lib/*.c)
add_library(notcurses SHARED ${LIBSRCS})
target_include_directories(notcurses
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
    "${TERMINFO_INCLUDE_DIR}"
    "${AVFORMAT_INCLUDE_DIR}"
    "${SWSCALE_INCLUDE_DIR}"
)
target_link_libraries(notcurses
  PRIVATE
    "${TERMINFO_LIBRARIES}"
    "${AVFORMAT_LIBRARIES}"
    "${SWSCALE_LIBRARIES}"
    "${LIBRT}"
)
target_link_directories(notcurses
  PRIVATE
    "${TERMINFO_LIBRARY_DIRS}"
    "${AVFORMAT_LIBRARY_DIRS}"
    "${SWSCALE_LIBRARY_DIRS}"
)
set_target_properties(notcurses PROPERTIES
  PUBLIC_HEADER "include/notcurses.h"
  VERSION ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
)

target_compile_options(notcurses
  PRIVATE
    -Wall -Wextra -W -Wshadow
)
target_compile_definitions(notcurses
  PUBLIC
    _XOPEN_SOURCE=700 # wcwidth(3) requires _XOPEN_SOURCE, and is in our headers
  PRIVATE
    FORTIFY_SOURCE=2 _GNU_SOURCE SOURCE_DEFAULT
)

file(GLOB DEMOSRCS CONFIGURE_DEPENDS src/demo/*.c)
add_executable(notcurses-demo ${DEMOSRCS})
target_include_directories(notcurses-demo PRIVATE include)
target_link_libraries(notcurses-demo
  PRIVATE
    notcurses
    Threads::Threads
)
target_compile_options(notcurses-demo
  PRIVATE
    -Wall -Wextra -W -Wshadow
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
    PRIVATE include "${TERMINFO_INCLUDE_DIR}"
  )
  target_link_libraries(${fe}
    PRIVATE notcurses "${TERMINFO_LIBRARIES}"
  )
  target_link_directories(${fe}
    PRIVATE "${TERMINFO_LIBRARY_DIRS}"
  )
endforeach()

file(GLOB INPUTSRCS CONFIGURE_DEPENDS src/input/*.cpp)
add_executable(notcurses-input ${INPUTSRCS})
target_include_directories(notcurses-input
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
)
target_link_libraries(notcurses-input
  PRIVATE
    notcurses
)
target_compile_options(notcurses-input
  PRIVATE
    -Wall -Wextra -W -Wshadow
)
target_compile_definitions(notcurses-input
  PRIVATE
    FORTIFY_SOURCE=2
)

file(GLOB VIEWSRCS CONFIGURE_DEPENDS src/view/*.cpp)
add_executable(notcurses-view ${VIEWSRCS})
target_include_directories(notcurses-view
  PRIVATE
    include
    "${PROJECT_BINARY_DIR}/include"
    "${AVUTIL_INCLUDE_DIR}"
)
target_link_directories(notcurses-view
  PRIVATE
    "${AVUTIL_LIBRARY_DIRS}"
)
target_link_libraries(notcurses-view
  PRIVATE
    notcurses
  PRIVATE
    "${AVUTIL_LIBRARIES}"
)
target_compile_options(notcurses-view
  PRIVATE
    -Wall -Wextra -W -Wshadow
)
target_compile_definitions(notcurses-view
  PRIVATE
    FORTIFY_SOURCE=2
)

file(GLOB TESTSRCS CONFIGURE_DEPENDS tests/*.cpp)
add_executable(notcurses-tester ${TESTSRCS})
find_package(GTest 1.9 REQUIRED)
target_include_directories(notcurses-tester
  PRIVATE
    include
    src/lib
)
target_link_libraries(notcurses-tester
  GTest::GTest
  notcurses
)
target_compile_options(notcurses-tester PRIVATE
  -Wall -Wextra -W -Wshadow
)
target_compile_definitions(notcurses-tester
  PRIVATE
    FORTIFY_SOURCE=2
)

gtest_discover_tests(notcurses-tester)
enable_testing()

configure_file(tools/notcurses.pc.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  @ONLY
)

include(CMakePackageConfigHelpers)

configure_package_config_file(tools/notcursesConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_PREFIX}/lib/notcurses/cmake
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfigVersion.cmake
  COMPATIBILITY SameMajorVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/notcursesConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/notcurses
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/notcurses.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig
)

file(GLOB TESTDATA CONFIGURE_DEPENDS tests/*.png tests/*.jpg tests/*.mkv)
install(FILES
  ${TESTDATA}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/notcurses
)

file(GLOB MANPAGES1 CONFIGURE_DEPENDS doc/man/man1/*)
file(GLOB MANPAGES3 CONFIGURE_DEPENDS doc/man/man3/*)
install(FILES
  ${MANPAGES1}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man1
)
install(FILES
  ${MANPAGES3}
  DESTINATION ${CMAKE_INSTALL_PREFIX}/man/man3
)

install(TARGETS notcurses-demo DESTINATION bin)
install(TARGETS notcurses-view DESTINATION bin)
install(TARGETS notcurses-input DESTINATION bin)
install(TARGETS notcurses
  LIBRARY
    DESTINATION ${CMAKE_INSTALL_LIBDIR}
    COMPONENT Libraries
    NAMELINK_COMPONENT Development
  PUBLIC_HEADER
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    COMPONENT Development
)

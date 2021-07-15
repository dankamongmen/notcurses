include(FindPkgConfig)
pkg_search_module(PYTHON3 REQUIRED python3)

set(NOTCURSES_PYTHON_SRC
    notcurses/context.c
    notcurses/misc.c
    notcurses/channels.c
    notcurses/main.c
    notcurses/notcurses-python.h
    notcurses/plane.c
)

add_library(
    dummy_python MODULE
    ${NOTCURSES_PYTHON_SRC}
)

target_include_directories(
    dummy_python
    PRIVATE ${PYTHON3_INCLUDE_DIRS}
    PRIVATE "${PROJECT_SOURCE_DIR}/include"
)

target_link_libraries(
    dummy_python
    PRIVATE
    notcurses
)

target_compile_options(
    dummy_python
    PRIVATE -Wall -Wextra
)

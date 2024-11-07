include(CMakeFindDependencyMacro)
find_dependency(Notcurses)

include("${CMAKE_CURRENT_LIST_DIR}/Notcurses++Targets.cmake")

set(Notcurses_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../include")
set(Notcurses_LIBRARY_DIRS "")
set(Notcurses_LIBRARIES Notcurses::Notcurses++)

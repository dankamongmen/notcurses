include(CMakeFindDependencyMacro)
find_dependency(Notcurses)

include("${CMAKE_CURRENT_LIST_DIR}/Notcurses++Targets.cmake")

set(Notcurses++_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../include")
set(Notcurses++_LIBRARY_DIRS "")
set(Notcurses++_LIBRARIES Notcurses::Notcurses++)

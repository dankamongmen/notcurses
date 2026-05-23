include(CMakeFindDependencyMacro)
find_dependency(NotcursesCore)

include("${CMAKE_CURRENT_LIST_DIR}/NotcursesTargets.cmake")

set(Notcurses_INCLUDE_DIRS "${CMAKE_CURRENT_LIST_DIR}/../../../include")
set(Notcurses_LIBRARY_DIRS "")
set(Notcurses_LIBRARIES Notcurses::Notcurses)

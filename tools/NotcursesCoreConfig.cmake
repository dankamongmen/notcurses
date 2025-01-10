include(CMakeFindDependencyMacro)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/NotcursesCoreTargets.cmake")

set(NotcursesCore_INCLUDE_DIRS "")
set(NotcursesCore_LIBRARY_DIRS "")
set(NotcursesCore_LIBRARIES Notcurses::NotcursesCore)

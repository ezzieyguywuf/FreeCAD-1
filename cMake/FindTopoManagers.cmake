# Try to find TopoManagers
# Once done, this will define:
# TOPOMANAGERS_FOUND - System has TopoManagers
# TOPOMANAGERS_INCLUDE_DIRS - The TopoManagers include directories
# TOPOMANAGERS_LIBRARIES - The libraries needed to use TopoManagers

set (TOPOMANAGERS_FOUND "NO")

find_path(TOPOMANAGERS_INCLUDE_DIR OccShape.h
    HINTS ${PROJECT_SOURCE_DIR}/../TopoManagers
    PATH_SUFFIXES include)

find_library(TOPOMANAGERS_LIBRARY 
    NAMES OccShape 
    HINTS ${PROJECT_SOURCE_DIR}/../TopoManagers/build
    PATH_SUFFIXES lib)

get_filename_component(TOPOMANAGERS_LIBRARIES_DIR ${TOPOMANAGERS_LIBRARY} DIRECTORY CACHE)

include(FindPackageHandleStandardArgs) # to handle standard args, provided by cMake (?)
find_package_handle_standard_args(TopoManagers DEFAULT_MSG
                                  TOPOMANAGERS_INCLUDE_DIR TOPOMANAGERS_LIBRARY)
if (TOPOMANAGERS_FOUND)
    message("-- Found TopoManagers")
    message("-- -- TopoManagers include directory = ${TOPOMANAGERS_INCLUDE_DIR}")
    message("-- -- TopoManagers library directory = ${TOPOMANAGERS_LIBRARIES_DIR}")
endif(TOPOMANAGERS_FOUND)

mark_as_advanced(TOPOMANAGERS_INCLUDE_DIR TOPOMANAGERS_LIBRARY)
set(TOPOMANAGERS_INCLUDE_DIRS ${TOPOMANAGERS_INCLUDE_DIR})
set(TOPOMANAGERS_LIBRARIES
    ISolidManager
    PrimitiveSolidManager
    CompoundSolidManager)

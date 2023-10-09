# --------------------------------------------------------------------------- #
#    Custom CMake find module for getopt                                      #
#                                                                             #
#    This module finds getopt include directories.                            #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(getopt [version] [EXACT] [REQUIRED])                    #
#                                                                             #
#    The results are stored in the following variables:                       #
#                                                                             #
#        getopt_FOUND         - True if headers are found                     #
#        getopt_INCLUDE_DIRS  - Include directories                           #
#        getopt_LIBRARIES     - Libraries to be linked                        #
#                                                                             #
#    The search results are saved in these persistent cache entries:          #
#                                                                             #
#        getopt_INCLUDE_DIR   - Directory containing headers                  #
#        getopt_LIBRARY       - The found library                             #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        getopt                                                               #
#                                                                             #
#    This find module is provided because the user may not have getopt        #
#    configuration file installed in the system default directories, which    #
#    happens to be the case for some of our main developers and testers.      #
#                                                                             #
#                              Niccolo' Iardella                              #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# Check if already in cache
if (getopt_INCLUDE_DIR AND getopt_LIBRARY)
    set(getopt_FOUND TRUE)
else ()

    # ----- Find the headers and library ------------------------------------ #
    # Note that find_path() creates a cache entry
    find_path(getopt_INCLUDE_DIR
              NAMES getopt.h
              DOC "getopt include directory.")

    # Note that find_library() creates a cache entry
    find_library(getopt_LIBRARY
                 NAMES getopt
                 DOC "getopt library.")

    # ----- Handle the standard arguments ----------------------------------- #
    # The following macro manages the QUIET, REQUIRED and version-related
    # options passed to find_package(). It also sets <PackageName>_FOUND if
    # REQUIRED_VARS are set.
    # REQUIRED_VARS should be cache entries and not output variables. See:
    # https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    find_package_handle_standard_args(
            getopt REQUIRED_VARS
            getopt_LIBRARY
            getopt_INCLUDE_DIR)
endif ()

# ----- Export the target --------------------------------------------------- #
if (getopt_FOUND)
    set(getopt_INCLUDE_DIRS "${getopt_INCLUDE_DIR}")

    if (NOT TARGET getopt)
        add_library(getopt INTERFACE IMPORTED)
        set_target_properties(
                getopt PROPERTIES
                IMPORTED_LOCATION "${getopt_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${getopt_INCLUDE_DIRS}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(getopt_INCLUDE_DIR
                 getopt_LIBRARY)

# --------------------------------------------------------------------------- #

# --------------------------------------------------------------------------- #
#    Custom CMake find module for GETOPT                                      #
#                                                                             #
#    This module finds GETOPT include directories.                            #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(GETOPT [version] [EXACT] [REQUIRED])                    #
#                                                                             #
#    The results are stored in the following variables:                       #
#                                                                             #
#        GETOPT_FOUND         - True if headers are found                     #
#        GETOPT_INCLUDE_DIRS  - Include directories                           #
#        GETOPT_LIBRARIES     - Libraries to be linked                        #
#                                                                             #
#    The search results are saved in these persistent cache entries:          #
#                                                                             #
#        GETOPT_INCLUDE_DIR   - Directory containing headers                  #
#        GETOPT_LIBRARY       - The found library                             #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        GETOPT                                                               #
#                                                                             #
#    This find module is provided because the user may not have GETOPT        #
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
if (GETOPT_INCLUDE_DIR)
    set(GETOPT_FOUND TRUE)
else ()

    # ----- Find the headers and library ------------------------------------ #
    # Note that find_path() creates a cache entry
    find_path(GETOPT_INCLUDE_DIR
              NAMES getopt.h
              DOC "GETOPT include directory.")

    # Note that find_library() creates a cache entry
    find_library(GETOPT_LIBRARY
                 NAMES getopt
                 DOC "GETOPT library.")

    if (WIN32)
        find_program(GETOPT_RUNTIME_LIBRARY
                     NAMES getopt.dll
                     DOC "GETOPT runtime library.")
    endif ()

    # ----- Handle the standard arguments ----------------------------------- #
    # The following macro manages the QUIET, REQUIRED and version-related
    # options passed to find_package(). It also sets <PackageName>_FOUND if
    # REQUIRED_VARS are set.
    # REQUIRED_VARS should be cache entries and not output variables. See:
    # https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    if (WIN32)
        find_package_handle_standard_args(
                GETOPT REQUIRED_VARS
                GETOPT_LIBRARY
                GETOPT_RUNTIME_LIBRARY
                GETOPT_INCLUDE_DIR)
    else ()
        find_package_handle_standard_args(
                GETOPT REQUIRED_VARS
                GETOPT_LIBRARY
                GETOPT_INCLUDE_DIR)
    endif ()
endif ()

# ----- Export the target --------------------------------------------------- #
if (GETOPT_FOUND)
    set(GETOPT_INCLUDE_DIRS "${GETOPT_INCLUDE_DIR}")

    if (NOT TARGET GETOPT)
        add_library(GETOPT INTERFACE IMPORTED)
        set_target_properties(
                GETOPT PROPERTIES
                IMPORTED_LOCATION "${GETOPT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${GETOPT_INCLUDE_DIRS}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(GETOPT_INCLUDE_DIR
                 GETOPT_LIBRARY)

# --------------------------------------------------------------------------- #

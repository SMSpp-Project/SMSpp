# --------------------------------------------------------------------------- #
#    Custom CMake find module for netCDF-C++                                  #
#                                                                             #
#    This module finds netCDF-C++ include directories and libraries.          #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(netCDFCxx [version] [EXACT] [REQUIRED])                 #
#                                                                             #
#    The results are stored in the following variables:                       #
#                                                                             #
#        netCDFCxx_FOUND         - True if headers are found                  #
#        netCDFCxx_INCLUDE_DIRS  - Include directories                        #
#        netCDFCxx_LIBRARIES     - Libraries to be linked                     #
#        netCDFCxx_VERSION       - Version number                             #
#                                                                             #
#    The search results are saved in these persistent cache entries:          #
#                                                                             #
#        netCDFCxx_INCLUDE_DIR   - Directory containing headers               #
#        netCDFCxx_LIBRARY       - The found library                          #
#                                                                             #
#    This module can read a search path from the variable:                    #
#                                                                             #
#        netCDFCxx_ROOT          - Preferred netCDF-C++ location              #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        netCDF::netCDFCxx                                                    #
#                                                                             #
#    This find module is provided because the CMake support from netCDF-C++   #
#    was found to be lacking. In particular, it appears that netCDFCxx does   #
#    not come with a CMake configuration (netCDFCxxConfig.cmake).             #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Requirements -------------------------------------------------------- #
# Try first with library's own configuration file, then with our find module.
find_package(netCDF QUIET NO_MODULE)
if (NOT netCDF_FOUND)
    find_package(netCDF REQUIRED)
    set(ncTarget "netCDF::netcdf")
else ()
    find_package(HDF5 QUIET)
    # Before 4.7.3, netCDF exported a target without namespace
    if ("${netCDF_VERSION}" VERSION_LESS "4.7.3")
        set(ncTarget "netcdf")
    else ()
        set(ncTarget "netCDF::netcdf")
    endif ()
endif ()

# ----- Check if already in cache ------------------------------------------- #
if (netCDFCxx_INCLUDE_DIR AND netCDFCxx_LIBRARY AND netCDFCxx_LIBRARY_DEBUG)
    set(netCDFCxx_FOUND TRUE)
else ()

    # ----- Find the headers and library ------------------------------------ #
    find_path(netCDFCxx_INCLUDE_DIR
              NAMES netcdf
              HINTS ${netCDFCxx_ROOT}
              DOC "netCDF-C++ include directory.")

    if (UNIX)
        find_library(netCDFCxx_LIBRARY
                     NAMES netcdf-cxx4 netcdf_c++4
                     HINTS ${netCDFCxx_ROOT}/lib
                     DOC "netCDF-C++ library.")
        set(netCDFCxx_LIBRARY_DEBUG ${netCDFCxx_LIBRARY})
    else ()
        find_library(netCDFCxx_LIBRARY
                     NAMES netcdf-cxx4 netcdf_c++4
                     HINTS ${netCDFCxx_ROOT}/lib
                     NO_DEFAULT_PATH
                     DOC "netCDF-C++ library.")

        find_library(netCDFCxx_LIBRARY_DEBUG
                     NAMES netcdf-cxx4
                     HINTS ${netCDFCxx_ROOT}/debug/lib
                     NO_DEFAULT_PATH
                     DOC "netCDF-C++ debug library.")
    endif ()

    # ----- Parse the version ----------------------------------------------- #
    # Get version from netCDF (there is no way to parse it from the headers)
    set(netCDFCxx_VERSION ${netCDF_VERSION})
    # TODO https://github.com/Unidata/netcdf-cxx4/issues/136
    #[[if (netCDFCxx_INCLUDE_DIR)
        file(STRINGS
                "${netCDFCxx_INCLUDE_DIR}/netcdfcpp_meta.h"
                _netCDFCxx_version_lines REGEX "#define NCXX_VERSION_(MAJOR|MINOR|PATCH)")

        string(REGEX REPLACE ".*NCXX_VERSION_MAJOR *\([0-9]*\).*" "\\1" _netCDFCxx_version_major "${_netCDFCxx_version_lines}")
        string(REGEX REPLACE ".*NCXX_VERSION_MINOR *\([0-9]*\).*" "\\1" _netCDFCxx_version_minor "${_netCDFCxx_version_lines}")
        string(REGEX REPLACE ".*NCXX_VERSION_PATCH *\([0-9]*\).*" "\\1" _netCDFCxx_version_patch "${_netCDFCxx_version_lines}")

        set(netCDFCxx_VERSION "${_netCDFCxx_version_major}.${_netCDFCxx_version_minor}.${_netCDFCxx_version_patch}")
        unset(_netCDFCxx_version_lines)
        unset(_netCDFCxx_version_major)
        unset(_netCDFCxx_version_minor)
        unset(_netCDFCxx_version_patch)
    endif ()]]

    # ----- Handle the standard arguments ----------------------------------- #
    # The following macro manages the QUIET, REQUIRED and version-related
    # options passed to find_package(). It also sets <PackageName>_FOUND if
    # REQUIRED_VARS are set.
    # REQUIRED_VARS should be cache entries and not output variables. See:
    # https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    find_package_handle_standard_args(
            netCDFCxx
            REQUIRED_VARS netCDFCxx_LIBRARY netCDFCxx_INCLUDE_DIR
            VERSION_VAR netCDFCxx_VERSION)
endif ()

# ----- Export the target --------------------------------------------------- #
if (netCDFCxx_FOUND)
    set(netCDFCxx_INCLUDE_DIRS "${netCDFCxx_INCLUDE_DIR}")
    set(netCDFCxx_LIBRARIES "${netCDFCxx_LIBRARY}")

    if (NOT TARGET netCDF::netCDFCxx)
        add_library(netCDF::netCDFCxx UNKNOWN IMPORTED)
        set_target_properties(
                netCDF::netCDFCxx PROPERTIES
                IMPORTED_LOCATION "${netCDFCxx_LIBRARY}"
                IMPORTED_LOCATION_DEBUG "${netCDFCxx_LIBRARY_DEBUG}"
                INTERFACE_INCLUDE_DIRECTORIES "${netCDFCxx_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${ncTarget};${HDF5_LIBRARIES}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(netCDFCxx_INCLUDE_DIR
                 netCDFCxx_LIBRARY
                 netCDFCxx_LIBRARY_DEBUG
                 netCDFCxx_VERSION)

# --------------------------------------------------------------------------- #

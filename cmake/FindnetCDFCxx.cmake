# --------------------------------------------------------------------------- #
#    Custom CMake find module for netCDF-C++                                  #
#                                                                             #
#    This module finds netCDF-C++ include directories and libraries.          #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(netCDF-C++ [version] [EXACT] [REQUIRED])                #
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
#    This module reads hints about search locations from variables:           #
#                                                                             #
#        NETCDFCXX_INC           - Preferred include directory                #
#        NETCDFCXX_LIB           - Preferred library directory                #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        netCDF::netCDFCxx                                                    #
#                                                                             #
#    This find module is provided because the CMake support from netCDF-C++   #
#    was found to be lacking. In particular, it appears that netCDFCxx does   #
#    not come with a CMake configuration (netCDFCxxConfig.cmake).             #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Requirements -------------------------------------------------------- #
# NetCDF's configuration file has a bug that prevents it from working
# under macOS 11.0, so in that case we use our own find module.
# TODO: This should be a temporary fix
if (${CMAKE_SYSTEM} MATCHES "Darwin-20.1.0")
    find_package(netCDF REQUIRED)
    set(ncTarget "netCDF::netcdf")
else ()
    find_package(netCDF REQUIRED CONFIG)
    # Before 4.7.3, netCDF exported a target without namespace
    if ("${netCDF_VERSION}" VERSION_LESS "4.7.3")
        set(ncTarget "netcdf")
    else ()
        set(ncTarget "netCDF::netcdf")
    endif ()
endif ()

# Check if already in cache
if (netCDFCxx_INCLUDE_DIR AND netCDFCxx_LIBRARY)
    set(netCDFCxx_FOUND TRUE)
else ()

    # ----- Find the headers and library ------------------------------------ #
    # Note that find_path() creates a cache entry
    find_path(netCDFCxx_INCLUDE_DIR netcdf
              HINTS ${NETCDFCXX_INC}
              DOC "netCDF-C++ include directory.")

    # Note that find_library() creates a cache entry
    find_library(netCDFCxx_LIBRARY
                 NAMES netcdf-cxx4 netcdf_c++4
                 HINTS ${NETCDFCXX_LIB}
                 DOC "netCDF-C++ library.")

    # Get version from netCDF (there is no way to parse it from the headers)
    set(netCDFCxx_VERSION ${netCDF_VERSION})

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
                INTERFACE_INCLUDE_DIRECTORIES "${netCDFCxx_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${ncTarget}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(netCDFCxx_INCLUDE_DIR
                 netCDFCxx_LIBRARY
                 netCDFCxx_VERSION)

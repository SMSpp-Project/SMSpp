# --------------------------------------------------------------------------- #
#    Custom CMake find module for netCDF-C                                    #
#                                                                             #
#    This module finds netCDF include directories and libraries.              #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(netCDF [version] [EXACT] [REQUIRED])                    #
#                                                                             #
#    The results are stored in the following output variables:                #
#                                                                             #
#        netCDF_FOUND         - True if headers are found                     #
#        netCDF_INCLUDE_DIRS  - Include directories                           #
#        netCDF_LIBRARIES     - Libraries to be linked                        #
#        netCDF_VERSION       - Version number                                #
#                                                                             #
#    The search results are saved in these persistent cache entries:          #
#                                                                             #
#        netCDF_INCLUDE_DIR   - Directory containing headers                  #
#        netCDF_LIBRARY       - The found library                             #
#                                                                             #
#    This module can read a search path from the variable:                    #
#                                                                             #
#        netCDF_ROOT          - Preferred netCDF-C location                   #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        netCDF::netcdf                                                       #
#                                                                             #
#    This find module is provided because the CMake support from netCDF       #
#    was found to be lacking. In particular, it appears that netCDF's         #
#    configuration file has the paths for its dependencies (HDF5, Zlib)       #
#    hardcoded, and that doesn't work under macOS 11.0.                       #
#    We use it in netCDF-C++ find module.                                     #
#                                                                             #
#                              Niccolo' Iardella                              #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Requirements -------------------------------------------------------- #
find_package(HDF5 QUIET REQUIRED)

# Check if already in cache
if (netCDF_INCLUDE_DIR AND netCDF_LIBRARY)
    set(netCDF_FOUND TRUE)
else ()

    # ----- Find the headers and library ------------------------------------ #
    # Note that find_path() creates a cache entry
    find_path(netCDF_INCLUDE_DIR netcdf.h
              DOC "netCDF include directory.")

    # Note that find_library() creates a cache entry
    find_library(netCDF_LIBRARY
                 NAMES netcdf
                 DOC "netCDF library.")

    # TODO: Find a way to get the version
    # ----- Handle the standard arguments ----------------------------------- #
    # The following macro manages the QUIET, REQUIRED and version-related
    # options passed to find_package(). It also sets <PackageName>_FOUND if
    # REQUIRED_VARS are set.
    # REQUIRED_VARS should be cache entries and not output variables. See:
    # https://cmake.org/cmake/help/latest/module/FindPackageHandleStandardArgs.html
    find_package_handle_standard_args(
            netCDF
            REQUIRED_VARS netCDF_LIBRARY netCDF_INCLUDE_DIR)
endif ()

# ----- Export the target --------------------------------------------------- #
if (netCDF_FOUND)
    set(netCDF_INCLUDE_DIRS "${netCDF_INCLUDE_DIR}")
    set(netCDF_LIBRARIES "${netCDF_LIBRARY}")

    if (NOT TARGET netCDF::netcdf)
        add_library(netCDF::netcdf UNKNOWN IMPORTED)
        set_target_properties(
                netCDF::netcdf PROPERTIES
                IMPORTED_LOCATION "${netCDF_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${netCDF_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "${HDF5_LIBRARIES}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(netCDF_INCLUDE_DIR
                 netCDF_LIBRARY
                 netCDF_VERSION)

# --------------------------------------------------------------------------- #

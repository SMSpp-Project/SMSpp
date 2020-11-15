# --------------------------------------------------------------------------- #
#    Custom CMake find module for netCDF                                      #
#                                                                             #
#    This find module is provided because the CMake support from netCDF       #
#    was found to be lacking. In particular, it appears that netCDF's         #
#    configuration file has the paths for its depencencies (HDF5, Zlib)       #
#    hardcoded, and this doesn't work under macOS 11.0.                       #
#                                                                             #
#    Accepts the following PATHS:                                             #
#                                                                             #
#    - NETCDF_INC - Custom path to netCDF headers                             #
#    - NETCDF_LIB - Custom path to netCDF libraries                           #
#                                                                             #
#    Provides (at least) the following variables:                             #
#                                                                             #
#    - netCDF_FOUND - Whether netCDF was found or not                         #
#    - netCDF_INCLUDE_DIRS - Include directories                              #
#    - netCDF_LIBRARIES - Libraries to link                                   #
#    - netCDF::netcdf - A target to use with target_link_libraries()          #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Requirements -------------------------------------------------------- #
find_package(HDF5 QUIET REQUIRED COMPONENTS C HL)

# ----- Find the headers and library ---------------------------------------- #
find_path(netCDF_INCLUDE_DIR netcdf.h
          PATHS ${NETCDF_INC}
          DOC "netCDF include directory")

find_library(netCDF_LIBRARY
             NAMES netcdf
             PATHS ${NETCDF_LIB}
             DOC "netCDF library")

# TODO: Find a way to get the version
# ----- Handle the standard arguments --------------------------------------- #
find_package_handle_standard_args(
        netCDF
        REQUIRED_VARS netCDF_LIBRARY netCDF_INCLUDE_DIR)

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
                INTERFACE_LINK_LIBRARIES "${HDF5_C_LIBRARIES}")
    endif ()
endif ()

mark_as_advanced(netCDF_INCLUDE_DIR
                 netCDF_LIBRARY
                 netCDF_VERSION)

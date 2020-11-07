# --------------------------------------------------------------------------- #
#    Custom CMake find module for netCDF-C++                                  #
#                                                                             #
#    This find module is provided because the CMake support from netCDF-C++   #
#    was found to be lacking. In particular, it appears that netCDFCxx does   #
#    not come with a CMake configuration (netCDFCxxConfig.cmake).             #
#                                                                             #
#    Accepts the following PATHS:                                             #
#                                                                             #
#    - NETCDFCXX_INC - Custom path to netCDF-C++ headers                      #
#    - NETCDFCXX_LIB - Custom path to netCDF-C++ libraries                    #
#                                                                             #
#    Provides (at least) the following variables:                             #
#                                                                             #
#    - netCDFCxx_FOUND - Whether netCDF-C++ was found or not                  #
#    - netCDFCxx_INCLUDE_DIRS - Include directories                           #
#    - netCDFCxx_LIBRARIES - Libraries to link                                #
#    - netCDF::netCDFCxx - A target to use with target_link_libraries()       #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Requirements -------------------------------------------------------- #
find_package(netCDF REQUIRED)

# ----- Find the headers and library ---------------------------------------- #
find_path(netCDFCxx_INCLUDE_DIR netcdf
          PATHS ${NETCDFCXX_INC}
          DOC "netCDF-C++ include directory")

find_library(netCDFCxx_LIBRARY
             NAMES netcdf-cxx4 netcdf_c++4
             PATHS ${NETCDFCXX_LIB}
             DOC "netCDF-C++ library")

# Get version from netCDF (there is no way to parse it from the headers)
set(netCDFCxx_VERSION ${netCDF_VERSION})

# ----- Handle the standard arguments --------------------------------------- #
find_package_handle_standard_args(
        netCDFCxx
        REQUIRED_VARS netCDFCxx_LIBRARY netCDFCxx_INCLUDE_DIR
        VERSION_VAR netCDFCxx_VERSION)

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
                INTERFACE_LINK_LIBRARIES "netCDF::netcdf")
    endif ()
endif ()

mark_as_advanced(netCDFCxx_INCLUDE_DIR
                 netCDFCxx_LIBRARY
                 netCDFCxx_VERSION)

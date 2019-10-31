# --------------------------------------------------------------------------- #
#    CMake find module for NetCDF-C++                                         #
#                                                                             #
#    Accepts the following HINTS:                                             #
#                                                                             #
#    - NETCDFCXX_INC - Custom path to NetCDF-C++ headers                      #
#    - NETCDFCXX_LIB - Custom path to NetCDF-C++ libraries                    #
#                                                                             #
#    Provides the following imported targets:                                 #
#                                                                             #
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

# ----- NetCDF-C++ ---------------------------------------------------------- #
# Find headers
find_path(netCDFCxx_INCLUDE_DIR
          NAMES netcdf
          HINTS ${NETCDFCXX_INC}
          DOC "NetCDF-C++ include directories")
mark_as_advanced(netCDFCxx_INCLUDE_DIR)

# Find library
find_library(netCDFCxx_LIBRARY
             NAMES netcdf-cxx4 netcdf_c++4
             HINTS ${NETCDFCXX_LIB}
             DOC "NetCDF-C++ library")
mark_as_advanced(netCDFCxx_LIBRARY)

# Handle standard arguments
find_package_handle_standard_args(
        netCDFCxx
        REQUIRED_VARS netCDFCxx_LIBRARY netCDFCxx_INCLUDE_DIR
        VERSION_VAR netCDF_VERSION)

# Define target
if (netCDFCxx_FOUND)
    set(netCDFCxx_INCLUDE_DIRS "${netCDFCxx_INCLUDE_DIR}")
    set(netCDFCxx_LIBRARIES "${netCDFCxx_LIBRARY}")

    if (NOT TARGET NetCDF::NetCDFCxx)
        add_library(NetCDF::NetCDFCxx UNKNOWN IMPORTED)
        set_target_properties(
                NetCDF::NetCDFCxx PROPERTIES
                IMPORTED_LOCATION "${netCDFCxx_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${netCDFCxx_INCLUDE_DIR}"
                INTERFACE_LINK_LIBRARIES "netcdf")
    endif ()
endif ()

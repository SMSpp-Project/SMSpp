# --------------------------------------------------------------------------- #
#    CMake find module for Eigen3                                             #
#                                                                             #
#    This find module is provided because the user may not have Eigen3        #
#    find module installed in the system default directories.                 #
#    It is a wrapper that tries to find Eigen3 manually in case the           #
#    find module provided by Eigen3 is not found or fails.                    #
#                                                                             #
#    Accepts the following HINT:                                              #
#                                                                             #
#    - EIGEN3_ROOT - Custom Eigen3 root directory                             #
#                                                                             #
#    Provides (at least) the following variables:                             #
#                                                                             #
#    - Eigen3_FOUND - Whether Eigen3 was found or not                         #
#    - EIGEN3_INCLUDE_DIRS - Include directories                              #
#    - Eigen3::Eigen - A target to use with target_link_libraries()           #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
# TODO: Support version check
# Try to find a CMake-built Eigen3 ------------------------------------------ #
find_package(Eigen3 CONFIG QUIET)
if (Eigen3_FOUND)
    return()
endif ()

# Try to find a Eigen3 "manually" ------------------------------------------- #
find_path(EIGEN3_INCLUDE_DIR Eigen/Dense
          PATHS ${EIGEN3_ROOT})
mark_as_advanced(EIGEN3_INCLUDE_DIR)

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(Eigen3 REQUIRED_VARS EIGEN3_INCLUDE_DIR)

if (Eigen3_FOUND)
    set(EIGEN3_INCLUDE_DIRS "${EIGEN3_INCLUDE_DIR}")

    if (NOT TARGET Eigen3::Eigen)
        add_library(Eigen3::Eigen UNKNOWN IMPORTED)
        set_target_properties(
                Eigen3::Eigen PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}")
    endif ()
endif ()

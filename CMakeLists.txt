# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ library                                        #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11)
project(SMS++ VERSION 0.3.2 LANGUAGES CXX)

# ----- Settings ------------------------------------------------------------ #
include(CTest)

# ----- Requirements -------------------------------------------------------- #
find_package(Boost 1.71.0 REQUIRED)

# This is to use the find modules we provide
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# This will try first with Eigen3 own configuration file,
# then with the find module we provide.
find_package(Eigen3 QUIET NO_MODULE)
if (NOT Eigen3_FOUND)
    find_package(Eigen3 REQUIRED)
endif ()

# This will straight use our find module, as netCDF-C++
# configuration file doesn't work.
find_package(netCDFCxx REQUIRED)

list(REMOVE_AT CMAKE_MODULE_PATH -1)

# ----- Library ------------------------------------------------------------- #
add_library(SMSpp)
target_compile_features(SMSpp PUBLIC cxx_std_17)

target_sources(SMSpp PRIVATE
               src/AbstractBlock.cpp
               src/Block.cpp
               src/BlockSolverConfig.cpp
               src/C05Function.cpp
               src/BendersBFunction.cpp
               src/BendersBlock.cpp
               src/ColVariable.cpp
               src/ColVariableSolution.cpp
               src/Configuration.cpp
               src/Constraint.cpp
               src/DataMapping.cpp
               src/DQuadFunction.cpp
               src/FRealObjective.cpp
               src/FRowConstraint.cpp
               src/Function.cpp
               src/LagBFunction.cpp
               src/LinearFunction.cpp
               src/Objective.cpp
               src/OneVarConstraint.cpp
               src/PolyhedralFunction.cpp
               src/RBlockConfig.cpp
               src/RowConstraintSolution.cpp
               src/PolyhedralFunctionBlock.cpp
               src/Solver.cpp
               src/ThinComputeInterface.cpp
               src/Variable.cpp)

# Different INSTALL_INTERFACE and BUILD_INTERFACE paths are used when
# generating the target import file (***Targets.cmake), that must be different
# for local build and installation.
target_include_directories(
        SMSpp PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/SMS++>)

# PRIVATE for build specifications
# INTERFACE for usage requirements
# PUBLIC for both
target_link_libraries(SMSpp PUBLIC
                      Boost::boost
                      Eigen3::Eigen
                      netCDF::netCDFCxx)

# Alias for testing
add_library(SMS++::SMSpp ALIAS SMSpp)

# ----- Install instructions ------------------------------------------------ #
# For installing the library and all its CMake modules on the system.
# Not required for local builds (see below).

include(GNUInstallDirs)

# Install library
install(TARGETS SMSpp
        EXPORT SMSpp-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install headers
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SMS++)

# Install target import file
install(EXPORT SMSpp-targets
        FILE SMSppTargets.cmake
        NAMESPACE SMS++::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SMSpp)

# Generate package version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

# Generate package configuration file
configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/SMSppConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SMSpp)

# Install package version and configuration files
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SMSpp)

# Shipping custom find modules should not be encouraged because third
# party developers should provide them.
# We ship FindnetCDFCxx because the netCDF-C++ configuration file doesn't
# work, and FindEigen3 because the user may want not to install Eigen3.
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDFCxx.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SMSpp)

# ----- Export from the build tree ------------------------------------------ #
# This is needed so other projects can find the library in a local build.
# It does not affect installation.

if (CMAKE_EXPORT_PACKAGE_REGISTRY)

    export(EXPORT SMSpp-targets
           FILE ${CMAKE_CURRENT_BINARY_DIR}/SMSppTargets.cmake
           NAMESPACE SMS++::)

    # Export also the find modules for the local config file to use it
    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindEigen3.cmake
                   COPYONLY)

    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDFCxx.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindnetCDFCxx.cmake
                   COPYONLY)

    # Register package in user's package registry (~/.cmake/registry)
    export(PACKAGE SMSpp)
endif ()

# ----- Unit testing -------------------------------------------------------- #
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

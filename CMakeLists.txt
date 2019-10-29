# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ library                                        #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.8...3.15)
# TODO: set proper version
project(SMS++ VERSION 1.0.0 LANGUAGES CXX)

# ----- Requirements -------------------------------------------------------- #
# find_package() is used to find libraries and automatically configure them.
# It requires either a package configuration module (***Config.cmake) or a
# find module (Find***.cmake), usually provided by libraries.

# We can provide a settings file with -DSETTINGS="filename"
if ((CMAKE_PROJECT_NAME STREQUAL PROJECT_NAME) AND SETTINGS)
    message(STATUS "Using CMake custom settings in ${SETTINGS}")
    include(${SETTINGS} OPTIONAL)
endif ()

# This is to use the find modules we provide
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Boost 1.71.0 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(netCDF REQUIRED)

# ----- Library ------------------------------------------------------------- #
add_library(SMSpp)
target_compile_features(SMSpp PUBLIC cxx_std_17)

target_sources(SMSpp PRIVATE
               src/AbstractBlock.cpp
               src/Block.cpp
               src/ColVariable.cpp
               src/ColVariableSolution.cpp
               src/Configuration.cpp
               src/Constraint.cpp
               src/DQuadFunction.cpp
               src/FRealObjective.cpp
               src/FRowConstraint.cpp
               src/LagBFunction.cpp
               src/LinearFunction.cpp
               src/Objective.cpp
               src/OneVarConstraint.cpp
               src/PolyhedralFunction.cpp
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
                      NetCDF::NetCDFCxx)

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
# We ship FindnetCDF because the provided one has shortcomings.
# We also ship FindEigen3 to support Eigen install-less usage.
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDF.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/SMSpp)

# ----- Export from the build tree ------------------------------------------ #
# This is needed so other projects can find the library in a local build.
# It does not affect installation.

if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(CMAKE_EXPORT_PACKAGE_REGISTRY ON)

    export(EXPORT SMSpp-targets
           FILE ${CMAKE_CURRENT_BINARY_DIR}/SMSppTargets.cmake
           NAMESPACE SMS++::)

    # Export also the find modules for the local config file to use it
    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindEigen3.cmake
                   COPYONLY)

    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDF.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindnetCDF.cmake
                   COPYONLY)

    # Register package in user's package registry (~/.cmake/registry)
    export(PACKAGE SMSpp)
endif ()

# ----- Unit testing -------------------------------------------------------- #
if (CMAKE_TESTING_ENABLED)
    add_subdirectory(test)
endif()

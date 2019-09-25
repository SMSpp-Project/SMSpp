# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ library                                        #
#                                                                             #
#    If Boost, Eigen3 and netCDF are installed properly,                      #
#    this should run just fine.                                               #
#    Otherwise, edit CMakeCustom.txt to set the proper paths.                 #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.8)
# ----- Project definition and directories ---------------------------------- #
# TODO: set proper version
project(SMS++ VERSION 1.0.0)

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ----- Requirements -------------------------------------------------------- #
# find_package() is used to find libraries and automatically configure them.
# It requires either a package configuration module (***Config.cmake) or a
# find module (Find***.cmake), usually provided by libraries.

# If we are configuring from the umbrella project, we assume that the valid
# settings are there.
# If we are building this project alone we use its settings but we check
# also for settings in its parent directory.
# TODO: Check if this behaviour is correct
if (NOT SMS++_CONFIG_ALL)
    include(CMakeCustom.txt OPTIONAL)
    include(../CMakeCustom.txt OPTIONAL)
endif ()

# This is to use the find modules we provide
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(netCDF REQUIRED)

# ----- Library ------------------------------------------------------------- #
add_library(SMSpp STATIC)
target_compile_features(SMSpp PUBLIC cxx_std_17)

target_sources(SMSpp PRIVATE
               ${PROJECT_SOURCE_DIR}/AbstractBlock.cpp
               ${PROJECT_SOURCE_DIR}/Block.cpp
               ${PROJECT_SOURCE_DIR}/ColVariable.cpp
               ${PROJECT_SOURCE_DIR}/ColVariableSolution.cpp
               ${PROJECT_SOURCE_DIR}/Configuration.cpp
               ${PROJECT_SOURCE_DIR}/Constraint.cpp
               ${PROJECT_SOURCE_DIR}/DQuadFunction.cpp
               ${PROJECT_SOURCE_DIR}/FRealObjective.cpp
               ${PROJECT_SOURCE_DIR}/FRowConstraint.cpp
               ${PROJECT_SOURCE_DIR}/LagBFunction.cpp
               ${PROJECT_SOURCE_DIR}/LinearFunction.cpp
               ${PROJECT_SOURCE_DIR}/Objective.cpp
               ${PROJECT_SOURCE_DIR}/OneVarConstraint.cpp
               ${PROJECT_SOURCE_DIR}/PolyhedralFunction.cpp
               ${PROJECT_SOURCE_DIR}/Solver.cpp
               ${PROJECT_SOURCE_DIR}/ThinComputeInterface.cpp
               ${PROJECT_SOURCE_DIR}/Variable.cpp
               )

# Different INSTALL_INTERFACE and BUILD_INTERFACE paths are used when
# generating the target import file (***Targets.cmake), that must be different
# for local build and installation.
target_include_directories(SMSpp PUBLIC
                           $<INSTALL_INTERFACE:include/SMS++>
                           $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>)

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
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/SMS++)

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
        DESTINATION ${INSTALL_CONFIGDIR})

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
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR})

# Install package version and configuration files
# TODO: Check if we should install also find modules
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfigVersion.cmake
        # ${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
        # ${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDF.cmake
        DESTINATION ${INSTALL_CONFIGDIR})

# ----- Export from the build tree ------------------------------------------ #
# This is needed so other projects can find the library in a local build.
# It does not affect installation.

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

# FIXME: I think that exporting this is a bad idea
#configure_file(${CMAKE_CURRENT_LIST_DIR}/CMakeCustom.txt
#               ${CMAKE_CURRENT_BINARY_DIR}/CMakeCustom.txt
#               COPYONLY)

# Register package in user's package registry (~/.cmake/registry)
export(PACKAGE SMSpp)

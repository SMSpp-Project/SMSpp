# ----- Project definition and directories -------------------------------------
cmake_minimum_required(VERSION 3.8)
project(SMS++
        VERSION 1.0.0 # TODO: set proper version
        )

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)

# This is to find the Find***.cmake modules we provide
list(INSERT CMAKE_MODULE_PATH 0 ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ----- Requirements -----------------------------------------------------------
include(CMakeConfig.txt)

# FindBoost is provided with CMake so this should never fail.
# It already supports BOOST_ROOT as HINT.
find_package(Boost REQUIRED)

# We provide FindEigen3 so this should never fail.
# It already supports EIGEN3_ROOT as HINT.
find_package(Eigen3 REQUIRED)

# FindEigen3 may not be installed, so we give the option
# to override it by setting EIGEN3_ROOT.
#if (EIGEN3_ROOT)
#    # EIGEN3_INCLUDE_DIRS is enough because it is a header-only library
#    find_path(EIGEN3_INCLUDE_DIRS Eigen/Dense
#              PATHS ${EIGEN3_ROOT}
#              NO_DEFAULT_PATH)
#
#    if (NOT EIGEN3_INCLUDE_DIRS)
#        message(FATAL_ERROR "Eigen not found in ${EIGEN3_ROOT}!")
#    endif ()
#else ()
#    find_package(Eigen3 REQUIRED)
#endif ()

message(STATUS "Found Eigen3: ${EIGEN3_INCLUDE_DIRS}")

#find_package(netCDF REQUIRED)

# FindnetCDF may not be installed, so we give the option
# to override both netCDF_INCLUDE_DIR and netCDF_LIB_DIR by setting
# NETCDF_INC and NETCDF_LIB.
#if (NETCDF_INC)
#    find_path(netCDF_INCLUDE_DIR netcdf
#              PATHS ${NETCDF_INC}
#              NO_DEFAULT_PATH)
#
#    if (NOT netCDF_INCLUDE_DIR)
#        message(FATAL_ERROR "netCDF headers not found in ${NETCDF_INC}!")
#    endif ()
#
#    if (NETCDF_LIB)
#        message(STATUS "NETCDF_LIB is True, value: ${NETCDF_LIB}")
#
#        find_library(netCDF_LIBRARIES
#                     NAMES netcdf-cxx4
#                     PATHS ${NETCDF_LIB}
#                     NO_DEFAULT_PATH)
#
#        if (NOT netCDF_LIBRARIES)
#            message(FATAL_ERROR "netCDF library not found in ${NETCDF_LIB}!")
#        else ()
#            set(netCDF_LIB_DIR ${NETCDF_LIB})
#        endif ()
#    else ()
#        message(FATAL_ERROR "You must provide both NETCDF_INC and NETCDF_LIB!")
#    endif ()
#else ()
#    find_package(netCDF REQUIRED)
#endif()

message(STATUS "Found netCDF: ${netCDF_INCLUDE_DIR} ${netCDF_LIB_DIR}")

# ----- Library ----------------------------------------------------------------
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

target_include_directories(SMSpp PUBLIC
                           $<INSTALL_INTERFACE:include/SMS++>
                           $<BUILD_INTERFACE:${PROJECT_INCLUDE_DIR}>)

target_link_libraries(SMSpp PUBLIC Boost::boost Eigen3)
#target_include_directories(SMSpp PUBLIC
#                           ${Boost_INCLUDE_DIRS}
#                           ${EIGEN3_INCLUDE_DIRS})

# Alias for testing
add_library(SMS++::SMSpp ALIAS SMSpp)

# ----- Install instructions ---------------------------------------------------
include(GNUInstallDirs)
set(INSTALL_CONFIGDIR ${CMAKE_INSTALL_LIBDIR}/cmake/SMS++)

# Install library
install(TARGETS SMSpp
        EXPORT  SMSpp-targets
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install headers
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/SMS++)

# Install export file *Targets.cmake
install(EXPORT SMSpp-targets
        FILE   SMSppTargets.cmake
        NAMESPACE SMS++::
        DESTINATION ${INSTALL_CONFIGDIR})

# Create *ConfigVersion.cmake
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

# Create *Config.cmake
configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/SMSppConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfig.cmake
        INSTALL_DESTINATION ${INSTALL_CONFIGDIR})

# Install the Config and ConfigVersion
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfig.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/SMSppConfigVersion.cmake
        DESTINATION ${INSTALL_CONFIGDIR})

# ----- Export from the build tree ---------------------------------------------
export(EXPORT SMSpp-targets
       FILE ${CMAKE_CURRENT_BINARY_DIR}/SMSppTargets.cmake
       NAMESPACE SMS++::)

# Register package in user's package registry (~/.cmake/registry)
export(PACKAGE SMSpp)

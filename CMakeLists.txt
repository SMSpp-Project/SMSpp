# ----- Project description and directories ------------------------------------
cmake_minimum_required(VERSION 3.8)
project(SMS++
        VERSION 1.0.0 # TODO: set proper version
        HOMEPAGE_URL "https://gitlab.com/frangio68/sms_plus_plus"
        DESCRIPTION "SMS++ main library")

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ----- Requirements -----------------------------------------------------------
find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)

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

target_link_libraries(SMSpp PUBLIC Boost::boost Eigen3::Eigen)

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

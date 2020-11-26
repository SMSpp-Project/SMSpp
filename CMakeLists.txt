# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ library                                        #
#                                                                             #
#    This file allows one to build the library using CMake.                   #
#    To do so, you can use the following commands:                            #
#                                                                             #
#        $ cmake -S <source-path> -B <build-path>                             #
#        $ cmake --build <build-path>                                         #
#                                                                             #
#    The following command also installs the library in the system:           #
#                                                                             #
#        $ cmake --build <build-path> --target install                        #
#                                                                             #
#    Note: this file tries to respect the principles of Modern CMake,         #
#          keep it in mind when editing it.                                   #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.11)
project(SMSpp VERSION 0.3.2 LANGUAGES CXX)

# These variables make the code harder to read but easier to change.
set(modName ${PROJECT_NAME})
set(modNamespace "SMS++")
set(modHeader "${modName}Config.h")
set(modTargets "${modName}Targets")

# ----- Settings ------------------------------------------------------------ #
# This creates an ENABLE_TESTING option (default: ON) and enables the testing
# using the ctest executable.
# See: https://cmake.org/cmake/help/latest/manual/ctest.1.html
include(CTest)

# ----- Requirements -------------------------------------------------------- #
# The Boost find module is provided by CMake,
# so this command will always work.
find_package(Boost 1.71.0 REQUIRED)

# This adds the cmake directory to the find module search paths,
# allowing us to use our find modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# This will try first with Eigen3 own configuration file,
# then with our find module.
find_package(Eigen3 NO_MODULE)
if (NOT Eigen3_FOUND)
    find_package(Eigen3 REQUIRED)
endif ()

# This will straight use our find module, as the configuration file
# that ships with netCDF-C++ doesn't work.
find_package(netCDFCxx REQUIRED)

# This removes from the search paths the cmake directory we added above.
list(REMOVE_AT CMAKE_MODULE_PATH -1)

# ----- Configuration header ------------------------------------------------ #
# This will generate a *Config.h header in the build directory.
configure_file(cmake/${modHeader}.in ${modHeader})

# ----- Library ------------------------------------------------------------- #
# With the BUILD_SHARED_LIBS variable we can specify if the library will
# be STATIC or SHARED, so no reason to do it now.
add_library(${modName})
target_compile_features(${modName} PUBLIC cxx_std_17)

# This definition is needed because the version of Clang shipped with
# macOS Big Sur has an issue with boost::any type recognition.
# TODO: Remove this when it's not needed anymore
if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_SYSTEM STREQUAL "Darwin-20.1.0")
    target_compile_definitions(${modName} PUBLIC CLANG_1200_0_32_27_PATCH)
endif ()

# When adding source files with target_sources(), PRIVATE means that the files
# should only be added to this library, whereas PUBLIC means they should be
# added to this library and to any target that links to it.
# INTERFACE can be used for sources that should not be added to this library
# but should be added to anything that links to it.
# Note: do not GLOB files here.
target_sources(${modName} PRIVATE
               src/AbstractBlock.cpp
               src/Block.cpp
               src/BlockSolverConfig.cpp
               src/C05Function.cpp
               src/BendersBFunction.cpp
               src/BendersBlock.cpp
               src/ColRowSolution.cpp
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

# When using target_include_directories(), PUBLIC means that any targets
# that link to this target also need that include directory.
# Other options are PRIVATE (only affect the current target, not dependencies),
# and INTERFACE (only needed for dependencies).

# Different INSTALL_INTERFACE and BUILD_INTERFACE paths are used when
# generating the target import file (***Targets.cmake).
# This means that if a target finds this library in its build directory
# will look into the BUILD_INTERFACE path for its headers, if it finds it
# installed in the system will look into the INSTALL_INTERFACE path.
target_include_directories(
        ${modName} PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include/${modNamespace}>)

# When linking other targets to the library with target_link_libraries(),
# PRIVATE means that the libraries will be linked only to this library,
# PUBLIC means they will be linked also to the targets that depend on this
# library, INTERFACE means they will be linked only to the targets that depend
# on this library.
target_link_libraries(${modName} PUBLIC
                      Boost::boost
                      Eigen3::Eigen
                      netCDF::netCDFCxx)

# This alias is defined so that executables in this same project can use
# the library with this notation.
add_library(${modNamespace}::${modName} ALIAS ${modName})

# ----- Install instructions ------------------------------------------------ #
# The following commands are used when installing the library
# and its CMake configuration files on the system.
# They are not required for local builds (see below).

include(GNUInstallDirs)

# Install the library
install(TARGETS ${modName}
        EXPORT ${modTargets}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

# Install the headers
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})

# Install the auto-generated configuration header (see above).
install(FILES ${PROJECT_BINARY_DIR}/${modName}Config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})

# (Generate and) install the target import file, that allows other
# CMake projects to import the target.
install(EXPORT ${modTargets}
        NAMESPACE ${modNamespace}::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Generate the package version file, that allows other
# CMake projects to know the version.
include(CMakePackageConfigHelpers)
write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}ConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY AnyNewerVersion)

# Generate the package configuration file, that allows other
# CMake projects to find the library with find_package().
configure_package_config_file(
        ${CMAKE_CURRENT_LIST_DIR}/cmake/${modName}Config.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}Config.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Install the package version and configuration files.
install(FILES
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/${modName}ConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Shipping custom find modules should not be encouraged because third
# party developers should provide for them.
# We ship FindnetCDFCxx because the netCDF-C++ configuration file doesn't
# work, and FindEigen3 because the user may want not to install Eigen3.
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDFCxx.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# We also ship FindnetCDF because current netCDF configuration file
# has a bug that prevents it from working on macOS 11.0.
# TODO: This should be a temporary fix
if (${CMAKE_SYSTEM} MATCHES "Darwin-20.1.0")
    install(FILES
            ${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDF.cmake
            DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})
endif ()

# ----- Export from the build tree ------------------------------------------ #
# The following commands are needed so other projects can find the library
# in a local build directory. They use CMake's User Package Registry
# (located in ~/.cmake/registry) to do so.
# See: https://cmake.org/cmake/help/latest/manual/cmake-packages.7.html
# They don't affect the installation.

# CMAKE_EXPORT_PACKAGE_REGISTRY is OFF by default.
# It may be ON if the project is included by the SMS++ umbrella project.
if (CMAKE_EXPORT_PACKAGE_REGISTRY)

    # Generate the target import file, that allows other
    # CMake projects to import the target.
    export(EXPORT ${modTargets}
           FILE ${CMAKE_CURRENT_BINARY_DIR}/${modTargets}.cmake
           NAMESPACE ${modNamespace}::)

    # Export the find modules for the local configuration file to use them.
    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindEigen3.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindEigen3.cmake
                   COPYONLY)

    configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDFCxx.cmake
                   ${CMAKE_CURRENT_BINARY_DIR}/FindnetCDFCxx.cmake
                   COPYONLY)

    # TODO: This should be a temporary fix
    if (${CMAKE_SYSTEM} MATCHES "Darwin-20.1.0")
        configure_file(${CMAKE_CURRENT_LIST_DIR}/cmake/FindnetCDF.cmake
                       ${CMAKE_CURRENT_BINARY_DIR}/FindnetCDF.cmake
                       COPYONLY)
    endif ()

    # Add the binary directory to RPATH, so executables that use
    # dynamic loading will look into it.
    get_directory_property(hasParent PARENT_DIRECTORY)
    if (hasParent)
        set(SMSPP_BUILD_RPATH "${SMSPP_BUILD_RPATH};${CMAKE_CURRENT_BINARY_DIR}"
            PARENT_SCOPE)
    endif ()

    # Export the package in user's package registry
    export(PACKAGE ${modName})
endif ()

# ----- Subdirectories ------------------------------------------------------ #
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

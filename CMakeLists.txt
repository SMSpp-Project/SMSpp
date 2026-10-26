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
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.21)
cmake_policy(VERSION 3.15)
# Derive the version from the most recent git tag (fallback for tarballs), so
# a release is just `git tag x.y.z`. Must be done before project().
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/DeriveVersion.cmake)
smspp_derive_version(SMSpp_VERSION)

project(SMS++
        VERSION ${SMSpp_VERSION}
        DESCRIPTION "SMS++ core library"
        HOMEPAGE_URL https://gitlab.com/smspp/smspp
        LANGUAGES C CXX)

# These variables make the code harder to read but easier to change.
set(modName ${PROJECT_NAME})
set(modNamespace "SMS++")

# A C-safe form of modName for the preprocessor macros in <modName>Config.h:
# modName ("SMS++") contains '+', which is not valid in an identifier. We use
# "SMSpp", matching the SMSpp_di_unipi_it namespace. Modules generated from the
# ModuleTemplate whose name is already a valid identifier use modName as-is.
set(modMacro "SMSpp")

# Find out if it's being called by the umbrella.
get_directory_property(hasParent PARENT_DIRECTORY)

# This adds the cmake directory to the module search paths,
# allowing us to use our modules.
list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# ----- Settings ------------------------------------------------------------ #
# This creates an ENABLE_TESTING option (default: ON) and enables the testing
# using the ctest executable.
# See: https://cmake.org/cmake/help/latest/manual/ctest.1.html
include(CTest)

# Sets the default build type (if none was specified).
# See: https://cmake.org/cmake/help/latest/variable/CMAKE_BUILD_TYPE.html
include(BuildType)

# ----- Requirements -------------------------------------------------------- #
# The Boost find module is provided by CMake.
find_package(Boost 1.72.0 REQUIRED)

# Try first with library's own configuration file, then with our find module.
find_package(Eigen3 NO_MODULE)
if (NOT Eigen3_FOUND)
    find_package(Eigen3 REQUIRED)
endif ()

# Try first with library's own configuration file, then with our find module.
if (UNIX)
    find_package(netCDFCxx QUIET NO_MODULE)
endif ()
if (NOT netCDFCxx_FOUND)
    find_package(netCDFCxx REQUIRED)
endif ()

# HDF5, which netCDF reads its files with, only to turn off the printing of
# its errors in the threads that read an image in memory [see NcMemFile]:
# without it the reading is the same, with the errors in the log
find_package(HDF5 COMPONENTS C QUIET)
set(SMSpp_HDF5 ${HDF5_FOUND})
# a static library carries it to whoever links it [see SMS++Config.cmake.in]
if (SMSpp_HDF5 AND NOT BUILD_SHARED_LIBS)
    set(SMSpp_HDF5_DEPENDENCY ON)
else ()
    set(SMSpp_HDF5_DEPENDENCY OFF)
endif ()

if (MSVC)
    find_library(GETOPT_LIBRARY getopt)
endif ()

# Fetch FastFlow, which the build interface of the library passes on to the
# modules [see cmake/FastFlow.cmake]
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/FastFlow.cmake)
smspp_fetch_fastflow()

# ----- Configuration header ------------------------------------------------ #
# This will generate a *Config.h header in the build directory.
configure_file(cmake/${modName}Config.h.in ${modName}Config.h)

# ----- RPATH --------------------------------------------------------------- #
# The installed libraries and executables look for the libraries of the same
# installation relative to their own location, so that the installed tree can
# be moved as a whole; the directories of the external libraries are kept.
include(GNUInstallDirs)
if (NOT CMAKE_INSTALL_RPATH)
    if (APPLE)
        set(rpathOrigin "@loader_path")
    else ()
        set(rpathOrigin "$ORIGIN")
    endif ()
    file(RELATIVE_PATH binToLib ${CMAKE_INSTALL_FULL_BINDIR}
            ${CMAKE_INSTALL_FULL_LIBDIR})
    set(CMAKE_INSTALL_RPATH "${rpathOrigin}" "${rpathOrigin}/${binToLib}")
endif ()
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# ----- Library ------------------------------------------------------------- #
# With the BUILD_SHARED_LIBS variable we can specify if the library will
# be STATIC or SHARED, so no reason to do it now.
add_library(${modName})
target_compile_features(${modName} PUBLIC cxx_std_20)

# The name of the shared library carries the version of its binary interface,
# which any minor release may change while the major version is 0, and then
# any major release.
if (PROJECT_VERSION_MAJOR EQUAL 0)
    set(soVersion ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
else ()
    set(soVersion ${PROJECT_VERSION_MAJOR})
endif ()
set_target_properties(${modName} PROPERTIES
        VERSION ${PROJECT_VERSION}
        SOVERSION ${soVersion})

# When adding source files with target_sources(), PRIVATE means that the files
# should only be added to this library, whereas PUBLIC means they should be
# added to this library and to any target that links to it.
# INTERFACE can be used for sources that should not be added to this library
# but should be added to anything that links to it.
# Note: do not GLOB files here.
target_sources(${modName} PRIVATE
        src/AbstractBlock.cpp
        src/AbstractChange.cpp
        src/Change.cpp
        src/BoxSolver.cpp
        src/CDASolver.cpp
        src/Block.cpp
        src/BlockSolverConfig.cpp
        src/C05Function.cpp
        src/C05SumFunction.cpp
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
        src/MasterProblemBlock.cpp
        src/Objective.cpp
        src/OneVarConstraint.cpp
        src/PolyhedralFunction.cpp
        src/RBlockConfig.cpp
        src/RowConstraintSolution.cpp
        src/PolyhedralFunctionBlock.cpp
        src/QuadFunction.cpp
        src/SMSnetCDF.cpp
        src/Solution.cpp
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
        # the build directory holds the generated ${modName}Config.h, so that
        # dependents built in-tree can #include it (installed builds get it from
        # the INSTALL_INTERFACE include dir, where it is installed)
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
        $<INSTALL_INTERFACE:include/${modNamespace}>
        $<BUILD_INTERFACE:${fastflow_SOURCE_DIR}>)

# When linking other targets to the library with target_link_libraries(),
# PRIVATE means that the libraries will be linked only to this library,
# PUBLIC means they will be linked also to the targets that depend on this
# library, INTERFACE means they will be linked only to the targets that depend
# on this library.
target_link_libraries(${modName} PUBLIC
        Boost::boost
        Eigen3::Eigen
        netCDF::netCDFCxx)

if (MSVC)
    target_link_libraries(${modName} PUBLIC ${GETOPT_LIBRARY})
endif ()

if (SMSpp_HDF5)
    target_compile_definitions(${modName} PRIVATE SMSpp_HAVE_HDF5)
    target_include_directories(${modName} PRIVATE ${HDF5_C_INCLUDE_DIRS}
                               ${HDF5_INCLUDE_DIRS})
    target_link_libraries(${modName} PRIVATE ${HDF5_C_LIBRARIES})
endif ()

# This alias is defined so that executables in this same project can use
# the library with this notation.
add_library(${modNamespace}::${modName} ALIAS ${modName})

# ----- C++20 module -------------------------------------------------------- #
# The module smspp (src/SMSpp.cppm) exports the names of all the headers, so
# that a translation unit can write "import smspp;" in place of including
# them. It is built only on request, since it needs CMake 3.28 or later, the
# Ninja or Visual Studio generator and a compiler that scans the module
# dependencies and accepts the headers of Eigen and Boost in a module (Clang
# 16 or MSVC 19.34 or later); where these are missing, the option is turned
# off with a warning [see CheckModule.cmake].
option(SMSpp_BUILD_MODULE "Build the C++20 module smspp" OFF)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/CheckModule.cmake)
smspp_check_module()

if (SMSpp_BUILD_MODULE)
    # the scan of the module dependencies, which cmake_policy(VERSION 3.15)
    # above would leave off
    cmake_policy(SET CMP0155 NEW)

    add_library(${modName}_module)
    target_sources(${modName}_module PUBLIC
            FILE_SET CXX_MODULES
            BASE_DIRS ${CMAKE_CURRENT_SOURCE_DIR}/src
            FILES src/SMSpp.cppm)
    target_compile_features(${modName}_module PUBLIC cxx_std_20)
    target_link_libraries(${modName}_module PUBLIC ${modName})
    add_library(${modNamespace}::${modName}_module ALIAS ${modName}_module)
endif ()

# ----- Subdirectories ------------------------------------------------------ #
if (BUILD_TESTING)
    add_subdirectory(test)
endif ()

# ----- Install instructions ------------------------------------------------ #
# The following commands are used when installing the library
# and its CMake configuration files on the system.
# They are not required for local builds (see below).

include(GNUInstallDirs)

# Install the library
install(TARGETS ${modName}
        EXPORT ${modName}Targets
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR} # .dll
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} # .so .dylib
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}) # .lib

# Install the module, whose interface is compiled again by each importer
if (SMSpp_BUILD_MODULE)
    install(TARGETS ${modName}_module
            EXPORT ${modName}Targets
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
            LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
            ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
            FILE_SET CXX_MODULES
            DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace}/modules)
endif ()

# Install the headers
install(DIRECTORY include/
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})

# Install the auto-generated configuration header (see above).
install(FILES ${PROJECT_BINARY_DIR}/${modName}Config.h
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/${modNamespace})

# (Generate and) install the target import file, that allows other
# CMake projects to import the target.
install(EXPORT ${modName}Targets
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
file(GLOB findMods ${CMAKE_CURRENT_LIST_DIR}/cmake/Find*)
install(FILES ${findMods}
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Install the check of the C++20 modules and the fetch of FastFlow, which the
# configuration file includes for the modules that find the core with
# find_package().
install(FILES ${CMAKE_CURRENT_LIST_DIR}/cmake/CheckModule.cmake
        ${CMAKE_CURRENT_LIST_DIR}/cmake/FastFlow.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/${modName})

# Install the README and LICENSE files.
install(FILES
        ${CMAKE_CURRENT_LIST_DIR}/README.md
        ${CMAKE_CURRENT_LIST_DIR}/LICENSE
        DESTINATION ${CMAKE_INSTALL_DATADIR}/${modName})

# ----- Add the build tree to RPATH------------------------------------------ #
# Add the binary directory to RPATH, so executables that use
# dynamic loading will look into it.
if (hasParent)
    set(SMSPP_BUILD_RPATH
            "${SMSPP_BUILD_RPATH};${CMAKE_CURRENT_BINARY_DIR}"
            PARENT_SCOPE)
endif ()

# ----- Packaging support --------------------------------------------------- #
# The following commands allow one to build a proper package using CPack.
# See: https://cmake.org/cmake/help/latest/manual/cpack.1.html

# Using this with the umbrella project is too messy.
if (NOT hasParent)
    set(CPACK_PACKAGE_VENDOR "SMS++ Team")
    set(CPACK_PACKAGE_CONTACT "Donato Meoli")
    set(CPACK_PACKAGE_DESCRIPTION_SUMMARY ${PROJECT_DESCRIPTION})
    set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_CURRENT_LIST_DIR}/LICENSE")
    set(CPACK_RESOURCE_FILE_README "${CMAKE_CURRENT_LIST_DIR}/README.md")
    set(CPACK_SOURCE_IGNORE_FILES
            \\\\.git/
            \\\\.gitignore$
            \\\\.gitattributes$)
    include(CPack)
endif ()

# --------------------------------------------------------------------------- #
# Remove from the search paths the cmake directory we added at the beginning.
list(REMOVE_AT CMAKE_MODULE_PATH -1)

# --------------------------------------------------------------------------- #

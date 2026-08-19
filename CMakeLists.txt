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

include(FetchContent)

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

if (MSVC)
    find_library(GETOPT_LIBRARY getopt)
endif ()

# Declare and fetch FastFlow
FetchContent_Declare(
        fastflow
        GIT_REPOSITORY https://github.com/fastflow/fastflow.git
        GIT_TAG master)

FetchContent_Populate(fastflow)

# ----- FastFlow Windows/MSVC compatibility patch --------------------------- #
# FastFlow's bundled Windows port (ff/platforms/pthread_minport_windows.h) is
# incomplete under MSVC: it provides pthread_cond_wait() but not the timed
# variant pthread_cond_timedwait(), nor clock_gettime()/CLOCK_REALTIME, all of
# which the FastFlow core (ff/node.hpp, lb.hpp, gt.hpp, farm.hpp, pipeline.hpp
# and ff/utils.hpp) relies on for its blocking-mode timed waits. On Linux these
# come from <pthread.h>/<time.h>; on Windows we inject a minimal implementation
# into the fetched header at configure time. The edit is idempotent (guarded by
# the FF_SMSPP_WIN_COMPAT marker) and mirrors the configure-time header shimming
# we already do for LEMON in MCFLemonSolver/CMakeLists.txt. Note: NOMINMAX and
# /Zc:__cplusplus (set in the top-level CMakeSettings.txt) are also required to
# build FastFlow with MSVC.
if (MSVC)
    set(_ff_winport "${fastflow_SOURCE_DIR}/ff/platforms/pthread_minport_windows.h")
    if (EXISTS "${_ff_winport}")
        file(READ "${_ff_winport}" _ff_winport_src)
        if (NOT _ff_winport_src MATCHES "FF_SMSPP_WIN_COMPAT")
            # Injected right after the `typedef CONDITION_VARIABLE pthread_cond_t;`
            # line, where INLINE/RESTRICT, <time.h> (struct timespec), <errno.h>
            # (ETIMEDOUT) and <Windows.h> are already in scope. pthread_mutex_t is
            # CRITICAL_SECTION and pthread_cond_t is CONDITION_VARIABLE here.
            set(_ff_shim [=[typedef CONDITION_VARIABLE pthread_cond_t;
/* ---- FF_SMSPP_WIN_COMPAT: MSVC compatibility shim injected by SMS++ ------- */
#ifndef FF_SMSPP_WIN_COMPAT
#define FF_SMSPP_WIN_COMPAT
#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif
/* clock_gettime(): map CLOCK_REALTIME onto the Windows system clock. FILETIME
   counts 100 ns ticks since 1601-01-01; 116444736000000000 is the offset (in
   ticks) from that epoch to the Unix epoch 1970-01-01. */
INLINE int clock_gettime(int, struct timespec *tp) {
    FILETIME ft; ULARGE_INTEGER u;
    GetSystemTimePreciseAsFileTime(&ft);
    u.LowPart = ft.dwLowDateTime; u.HighPart = ft.dwHighDateTime;
    unsigned long long t = u.QuadPart - 116444736000000000ULL;
    tp->tv_sec  = (long)(t / 10000000ULL);
    tp->tv_nsec = (long)((t % 10000000ULL) * 100ULL);
    return 0;
}
/* pthread_cond_timedwait(): turn the absolute POSIX deadline into a relative
   millisecond timeout for SleepConditionVariableCS(); return ETIMEDOUT on
   expiry, as POSIX requires. */
INLINE int pthread_cond_timedwait(pthread_cond_t *cond, pthread_mutex_t *mutex,
                                  const struct timespec *abstime) {
    struct timespec now;
    clock_gettime(CLOCK_REALTIME, &now);
    long long ms = (long long)(abstime->tv_sec - now.tv_sec) * 1000LL
                 + (long long)(abstime->tv_nsec - now.tv_nsec) / 1000000LL;
    if (ms < 0) ms = 0;
    if (!SleepConditionVariableCS(cond, mutex, (DWORD)ms))
        if (GetLastError() == ERROR_TIMEOUT) return ETIMEDOUT;
    return 0;
}
#endif
/* ---- end FF_SMSPP_WIN_COMPAT --------------------------------------------- */]=])
            string(REPLACE "typedef CONDITION_VARIABLE pthread_cond_t;"
                    "${_ff_shim}" _ff_winport_src "${_ff_winport_src}")
            file(WRITE "${_ff_winport}" "${_ff_winport_src}")
            message(STATUS
                    "FastFlow: injected MSVC pthread_cond_timedwait/clock_gettime shim")
        endif ()
    endif ()
endif ()

# ----- Configuration header ------------------------------------------------ #
# This will generate a *Config.h header in the build directory.
configure_file(cmake/${modName}Config.h.in ${modName}Config.h)

# ----- Library ------------------------------------------------------------- #
# With the BUILD_SHARED_LIBS variable we can specify if the library will
# be STATIC or SHARED, so no reason to do it now.
add_library(${modName})
target_compile_features(${modName} PUBLIC cxx_std_20)
include(ClangPatch) # TODO: Remove this when it's not needed anymore

# When adding source files with target_sources(), PRIVATE means that the files
# should only be added to this library, whereas PUBLIC means they should be
# added to this library and to any target that links to it.
# INTERFACE can be used for sources that should not be added to this library
# but should be added to anything that links to it.
# Note: do not GLOB files here.
target_sources(${modName} PRIVATE
        src/AbstractBlock.cpp
        src/Change.cpp
        src/BoxSolver.cpp
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
        src/QuadFunction.cpp
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

# This alias is defined so that executables in this same project can use
# the library with this notation.
add_library(${modNamespace}::${modName} ALIAS ${modName})

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

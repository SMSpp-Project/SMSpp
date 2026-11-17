# --------------------------------------------------------------------------- #
#    CMake find module for GUROBI                                             #
#                                                                             #
#    This module finds GUROBI include directories and libraries.              #
#    Use it by invoking find_package() with the form:                         #
#                                                                             #
#        find_package(GUROBI [version] [EXACT] [REQUIRED])                    #
#                                                                             #
#    The results are stored in the following variables:                       #
#                                                                             #
#        GUROBI_FOUND         - True if headers are found                     #
#        GUROBI_LICENSE_FOUND - True if gurobi.lic file is found              #
#        GUROBI_INCLUDE_DIRS  - Include directories                           #
#        GUROBI_LIBRARIES     - Libraries to be linked                        #
#        GUROBI_DLL           - The found runtime DLL, where there is one     #
#        GUROBI_VERSION       - Version number                                #
#                                                                             #
#    This module reads hints about search locations from variables:           #
#                                                                             #
#        GUROBI_ROOT          - Custom path to GUROBI                         #
#                                                                             #
#    The following IMPORTED target is also defined:                           #
#                                                                             #
#        GUROBI::Gurobi                                                       #
#                                                                             #
#    This find module is provided because GUROBI does not provide             #
#    a CMake configuration file on its own. It is the one of all the SMS++    #
#    modules, which find it in the directory of the CMake files of the core   #
#    [see SMSppModule.cmake].                                                 #
#                                                                             #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
include(FindPackageHandleStandardArgs)

# ----- Find the path to GUROBI --------------------------------------------- #
# This takes the greatest gurobi* found where the installers put it (/opt,
# /Library, C:), of which only those of the system at hand are there

foreach (dir /opt /Library "C:")
    file(GLOB GUROBI_DIRS "${dir}/gurobi*")
    if (NOT IS_DIRECTORY "${GUROBI_ROOT}")
        if (NOT "${GUROBI_ROOT}" STREQUAL "")
            message(STATUS "Specified Gurobi: ${GUROBI_ROOT} not found")
        endif ()
        list(SORT GUROBI_DIRS)
        list(REVERSE GUROBI_DIRS)
        if (GUROBI_DIRS)
            list(GET GUROBI_DIRS 0 GUROBI_ROOT)
            message(STATUS "Using Gurobi: ${GUROBI_ROOT}")
            break()
        else ()
            set(GUROBI_ROOT GUROBI_ROOT-NOTFOUND)
        endif ()
    else ()
        break()
    endif ()
endforeach ()

# ----- Requirements -------------------------------------------------------- #
# This sets the variable CMAKE_THREAD_LIBS_INIT, see:
# https://cmake.org/cmake/help/latest/module/FindThreads.html
find_package(Threads QUIET)

# Drop the cached paths if one of them does not exist any more, so that
# they are all searched again from the same place
set(_gurobi_cached GUROBI_INCLUDE_DIR GUROBI_LIBRARY GUROBI_DLL GUROBI_VERSION)
foreach (_var ${_gurobi_cached})
    foreach (_path ${${_var}})
        if (IS_ABSOLUTE "${_path}" AND NOT EXISTS "${_path}")
            set(_gurobi_stale TRUE)
        endif ()
    endforeach ()
endforeach ()
if (_gurobi_stale)
    message(STATUS "Cached GUROBI paths not found, searching again")
    foreach (_var ${_gurobi_cached})
        unset(${_var} CACHE)
    endforeach ()
endif ()
unset(_gurobi_cached)
unset(_gurobi_stale)
unset(_var)
unset(_path)

# The cached version is the one of the include directory it was read from
if (NOT "${GUROBI_INCLUDE_DIR}" STREQUAL
        "${_GUROBI_VERSION_INCLUDE_DIR}")
    unset(GUROBI_VERSION CACHE)
endif ()

# Check if already in cache
if (GUROBI_INCLUDE_DIR AND GUROBI_LIBRARY AND GUROBI_VERSION)
    set(GUROBI_FOUND TRUE)
endif ()

if (NOT GUROBI_FOUND)

    # ----- Find the GUROBI include directory ------------------------------- #
    # The directory of the platform is the one of those GUROBI is built for
    # that is there, and the library is taken from it, hence so is the
    # include directory
    unset(GUROBI_INCLUDE_DIR CACHE)
    find_path(GUROBI_INCLUDE_DIR
            NAMES gurobi_c.h
            PATHS ${GUROBI_ROOT}
            PATH_SUFFIXES linux64/include armlinux64/include
                          macos_universal2/include win64/include
            NO_DEFAULT_PATH
            DOC "GUROBI include directory.")
    get_filename_component(GUROBI_DIR "${GUROBI_INCLUDE_DIR}" DIRECTORY)

    # ----- Parse the version ----------------------------------------------- #
    if (GUROBI_INCLUDE_DIR)
        file(STRINGS "${GUROBI_INCLUDE_DIR}/gurobi_c.h"
                _gurobi_version_lines
                REGEX "#define GRB_VERSION_(MAJOR|MINOR|TECHNICAL)")
        foreach (_part MAJOR MINOR TECHNICAL)
            string(REGEX REPLACE ".*GRB_VERSION_${_part} *\([0-9]*\).*"
                    "\\1" _gurobi_${_part} "${_gurobi_version_lines}")
        endforeach ()
        set(GUROBI_VERSION
                "${_gurobi_MAJOR}.${_gurobi_MINOR}.${_gurobi_TECHNICAL}")
        set(_gurobi_name gurobi${_gurobi_MAJOR}${_gurobi_MINOR})
        unset(_gurobi_version_lines)
        unset(_gurobi_MAJOR)
        unset(_gurobi_MINOR)
        unset(_gurobi_TECHNICAL)
        unset(_part)
    endif ()

    # ----- Find the GUROBI library ----------------------------------------- #
    # The name carries the major and minor version, to which CMake adds the
    # prefix and the suffix of the platform; where the library is a DLL, the
    # one found in lib/ is its import library, and the DLL is in bin/
    if (_gurobi_name)
        find_library(GUROBI_LIBRARY
                NAMES ${_gurobi_name}
                PATHS ${GUROBI_DIR}/lib
                NO_DEFAULT_PATH
                DOC "GUROBI library.")
        set(_gurobi_dll ${CMAKE_SHARED_LIBRARY_PREFIX}${_gurobi_name})
        string(APPEND _gurobi_dll ${CMAKE_SHARED_LIBRARY_SUFFIX})
        find_file(GUROBI_DLL
                NAMES ${_gurobi_dll}
                PATHS ${GUROBI_DIR}/bin
                NO_DEFAULT_PATH
                DOC "GUROBI runtime DLL.")
        unset(_gurobi_dll)
        unset(_gurobi_name)
    endif ()

    # ----- Handle the standard arguments ----------------------------------- #
    # The following macro manages the QUIET, REQUIRED and version-related
    # options passed to find_package(). It also sets <PackageName>_FOUND if
    # REQUIRED_VARS are set.
    # REQUIRED_VARS should be cache entries and not output variables. See:
    # https://cmake.org/cmake/help/latest/module/
    # FindPackageHandleStandardArgs.html
    find_package_handle_standard_args(
            GUROBI
            REQUIRED_VARS GUROBI_LIBRARY GUROBI_INCLUDE_DIR
            VERSION_VAR GUROBI_VERSION)

    # ----- Find the GUROBI license ----------------------------------------- #
    set(GUROBI_LICENSE_FOUND FALSE)

    set(LICENSE_PATHS ${GUROBI_ROOT} "$ENV{HOME}" "$ENV{USERPROFILE}")
    if (DEFINED ENV{GRB_LICENSE_FILE})
        get_filename_component(_gurobi_lic "$ENV{GRB_LICENSE_FILE}" DIRECTORY)
        list(PREPEND LICENSE_PATHS "${_gurobi_lic}")
        unset(_gurobi_lic)
    endif ()

    foreach (path ${LICENSE_PATHS})
        if (EXISTS "${path}/gurobi.lic")
            set(GUROBI_LICENSE_FOUND TRUE)
            message(STATUS "Gurobi license file found at: ${path}/gurobi.lic")
            break()
        endif ()
    endforeach ()

    if (NOT GUROBI_LICENSE_FOUND)
        message(WARNING "Gurobi license file not found in default \
locations. Define the GRB_LICENSE_FILE environment variable specifying your \
custom location of the `gurobi.lic` file, or move it to one of the default \
locations to definitively suppress this warning, i.e.: ${LICENSE_PATHS}")
    endif ()
endif ()

# A found package keeps its version in the cache, with the include
# directory it was read from
if (GUROBI_FOUND)
    set(GUROBI_VERSION "${GUROBI_VERSION}" CACHE INTERNAL
            "GUROBI version.")
    set(_GUROBI_VERSION_INCLUDE_DIR "${GUROBI_INCLUDE_DIR}"
            CACHE INTERNAL "Include directory of the GUROBI version.")
endif ()

# ----- Export the target --------------------------------------------------- #
# A DLL is imported with its import library, any other library as it is
if (GUROBI_FOUND)
    set(GUROBI_INCLUDE_DIRS ${GUROBI_INCLUDE_DIR})
    set(GUROBI_LIBRARIES ${CMAKE_THREAD_LIBS_INIT} ${CMAKE_DL_LIBS})

    if (NOT TARGET GUROBI::Gurobi)
        if (GUROBI_DLL)
            add_library(GUROBI::Gurobi SHARED IMPORTED)
            set_target_properties(
                    GUROBI::Gurobi PROPERTIES
                    IMPORTED_IMPLIB "${GUROBI_LIBRARY}"
                    IMPORTED_LOCATION "${GUROBI_DLL}")
        else ()
            add_library(GUROBI::Gurobi UNKNOWN IMPORTED)
            set_target_properties(
                    GUROBI::Gurobi PROPERTIES
                    IMPORTED_LOCATION "${GUROBI_LIBRARY}")
        endif ()
        set_target_properties(
                GUROBI::Gurobi PROPERTIES
                INTERFACE_INCLUDE_DIRECTORIES "${GUROBI_INCLUDE_DIRS}"
                INTERFACE_LINK_LIBRARIES "${GUROBI_LIBRARIES}")
    endif ()
endif ()

# Variables marked as advanced are not displayed in CMake GUIs, see:
# https://cmake.org/cmake/help/latest/command/mark_as_advanced.html
mark_as_advanced(GUROBI_INCLUDE_DIR
        GUROBI_LIBRARY
        GUROBI_DLL
        GUROBI_VERSION)

# --------------------------------------------------------------------------- #

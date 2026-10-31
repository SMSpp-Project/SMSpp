# --------------------------------------------------------------------------- #
# FastFlow.cmake                                                              #
#                                                                             #
# Fetches FastFlow (the header-only library of parallel patterns that some    #
# sources of the modules use) into the build tree, as smspp_fetch_fastflow(), #
# after which fastflow_SOURCE_DIR is the directory to add to the include      #
# paths. It is fetched once, even if more projects of the same build ask for  #
# it; FETCHCONTENT_SOURCE_DIR_FASTFLOW gives a source tree in place of the    #
# fetch, as a release tarball does [see the umbrella].                        #
#                                                                             #
# The core fetches it and passes it on through the build interface of its     #
# target; an installed core does not carry it, so a module built alone with   #
# find_package() fetches it with this, which the package configuration file   #
# of the core includes:                                                       #
#     smspp_fetch_fastflow()                                                  #
#     target_include_directories(<target> PRIVATE ${fastflow_SOURCE_DIR})     #
# --------------------------------------------------------------------------- #

include(FetchContent)

# the directory of this file, where the patches of FastFlow are
set(SMSpp_FASTFLOW_PATCHES "${CMAKE_CURRENT_LIST_DIR}/fastflow")

macro(smspp_fetch_fastflow)
    FetchContent_Declare(
            fastflow
            GIT_REPOSITORY https://github.com/fastflow/fastflow.git
            GIT_TAG master)

    FetchContent_GetProperties(fastflow)
    if (NOT fastflow_POPULATED)
        FetchContent_Populate(fastflow)
    endif ()

    # the patches in cmake/fastflow, each the change proposed to FastFlow
    # by a pull request of its own, which are applied to the fetched sources
    # until FastFlow has them: msvc-timed-wait gives the port of the pthreads
    # for MSVC the timed waits of the blocking mode, parallel-for-self-
    # contained lets parallel_for.hpp be included alone by MSVC, and
    # mingw-winpthreads builds FastFlow with the compilers of MinGW-w64
    # (MSYS2) on their POSIX threads. A patch already there, as in a FastFlow
    # that has merged it or in sources patched by a previous configuration,
    # is left alone (git apply --reverse --check succeeds on it); one that
    # applies neither way stops the configuration. NOMINMAX and
    # /Zc:__cplusplus (set in the CMakeSettings.txt of the umbrella) are
    # also needed to build FastFlow with MSVC.
    find_package(Git REQUIRED QUIET)
    file(GLOB _ff_patches "${SMSpp_FASTFLOW_PATCHES}/*.patch")
    # git looks for a repository no higher than the sources of FastFlow, so
    # that the paths of the patches are taken from there even when they lie
    # in the tree of another repository with no .git of their own
    get_filename_component(_ff_up "${fastflow_SOURCE_DIR}" DIRECTORY)
    set(_ff_git "${CMAKE_COMMAND}" -E env "GIT_CEILING_DIRECTORIES=${_ff_up}"
            "${GIT_EXECUTABLE}")
    foreach (_ff_patch IN LISTS _ff_patches)
        get_filename_component(_ff_name "${_ff_patch}" NAME_WE)
        execute_process(
                COMMAND ${_ff_git} apply --reverse --check
                        "${_ff_patch}"
                WORKING_DIRECTORY "${fastflow_SOURCE_DIR}"
                RESULT_VARIABLE _ff_there OUTPUT_QUIET ERROR_QUIET)
        if (NOT _ff_there EQUAL 0)
            execute_process(
                    COMMAND ${_ff_git} apply "${_ff_patch}"
                    WORKING_DIRECTORY "${fastflow_SOURCE_DIR}"
                    RESULT_VARIABLE _ff_applied ERROR_VARIABLE _ff_error)
            if (NOT _ff_applied EQUAL 0)
                message(FATAL_ERROR "FastFlow: the patch ${_ff_name} does "
                        "not apply: ${_ff_error}")
            endif ()
            message(STATUS "FastFlow: applied ${_ff_name}")
        endif ()
    endforeach ()
endmacro()

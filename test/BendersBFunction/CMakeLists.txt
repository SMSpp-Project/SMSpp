cmake_minimum_required(VERSION 3.11...3.15)
# TODO: set proper version
project(SMS++_test_sys VERSION 1.0.0 LANGUAGES CXX)

# ----- Settings ------------------------------------------------------------ #
include(CTest)

# Since we are using the block factory, objects from statically linked libraries
# may wrongly appear unused, and by default the linker does not include them,
# so we have to force the linking.

if (NOT BUILD_SHARED_LIBS)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang")
        add_link_options("-Wl,-all_load")
    else()
        add_link_options("-Wl,--whole-archive,--allow-multiple-definition")
    endif ()
endif ()

# ----- Requirements -------------------------------------------------------- #
find_package(MILPSolver REQUIRED)
find_package(BundleSolver REQUIRED)

# ----- Fetch GoogleTest ---------------------------------------------------- #
include(FetchContent)
fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.8.1
)

fetchcontent_getproperties(googletest)
if (NOT googletest_POPULATED)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_MACOSX_RPATH 1)
    endif ()
    fetchcontent_populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

# ----- BendersBFunction_test ----------------------------------------------- #
add_executable(BendersBFunction_test tests_BendersBFunction.cpp
               cwl-mcf/cwl-mcf.cpp cwl-mcf/MCFSimplex.C)
target_link_libraries(BendersBFunction_test PRIVATE SMS++::SMSpp
                      SMS++::MILPSolver
                      SMS++::BundleSolver)

add_test(
        NAME BendersBFunction_test
        COMMAND BendersBFunction_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

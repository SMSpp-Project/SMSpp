# ----- Requirements -------------------------------------------------------- #
find_package(MILPSolver REQUIRED)

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

# ----- Block_test ---------------------------------------------------------- #
add_executable(AbstractBlock_test tests_AbstractBlock.cpp)
target_link_libraries(AbstractBlock_test PRIVATE SMS++::SMSpp gtest)

add_test(
        NAME AbstractBlock_test
        COMMAND AbstractBlock_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# ----- AbstractPath_test ----------------------------------------------- #
add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMSpp)

add_test(
        NAME AbstractPath_test
        COMMAND AbstractPath_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# --------------------------------------------------------------------------- #
#    CMake file for unit tests                                                #
# --------------------------------------------------------------------------- #

# ----- AbstractPath_test --------------------------------------------------- #
add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMSpp)

#add_test(NAME AbstractPath_test
#         COMMAND AbstractPath_test)

# ----- ClassFactory_test --------------------------------------------------- #
add_executable(ClassFactory_test ClassFactory/main.cpp)
target_link_libraries(ClassFactory_test PRIVATE SMS++::SMSpp)

#add_test(NAME ClassFactory_test
#         COMMAND ClassFactory_test)

# ----- GoogleTest unit tests ----------------------------------------------- #

# Fetch GoogleTest
include(FetchContent)
fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG release-1.10.0)

fetchcontent_getproperties(googletest)
if (NOT googletest_POPULATED)
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_MACOSX_RPATH 1)
    endif ()
    fetchcontent_populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

# Declare all tests
set(CLASSES
    "AbstractBlock"
    "ColVariable"
    "Function"
    "LinearFunction")

foreach(CLASS ${CLASSES})
    add_executable(${CLASS}_test tests_${CLASS}.cpp)
    target_link_libraries(${CLASS}_test PRIVATE SMS++::SMSpp gtest)

    add_test(NAME ${CLASS}_test COMMAND ${CLASS}_test)
endforeach()

# --------------------------------------------------------------------------- #
#    CMake file for SMS++/test                                                #
#                                                                             #
#    In this file we define the unit tests for this library.                  #
#    Tests that should be supported by ctest executable must be added         #
#    with the add_test() command.                                             #
#                                                                             #
#    Note: this file tries to respect the principles of Modern CMake,         #
#          keep it in mind when editing it.                                   #
#                                                                             #
#                              Niccolo' Iardella                              #
#                                Donato Meoli                                 #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #

# ----- Standard unit tests ------------------------------------------------- #
add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMS++)
add_test(NAME AbstractPath_test
         COMMAND AbstractPath_test)

add_executable(ClassFactory_test ClassFactory/main.cpp)
target_link_libraries(ClassFactory_test PRIVATE SMS++::SMS++)
add_test(NAME ClassFactory_test
         COMMAND ClassFactory_test)

# ----- GoogleTest unit tests ----------------------------------------------- #
# The following tests are built using the GoogleTest framework,
# see: https://github.com/google/googletest
# GoogleTest is automatically fetched and built using CMake's FetchContent,
# see: https://cmake.org/cmake/help/latest/module/FetchContent.html
include(FetchContent)

# Declare GoogleTest repository
fetchcontent_declare(
        googletest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main)

# Fetch GoogleTest, if not already fetched
fetchcontent_getproperties(googletest)
if (NOT googletest_POPULATED)
    # This is so GoogleTest doesn't get installed alongside the library.
    set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)

    # See: https://cmake.org/cmake/help/latest/prop_tgt/MACOSX_RPATH.html
    if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
        set(CMAKE_MACOSX_RPATH 1)
    endif ()

    fetchcontent_populate(googletest)
    add_subdirectory(${googletest_SOURCE_DIR} ${googletest_BINARY_DIR})
endif ()

# Declare all GoogleTest unit tests at once
set(CLASSES
    "AbstractBlock"
    "ColVariable"
    "Function"
    "LinearFunction")

foreach (CLASS ${CLASSES})
    add_executable(${CLASS}_test tests_${CLASS}.cpp)
    target_link_libraries(${CLASS}_test PRIVATE SMS++::SMS++ gtest)

    add_test(NAME ${CLASS}_test COMMAND ${CLASS}_test)
endforeach ()

# --------------------------------------------------------------------------- #

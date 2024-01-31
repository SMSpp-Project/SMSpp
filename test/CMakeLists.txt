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

# Since we are using the block factory, objects from linked libraries
# may wrongly appear unused, and by default the linker does not include them,
# so we have to force the linking.
if (BUILD_SHARED_LIBS)
    if (UNIX AND (NOT APPLE))
        add_link_options("-Wl,--no-as-needed")
    endif ()
else ()
    if (MSVC)
        add_link_options("/WHOLEARCHIVE")
        add_link_options("/FORCE:MULTIPLE")
    else () # Unix
        if (APPLE)
            add_link_options("-Wl,-all_load")
        else ()
            add_link_options("-Wl,--whole-archive,--allow-multiple-definition")
        endif ()
    endif ()
endif ()

# ----- Standard unit tests ------------------------------------------------- #
add_executable(ClassFactory_test ClassFactory/main.cpp)
target_link_libraries(ClassFactory_test PRIVATE SMS++::SMS++)
add_test(NAME ClassFactory_test
         COMMAND ClassFactory_test)

add_executable(AbstractBlock_test tests_AbstractBlock.cpp)
target_link_libraries(AbstractBlock_test PRIVATE SMS++::SMS++)
add_test(NAME AbstractBlock_test
         COMMAND AbstractBlock_test)

add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMS++)
add_test(NAME AbstractPath_test
         COMMAND AbstractPath_test)

add_executable(ColVariable_test tests_ColVariable.cpp)
target_link_libraries(ColVariable_test PRIVATE SMS++::SMS++)
add_test(NAME ColVariable_test
         COMMAND ColVariable_test)

add_executable(Function_test tests_Function.cpp)
target_link_libraries(Function_test PRIVATE SMS++::SMS++)
add_test(NAME Function_test
         COMMAND Function_test)

add_executable(LinearFunction_test tests_LinearFunction.cpp)
target_link_libraries(LinearFunction_test PRIVATE SMS++::SMS++)
add_test(NAME LinearFunction_test
         COMMAND LinearFunction_test)

# --------------------------------------------------------------------------- #

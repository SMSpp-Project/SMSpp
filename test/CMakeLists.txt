# --------------------------------------------------------------------------- #
#    CMake file for SMS++/test                                                #
#                                                                             #
#    In this file we define the unit tests for this library.                  #
#    Tests that should be supported by ctest executable must be added         #
#    with the add_test() command.                                             #
#                                                                             #
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
        add_link_options("/WHOLEARCHIVE" "/FORCE:MULTIPLE")
    else () # Unix
        if (APPLE)
            add_link_options("-Wl,-all_load")
        else ()
            add_link_options("-Wl,--whole-archive,--allow-multiple-definition")
        endif ()
    endif ()
endif ()

# Utility function to standardize installation and runtime dependency handling
# for SMS++ tests.
function(smspp_install target_name)
    if (WIN32 AND BUILD_SHARED_LIBS AND NOT DEFINED ENV{CONDA_BUILD})
        # On Windows with shared libraries enabled copies all runtime DLL
        # dependencies next to the executable in the build directory
        # (POST_BUILD), using CMake's TARGET_RUNTIME_DLLS generator expression
        add_custom_command(TARGET ${target_name} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                $<TARGET_RUNTIME_DLLS:${target_name}>
                $<TARGET_FILE_DIR:${target_name}>
                COMMAND_EXPAND_LISTS
                COMMENT "Copying runtime DLLs next to ${target_name}.exe")
    endif ()

    #[[install(TARGETS ${target_name}
            RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})]]
endfunction()

# ----- Standard unit tests ------------------------------------------------- #
add_executable(ClassFactory_test ClassFactory/main.cpp)
target_link_libraries(ClassFactory_test PRIVATE SMS++::SMS++)
smspp_install(ClassFactory_test)
add_test(NAME ClassFactory_test
         COMMAND ClassFactory_test)

add_executable(AbstractBlock_test tests_AbstractBlock.cpp)
target_link_libraries(AbstractBlock_test PRIVATE SMS++::SMS++)
smspp_install(AbstractBlock_test)
add_test(NAME AbstractBlock_test
         COMMAND AbstractBlock_test)

add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMS++)
smspp_install(AbstractPath_test)
add_test(NAME AbstractPath_test
         COMMAND AbstractPath_test)

add_executable(ColVariable_test tests_ColVariable.cpp)
target_link_libraries(ColVariable_test PRIVATE SMS++::SMS++)
smspp_install(ColVariable_test)
add_test(NAME ColVariable_test
         COMMAND ColVariable_test)

add_executable(Function_test tests_Function.cpp)
target_link_libraries(Function_test PRIVATE SMS++::SMS++)
smspp_install(Function_test)
add_test(NAME Function_test
         COMMAND Function_test)

add_executable(LinearFunction_test tests_LinearFunction.cpp)
target_link_libraries(LinearFunction_test PRIVATE SMS++::SMS++)
smspp_install(LinearFunction_test)
add_test(NAME LinearFunction_test
         COMMAND LinearFunction_test)

add_executable(QuadFunction_test tests_QuadFunction.cpp)
target_link_libraries(QuadFunction_test PRIVATE SMS++::SMS++)
smspp_install(QuadFunction_test)
add_test(NAME QuadFunction_test
         COMMAND QuadFunction_test)

# --------------------------------------------------------------------------- #

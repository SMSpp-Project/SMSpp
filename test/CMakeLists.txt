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
    if (UNIX)
        if (APPLE)
            add_link_options("-Wl,-dead_strip_dylibs")
        else ()
            add_link_options("-Wl,--no-as-needed")
        endif ()
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

# ----- Standard unit tests ------------------------------------------------- #
add_executable(ClassFactory_test ClassFactory/main.cpp)
target_link_libraries(ClassFactory_test PRIVATE SMS++::SMS++)
add_test(NAME ClassFactory_test
         COMMAND ClassFactory_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET ClassFactory_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ClassFactory_test>
            $<TARGET_FILE_DIR:ClassFactory_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to ClassFactory_test.exe"
    )
endif()

add_executable(AbstractBlock_test tests_AbstractBlock.cpp)
target_link_libraries(AbstractBlock_test PRIVATE SMS++::SMS++)
add_test(NAME AbstractBlock_test
         COMMAND AbstractBlock_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET AbstractBlock_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:AbstractBlock_test>
            $<TARGET_FILE_DIR:AbstractBlock_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to AbstractBlock_test.exe"
    )
endif()

add_executable(AbstractPath_test tests_AbstractPath.cpp)
target_link_libraries(AbstractPath_test PRIVATE SMS++::SMS++)
add_test(NAME AbstractPath_test
         COMMAND AbstractPath_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET AbstractPath_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:AbstractPath_test>
            $<TARGET_FILE_DIR:AbstractPath_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to AbstractPath_test.exe"
    )
endif()

add_executable(ColVariable_test tests_ColVariable.cpp)
target_link_libraries(ColVariable_test PRIVATE SMS++::SMS++)
add_test(NAME ColVariable_test
         COMMAND ColVariable_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET ColVariable_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:ColVariable_test>
            $<TARGET_FILE_DIR:ColVariable_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to ColVariable_test.exe"
    )
endif()

add_executable(Function_test tests_Function.cpp)
target_link_libraries(Function_test PRIVATE SMS++::SMS++)
add_test(NAME Function_test
         COMMAND Function_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET Function_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:Function_test>
            $<TARGET_FILE_DIR:Function_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to Function_test.exe"
    )
endif()

add_executable(LinearFunction_test tests_LinearFunction.cpp)
target_link_libraries(LinearFunction_test PRIVATE SMS++::SMS++)
add_test(NAME LinearFunction_test
         COMMAND LinearFunction_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET LinearFunction_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:LinearFunction_test>
            $<TARGET_FILE_DIR:LinearFunction_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to LinearFunction_test.exe"
    )
endif()

add_executable(QuadFunction_test tests_QuadFunction.cpp)
target_link_libraries(QuadFunction_test PRIVATE SMS++::SMS++)
add_test(NAME QuadFunction_test
         COMMAND QuadFunction_test)

if (WIN32 AND BUILD_SHARED_LIBS)
    # copy runtime DLLs next to the exe after build
    add_custom_command(TARGET QuadFunction_test POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_RUNTIME_DLLS:QuadFunction_test>
            $<TARGET_FILE_DIR:QuadFunction_test>
            COMMAND_EXPAND_LISTS
            COMMENT "Copying runtime DLLs next to QuadFunction_test.exe"
    )
endif()

# --------------------------------------------------------------------------- #

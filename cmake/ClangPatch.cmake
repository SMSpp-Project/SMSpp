# This definition is needed because the version of Clang shipped with
# macOS Big Sur has an issue with boost::any type recognition.

if (${CMAKE_CXX_COMPILER_ID} MATCHES "AppleClang" AND
    ${CMAKE_SYSTEM_NAME} MATCHES "Darwin" AND
    ${CMAKE_SYSTEM_VERSION} VERSION_GREATER_EQUAL 20.1.0)
    target_compile_definitions(${modName} PUBLIC CLANG_1200_0_32_27_PATCH)
endif ()

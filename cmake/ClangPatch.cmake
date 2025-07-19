# This definition is needed because the version of Clang ≥ 12.0.0 shipped
# with macOS ≥ 10.15 Big Sur has an issue with boost::any type recognition.

if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
    CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND
    CMAKE_SYSTEM_VERSION VERSION_GREATER_EQUAL 19.0.0 AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL 12.0.0)
    target_compile_definitions(${modName} PUBLIC CLANG_1200_0_32_27_PATCH)
endif ()

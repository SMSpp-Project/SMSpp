# This definition is needed because the version of Clang shipped with
# macOS Big Sur has an issue with boost::any type recognition.
# 12.0.0 ≤ AppleClang < 12.1.0
# CMAKE_SYSTEM_VERSION ≤ 20.2.0

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_GREATER_EQUAL "12.0.0" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS "12.1.0" AND
    CMAKE_SYSTEM_NAME STREQUAL "Darwin" AND
    CMAKE_SYSTEM_VERSION VERSION_LESS_EQUAL "20.2.0")
    target_compile_definitions(${modName} PUBLIC CLANG_1200_0_32_27_PATCH)
endif ()

# This definition is needed because the version of AppleClang ≥ 12.0.0 shipped
# with macOS ≥ 10.15 Big Sur has an issue with boost::any type recognition.

if (CMAKE_CXX_COMPILER_ID STREQUAL "AppleClang" AND
    CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_compile_definitions(${modName} PUBLIC CLANG_1200_0_32_27_PATCH)
endif ()

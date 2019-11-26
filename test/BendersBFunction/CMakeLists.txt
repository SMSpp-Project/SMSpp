# ----- Requirements -------------------------------------------------------- #
find_package(MILPSolver REQUIRED)
find_package(BundleSolver REQUIRED)

# ----- BendersBFunction_test ----------------------------------------------- #
add_executable(BendersBFunction_test tests_BendersBFunction.cpp
               cwl-mcf/cwl-mcf.cpp cwl-mcf/MCFSimplex.C)
target_link_libraries(BendersBFunction_test PRIVATE SMS++::SMSpp
                      SMS++::MILPSolver
                      SMS++::BundleSolver)

add_test(
        NAME BendersBFunction_test
        COMMAND BendersBFunction_test
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

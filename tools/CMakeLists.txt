# ----- block_solver -------------------------------------------------------- #
add_executable(block_solver block_solver.cpp)

target_link_libraries(block_solver PRIVATE
                      SMS++::SMSpp
                      SMS++::UCBlock
                      SMS++::MILPSolver)

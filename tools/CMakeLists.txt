# --------------------------------------------------------------------------- #
#    Main CMake file for SMS++ tools                                          #
#                                                                             #
#                              Niccolo' Iardella                              #
#                          Operations Research Group                          #
#                         Dipartimento di Informatica                         #
#                             Universita' di Pisa                             #
# --------------------------------------------------------------------------- #
cmake_minimum_required(VERSION 3.8...3.15)
# TODO: set proper version
project(SMS++_tools VERSION 1.0.0 LANGUAGES CXX)

# ----- Requirements -------------------------------------------------------- #
find_package(UCBlock REQUIRED)
find_package(MILPSolver REQUIRED)

# ----- block_solver -------------------------------------------------------- #
add_executable(block_solver block_solver.cpp)
target_compile_features(block_solver PUBLIC cxx_std_17)

target_link_libraries(block_solver PRIVATE
                      SMS++::UCBlock
                      SMS++::MILPSolver)

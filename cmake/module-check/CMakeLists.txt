# A module whose template walks a std::vector by its iterators, and a unit
# that imports it and instantiates the template without including <vector>,
# as one that imports a module of SMS++ does: it builds where the generator
# and the compiler build modules whose templates find the operators of the
# standard library [see CheckModule.cmake]
cmake_minimum_required(VERSION 3.28)
project(smspp_module_check CXX)
set(CMAKE_CXX_STANDARD 20)
add_library(m)
target_sources(m PUBLIC FILE_SET CXX_MODULES FILES m.cppm)
add_executable(u u.cpp)
target_link_libraries(u PRIVATE m)

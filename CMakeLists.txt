cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_VERBOSE_MAKEFILE OFF)

project(SMS++ DESCRIPTION "SMS++ main library")

set(PROJECT_INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(SMS++_SOURCES
    ${PROJECT_SOURCE_DIR}/Block.cpp
    ${PROJECT_SOURCE_DIR}/ColVariable.cpp
    ${PROJECT_SOURCE_DIR}/ColVariableSolution.cpp
    ${PROJECT_SOURCE_DIR}/Configuration.cpp
    ${PROJECT_SOURCE_DIR}/Constraint.cpp
    ${PROJECT_SOURCE_DIR}/DQuadFunction.cpp
    ${PROJECT_SOURCE_DIR}/FRealObjective.cpp
    ${PROJECT_SOURCE_DIR}/FRowConstraint.cpp
    #${PROJECT_SOURCE_DIR}/LagBFunction.cpp
    ${PROJECT_SOURCE_DIR}/LinearFunction.cpp
    ${PROJECT_SOURCE_DIR}/Objective.cpp
    ${PROJECT_SOURCE_DIR}/OneVarConstraint.cpp
    ${PROJECT_SOURCE_DIR}/Solver.cpp
    ${PROJECT_SOURCE_DIR}/ThinComputeInterface.cpp
    ${PROJECT_SOURCE_DIR}/Variable.cpp
    )

find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS})

find_package(Eigen3 REQUIRED)
include_directories(${EIGEN3_INCLUDE_DIRS})

include_directories("${PROJECT_INCLUDE_DIR}")
add_library(${PROJECT_NAME} STATIC ${SMS++_SOURCES})

#add_subdirectory(test)

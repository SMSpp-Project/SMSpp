cmake_minimum_required(VERSION 3.1)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_COMPILER "/usr/bin/clang++")
set(CMAKE_VERBOSE_MAKEFILE OFF)

# ----- Project description and directories ------------------------------------
project(SMS++ DESCRIPTION "SMS++ main library")

set(PROJECT_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PROJECT_SOURCE_DIR  ${CMAKE_CURRENT_SOURCE_DIR}/src)

# ----- Requirements -----------------------------------------------------------
find_package(Boost REQUIRED)
find_package(Eigen3 REQUIRED)

# ----- Library ----------------------------------------------------------------
set(${PROJECT_NAME}_SOURCES
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

add_library(${PROJECT_NAME} STATIC ${${PROJECT_NAME}_SOURCES})
target_include_directories(${PROJECT_NAME} PUBLIC ${Boost_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${EIGEN3_INCLUDE_DIRS})
target_include_directories(${PROJECT_NAME} PUBLIC ${PROJECT_INCLUDE_DIR})

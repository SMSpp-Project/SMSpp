set(SMS++_SOURCES
    src/Block.cpp
    src/ColVariable.cpp
    src/ColVariableSolution.cpp
    src/Configuration.cpp
    src/Constraint.cpp
    src/DQuadFunction.cpp
    src/FRealObjective.cpp
    src/FRowConstraint.cpp
    #src/LagBFunction.cpp
    src/LinearFunction.cpp
    src/Objective.cpp
    src/OneVarConstraint.cpp
    src/Solver.cpp
    src/ThinComputeInterface.cpp
    src/Variable.cpp
    )

add_library(SMS++ STATIC ${SMS++_SOURCES})

# Test executable
#add_subdirectory(test)

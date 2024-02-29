##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of the core SMS++                                               #
#                                                                            #
#   The makefile takes in input the -I directives for all the external       #
#   libraries needed by the core SMS++ and *adds* to it the -I directive     #
#   for the library source. This makefile is assumed to be included in some  #
#   other "main" makefile setting the target and doing the finishing, hence  #
#   external libraries and the corresponding -L< libdirs > are not dealt     #
#   with here.                                                               #
#                                                                            #
#   Input:  $(CC)       = compiler command                                   #
#           $(SW)       = compiler options                                   #
#           $(SMS++SDR) = the directory where the core SMS++ source is       #
#           $(SMS++INC) = the -I for the libraries needed by the core SMS++: #
#                         - the Boost library                                #
#                         - the Eigen library                                #
#                         - the NetCDF library                               #
#                                                                            #
#   Output: $(SMS++OBJ) = the final objects                                  #
#           $(SMS++H)   = the .h files to include for the core SMS++         #
#           $(SMS++INC) = all the -I$( include dirs ) of the core SMS++      #
#                                                                            #
#                              Antonio Frangioni                             #
#                         Dipartimento di Informatica                        #
#                             Universita' di Pisa                            #
#                                                                            #
##############################################################################

# macros to be exported - - - - - - - - - - - - - - - - - - - - - - - - - - -

SMS++OBJ = $(SMS++SDR)/obj/AbstractBlock.o \
	$(SMS++SDR)/obj/BendersBFunction.o \
	$(SMS++SDR)/obj/BendersBlock.o \
	$(SMS++SDR)/obj/Block.o \
	$(SMS++SDR)/obj/BlockSolverConfig.o \
	$(SMS++SDR)/obj/BoxSolver.o \
	$(SMS++SDR)/obj/C05Function.o \
	$(SMS++SDR)/obj/ColRowSolution.o \
	$(SMS++SDR)/obj/ColVariable.o \
	$(SMS++SDR)/obj/ColVariableSolution.o \
	$(SMS++SDR)/obj/Configuration.o \
	$(SMS++SDR)/obj/Constraint.o \
	$(SMS++SDR)/obj/DataMapping.o \
	$(SMS++SDR)/obj/DQuadFunction.o \
	$(SMS++SDR)/obj/FRealObjective.o \
	$(SMS++SDR)/obj/FRowConstraint.o \
	$(SMS++SDR)/obj/Function.o \
	$(SMS++SDR)/obj/LagBFunction.o \
	$(SMS++SDR)/obj/LinearFunction.o \
	$(SMS++SDR)/obj/Objective.o \
	$(SMS++SDR)/obj/OneVarConstraint.o \
	$(SMS++SDR)/obj/PolyhedralFunction.o \
	$(SMS++SDR)/obj/PolyhedralFunctionBlock.o \
	$(SMS++SDR)/obj/RBlockConfig.o \
	$(SMS++SDR)/obj/RowConstraintSolution.o \
	$(SMS++SDR)/obj/Solver.o \
	$(SMS++SDR)/obj/ThinComputeInterface.o \
	$(SMS++SDR)/obj/Variable.o
#	$(SMS++SDR)/obj/RowConstraint.o \

SMS++INC := $(SMS++INC) -I$(SMS++SDR)/include

SMS++H = $(SMS++SDR)/include/AbstractBlock.h \
	$(SMS++SDR)/include/AbstractPath.h \
	$(SMS++SDR)/include/BendersBFunction.h \
	$(SMS++SDR)/include/BendersBlock.h \
	$(SMS++SDR)/include/Block.h \
	$(SMS++SDR)/include/BlockSolverConfig.h \
	$(SMS++SDR)/include/BoxSolver.h \
	$(SMS++SDR)/include/C05Function.h \
	$(SMS++SDR)/include/C15Function.h \
	$(SMS++SDR)/include/CDASolver.h \
	$(SMS++SDR)/include/Change.h \
	$(SMS++SDR)/include/ColRowSolution.h \
	$(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/ColVariableSolution.h \
	$(SMS++SDR)/include/Configuration.h \
	$(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/DataMapping.h \
	$(SMS++SDR)/include/DQuadFunction.h \
	$(SMS++SDR)/include/FakeSolver.h \
	$(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/LagBFunction.h \
	$(SMS++SDR)/include/LinearFunction.h \
	$(SMS++SDR)/include/Modification.h \
	$(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/Observer.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/PolyhedralFunction.h \
	$(SMS++SDR)/include/PolyhedralFunctionBlock.h \
	$(SMS++SDR)/include/RBlockConfig.h \
	$(SMS++SDR)/include/RowConstraint.h \
	$(SMS++SDR)/include/RowConstraintSolution.h \
	$(SMS++SDR)/include/SMSTypedefs.h \
	$(SMS++SDR)/include/Solution.h \
	$(SMS++SDR)/include/Solver.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h \
	$(SMS++SDR)/include/UpdateSolver.h \
	$(SMS++SDR)/include/Variable.h

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(SMS++SDR)/obj/*.o $(SMS++SDR)/src/*~ $(SMS++SDR)/include/*~

# dependencies: every .o from its .C + every recursively included .h- - - - -

# temp macro for what Block includes, and basically everything includes Block
intBlockINC = $(SMS++SDR)/include/Block.h $(SMS++SDR)/include/Configuration.h \
	$(SMS++SDR)/include/Modification.h $(SMS++SDR)/include/Solver.h \
	$(SMS++SDR)/include/Observer.h $(SMS++SDR)/include/SMSTypedefs.h \
	$(SMS++SDR)/include/ThinComputeInterface.h

$(SMS++SDR)/obj/AbstractBlock.o: $(SMS++SDR)/src/AbstractBlock.cpp \
	$(SMS++SDR)/include/AbstractBlock.h $(intBlockINC) \
	$(SMS++SDR)/include/Variable.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Objective.h $(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/RowConstraint.h  \
	$(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/OneVarConstraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/ColVariableSolution.h \
	$(SMS++SDR)/include/Solution.h
	$(CC) -c $(SMS++SDR)/src/AbstractBlock.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/BendersBFunction.o: $(SMS++SDR)/src/BendersBFunction.cpp \
	$(SMS++SDR)/include/AbstractPath.h \
	$(SMS++SDR)/include/BendersBFunction.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/FRowConstraint.h $(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/Observer.h $(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/SMSTypedefs.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/Solution.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/BendersBFunction.cpp -o $@ $(SMS++INC) $(SW)
 
$(SMS++SDR)/obj/BendersBlock.o: $(SMS++SDR)/src/BendersBlock.cpp \
	$(intBlockINC) $(SMS++SDR)/include/AbstractPath.h \
	$(SMS++SDR)/include/BendersBFunction.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/FRowConstraint.h $(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/Variable.h $(SMS++SDR)/include/Solution.h \
	$(SMS++SDR)/include/ThinComputeInterface.h
	$(CC) -c $(SMS++SDR)/src/BendersBlock.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Block.o: $(SMS++SDR)/src/Block.cpp $(intBlockINC) \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/Block.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/BlockSolverConfig.o: $(SMS++SDR)/src/BlockSolverConfig.cpp \
	$(SMS++SDR)/include/BlockSolverConfig.h $(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/BlockSolverConfig.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/BoxSolver.o: $(SMS++SDR)/src/BoxSolver.cpp $(intBlockINC) \
	$(SMS++SDR)/include/BoxSolver.h $(SMS++SDR)/include/C05Function.h \
	$(SMS++SDR)/include/C15Function.h $(SMS++SDR)/include/CDASolver.h \
	$(SMS++SDR)/include/ColVariable.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/DQuadFunction.h \
	$(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/FRowConstraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/LinearFunction.h $(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h \
	$(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/BoxSolver.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/C05Function.o: $(SMS++SDR)/src/C05Function.cpp \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/Modification.h $(SMS++SDR)/include/Observer.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/C05Function.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Configuration.o: $(SMS++SDR)/src/Configuration.cpp \
	$(SMS++SDR)/include/Configuration.h $(SMS++SDR)/include/SMSTypedefs.h
	$(CC) -c $(SMS++SDR)/src/Configuration.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/ColVariable.o: $(SMS++SDR)/src/ColVariable.cpp \
	$(SMS++SDR)/include/ColVariable.h $(SMS++SDR)/include/Variable.h \
	$(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/ColVariable.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/ColRowSolution.o: $(SMS++SDR)/src/ColRowSolution.cpp \
	$(intBlockINC) $(SMS++SDR)/include/ColRowSolution.h \
	$(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/ColVariableSolution.h \
	$(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/FRowConstraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h \
	$(SMS++SDR)/include/RowConstraintSolution.h \
	$(SMS++SDR)/include/Solution.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/ColRowSolution.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/ColVariableSolution.o: $(SMS++SDR)/src/ColVariableSolution.cpp \
	$(intBlockINC) $(SMS++SDR)/include/ColVariableSolution.h \
	$(SMS++SDR)/include/Solution.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/ThinComputeInterface.h
	$(CC) -c $(SMS++SDR)/src/ColVariableSolution.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Constraint.o: $(SMS++SDR)/src/Constraint.cpp \
	$(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h $(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/Constraint.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/DataMapping.o: $(SMS++SDR)/src/DataMapping.cpp $(intBlockINC) \
	$(SMS++SDR)/include/AbstractPath.h $(SMS++SDR)/include/C05Function.h \
	$(SMS++SDR)/include/C15Function.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/LinearFunction.h $(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/PolyhedralFunction.h \
	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/Solution.h $(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/DataMapping.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/DQuadFunction.o: $(SMS++SDR)/src/DQuadFunction.cpp \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/C15Function.h \
	$(SMS++SDR)/include/ColVariable.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/DQuadFunction.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/Variable.h $(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/DQuadFunction.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/FRealObjective.o: $(SMS++SDR)/src/FRealObjective.cpp \
	$(intBlockINC) $(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/Objective.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/FRealObjective.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/FRowConstraint.o: $(SMS++SDR)/src/FRowConstraint.cpp \
	$(intBlockINC) $(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/Function.h $(SMS++SDR)/include/RowConstraint.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/FRowConstraint.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Function.o: $(SMS++SDR)/src/Function.cpp \
	$(SMS++SDR)/include/Function.h $(SMS++SDR)/include/Modification.h \
	$(SMS++SDR)/include/Observer.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/Function.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/LagBFunction.o: $(SMS++SDR)/src/LagBFunction.cpp \
	$(intBlockINC) $(SMS++SDR)/include/LagBFunction.h \
	$(SMS++SDR)/include/BlockSolverConfig.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/C15Function.h \
	$(SMS++SDR)/include/ColVariable.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/Function.h $(SMS++SDR)/include/LinearFunction.h \
	$(SMS++SDR)/include/Objective.h $(SMS++SDR)/include/RBlockConfig.h \
	$(SMS++SDR)/include/RowConstraint.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h \
	$(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/LagBFunction.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/LinearFunction.o: $(SMS++SDR)/src/LinearFunction.cpp \
	$(SMS++SDR)/include/LinearFunction.h $(SMS++SDR)/include/C15Function.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/ColVariable.h $(SMS++SDR)/include/Variable.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/Modification.h \
	$(SMS++SDR)/include/Observer.h $(SMS++SDR)/include/SMSTypedefs.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/LinearFunction.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Objective.o: $(SMS++SDR)/src/Objective.cpp \
	$(SMS++SDR)/include/Objective.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h $(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/Objective.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/OneVarConstraint.o: $(SMS++SDR)/src/OneVarConstraint.cpp \
	$(intBlockINC) $(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Constraint.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h
	$(CC) -c $(SMS++SDR)/src/OneVarConstraint.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/PolyhedralFunction.o: $(SMS++SDR)/src/PolyhedralFunction.cpp \
	$(SMS++SDR)/include/PolyhedralFunction.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Function.h $(SMS++SDR)/include/Modification.h \
	$(SMS++SDR)/include/LinearFunction.h $(SMS++SDR)/include/Observer.h \
	$(SMS++SDR)/include/SMSTypedefs.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h \
	$(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/PolyhedralFunction.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/PolyhedralFunctionBlock.o: $(intBlockINC) \
	$(SMS++SDR)/src/PolyhedralFunctionBlock.cpp \
	$(SMS++SDR)/include/PolyhedralFunctionBlock.h \
	$(SMS++SDR)/include/C05Function.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/Function.h $(SMS++SDR)/include/FRealObjective.h \
	$(SMS++SDR)/include/FRowConstraint.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/PolyhedralFunction.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h $(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/PolyhedralFunctionBlock.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/RBlockConfig.o: $(SMS++SDR)/src/RBlockConfig.cpp \
	$(SMS++SDR)/include/RBlockConfig.h $(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/RBlockConfig.cpp -o $@ $(SMS++INC) $(SW)

#$(SMS++SDR)/obj/RowConstraint.o: $(SMS++SDR)/src/RowConstraint.cpp \
#	$(intBlockINC)
#	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Constraint.h
#	$(SMS++SDR)/include/ThinVarDepInterface.h
#	$(CC) -c $(SMS++SDR)/src/RowConstraint.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/RowConstraintSolution.o: \
	$(SMS++SDR)/src/RowConstraintSolution.cpp \
	$(SMS++SDR)/include/RowConstraintSolution.h \
	$(SMS++SDR)/include/Constraint.h $(SMS++SDR)/include/ColVariable.h \
	$(SMS++SDR)/include/FRowConstraint.h $(SMS++SDR)/include/Function.h \
	$(SMS++SDR)/include/OneVarConstraint.h \
	$(SMS++SDR)/include/RowConstraint.h $(SMS++SDR)/include/Solution.h \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/ThinVarDepInterface.h $(SMS++SDR)/include/Variable.h
	$(CC) -c $(SMS++SDR)/src/RowConstraintSolution.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Solver.o: $(SMS++SDR)/src/Solver.cpp $(intBlockINC) \
	$(SMS++SDR)/include/FakeSolver.h $(SMS++SDR)/include/UpdateSolver.h
	$(CC) -c $(SMS++SDR)/src/Solver.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/Variable.o: $(SMS++SDR)/src/Variable.cpp \
	$(SMS++SDR)/include/Variable.h $(intBlockINC)
	$(CC) -c $(SMS++SDR)/src/Variable.cpp -o $@ $(SMS++INC) $(SW)

$(SMS++SDR)/obj/ThinComputeInterface.o: \
	$(SMS++SDR)/src/ThinComputeInterface.cpp \
	$(SMS++SDR)/include/ThinComputeInterface.h \
	$(SMS++SDR)/include/Configuration.h $(SMS++SDR)/include/SMSTypedefs.h
	$(CC) -c $(SMS++SDR)/src/ThinComputeInterface.cpp -o $@ $(SMS++INC) $(SW)

########################## End of makefile ###################################

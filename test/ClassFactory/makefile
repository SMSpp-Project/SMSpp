##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for core SMS++)                                        #
#                                                                            #
#                              Antonio Frangioni                             #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = test

# basic directory
DIR = .

# compiler
CC = clang++

# debug switches
SW = -g -std=c++17 -Wno-enum-compare -ferror-limit=1

# production switches
#SW = -O3 -DNDEBUG -std=c++17

# linker option for forcing all the library to be loaded
# if a component is not explicitly referenced anywhere in the object, the
# linker may decide to exclude it from the final executable. if this happens
# for a component defining one of the classes used in the test (a :Block,
# a :Configuration, a :Solver), the code inserting it in the factory is not
# included and the test wll fail. in this case, one must use the option of
# the linker that disables this optimization (e.g., whole-archive for g++,
# force_load for clang++). this macro must define said option
FORCE = -Wl,--whole-archive,--allow-multiple-definition

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

all default: $(NAME)

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)/*.o $(DIR)/*~ $(NAME)

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# if a module is not used in the current configuration, just comment out the
# corresponding include line
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libraries + -L< libdirs >
# *H   is the list of all include files
# *INC is the -I< include directories >

# core SMS++
SMS++SDR = ../..
include $(SMS++SDR)/lib/makefile-inc

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(SMS++OBJ)

# libraries
MLIB =  $(SMS++LIB)

$(NAME): $(MOBJ) $(DIR)/main.o
	$(CC) -o $(NAME) $(DIR)/main.o $(FORCE) $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(SMS++INC)

# includes
MH =    $(SMS++H)

# compile command

$(DIR)/main.o: $(DIR)/main.cpp $(MH)
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile ################################

##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for PolthedralFunction)                                #
#                                                                            #
#                                VERSION 1.00                                #
#                               23 - 07 - 2019                               #
#                                                                            #
#                              Implementation by:                            #
#                                                                            #
#                              Antonio Frangioni                             #
#                           Operations Research Group                        #
#                          Dipartimento di Informatica                       #
#                              Universita' di Pisa                           #
#                                                                            #
##############################################################################

# module name
NAME = test

# basic directory
DIR = ./

# debug switches
SW = -g -std=c++14 -ferror-limit=1
# production switches
#SW = -O3 -std=c++14 -DNDEBUG

# compiler
CC = clang++

# default target- - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

default: $(NAME)

# clean - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -

clean::
	rm -f $(DIR)*.o $(DIR)*~ $(NAME)

# define & include the necessary modules- - - - - - - - - - - - - - - - - - -
# if a module is not used in the current configuration, just comment out the
# corresponding include line
# each module outputs some macros to be used here:
# *OBJ is the final object(s) / library
# *LIB is the external libreries + -L<libdirs>
# *H   is the list of all include files
# *INC is the -I<include directories>

# MILPSolver
MILPSSDR = ../
include $(MILPSSDR)makefile-c

# BundleSolver
BNDSLVSDR = ../../BundleSolver/
BNDSLVODR = ../../BundleSolver/
include $(BNDSLVSDR)makefile-c

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(MILPSOBJ) $(BNDSLVOBJ)

# libreries
MLIB =  $(MILPSLIB) $(BNDSLVLIB)

$(NAME): $(MOBJ) $(DIR)$(NAME).o
	$(CC) -o test $(DIR)$(NAME).o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(MILPSINC) $(BNDSLVINC)

# includes
MH =    $(MILPSH) $(BNDSLVH)

# compile command

$(DIR)$(NAME).o: $(DIR)$(NAME).cpp $(MH)
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################

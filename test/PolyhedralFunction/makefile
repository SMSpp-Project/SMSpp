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

# BundleSolver (this also includes SMS++)
BNDSLVSDR = ../../../BundleSolver/
BNDSLVODR = ../../../BundleSolver/
include $(BNDSLVSDR)makefile-c

# define input macros for SMS++ complete makefile, then include it
#SMS++SDR = ../../../SMS++/
#include $(SMS++SDR)lib/makefile-inc

# define input macros for libCPLEX complete makefile, then include it
libCPLEXDIR = ../../../extlib/
include $(libCPLEXDIR)makefile-libCPLEX

# MILPSolver
MILPSSDR = ../../../MILPSolver/
include $(MILPSSDR)makefile


# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(MILPSOBJ) $(BNDSLVOBJ) $(libNDOOBJ) $(libCPLEXOBJ) $(SMS++OBJ)

# libreries
MLIB =  $(MILPSLIB) $(BNDSLVLIB) $(libNDOLIB) $(libCPLEXLIB) $(SMS++LIB) 

$(NAME): $(MOBJ) $(DIR)$(NAME).o
	$(CC) -o test $(DIR)$(NAME).o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(MILPSINC) $(BNDSLVINC) $(libNDOINC) $(libCPLEXINC) $(SMS++INC)

# includes
MH =    $(MILPSH) $(BNDSLVH)

# compile command

$(DIR)$(NAME).o: $(DIR)$(NAME).cpp $(MH)
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile #################################

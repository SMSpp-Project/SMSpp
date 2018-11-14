##############################################################################
################################ makefile ####################################
##############################################################################
#                                                                            #
#   makefile of test (for core SMS++)                                        #
#                                                                            #
#                                VERSION 2.00                                #
#                               20 - 04 - 2005                               #
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
SW = -g -std=c++14

# production switches
#SW = -O3 -DNDEBUG -std=c++14

# compiler
#CC = g++-8
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

# core SMS++
SMS++SDR = ../
include $(SMS++SDR)lib/makefile-inc

# main module (linking phase) - - - - - - - - - - - - - - - - - - - - - - - -

# object files
MOBJ =  $(SMS++OBJ)

# libreries
MLIB =  $(SMS++LIB)

$(NAME): $(MOBJ) $(DIR)main.o 
	$(CC) -o $(NAME) $(DIR)main.o $(MOBJ) $(MLIB) $(SW)

# dependencies: every .o from its .C + every recursively included .h- - - - -

# include directives
MINC =  $(SMS++INC)

# includes
MH =    $(SMS++H)

# compile command

$(DIR)main.o: $(DIR)main.cpp $(MH) 
	$(CC) -c $*.cpp -o $@ $(MINC) $(SW)

############################ End of makefile ################################


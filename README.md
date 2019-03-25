The "core" SMS++ {#mainpage}
================

![To boldly model (and solve) what no one has modeled (and solved) before](./doxygen/SMSpp_logo_mid_noback.png)

Introduction
------------

This is a the splash page of the documentation of the "core" SMS++, a set of
C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them via
sophisticated, structure-exploiting algorithms (in praticular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

A General Description of SMS++
------------------------------

Current systems for modeling mathematical models typically falls into two
opposite approaches:

- *General-purpose Algebraic Modeling Languages*, commercial ones like
  [AMPL](https://ampl.com) and [GAMS](https://www.gams.co/), or open-source
  ones like [Coliop](http://www.coliop.org) and [ZIMPL](http://zimpl.zib.de).
  These try to abstract away as much as possible the mathematical model from
  the underlying solver, thereby
  taking away from the user all the effort
  from interfacing with specific solvers tackling their idiosyncrasies.
  However, in order to do to they typically "flatten" all the structure
  available in the model, making it almost unreachable from the underlying
  solver. Some extensions have been proposed for specific forms of structure,
  mainly the block one, e.g. in the [Structured Modeling Language
  (SML)](https://www.maths.ed.ac.uk/ERGO/sml). An alternativa approach has
  been to re-construct the structure backwards from the "flat" model, as
  [Generic Column Generation (GCG)](http://www.or.rwth-aachen.de/gcg) does.

- The alternative has typically been to *directly interface to a specific
  solver via its APIs*; all modern general-purpose (and specialized)
  solvers, both commercial ones like
  [Cplex](https://www.ibm.com/analytics/cplex-optimizer),
  [GuRoBi](http://www.gurobi.com) and [MOSEK](https://www.mosek.com), and
  open-source ones like [Cbc](https://projects.coin-or.org/Cbc) and
  [SCIP](http://scip.zib.de), offer one or more APIs, usually for
  different languages (among which, surely C/C++). This allows to directly
  address all the structure-exploiting features of the solver, if any, but
  directly explosed the user to its complexity and idiosyncrasies.

- A somewhat "in the middle" approach is that of *modeling systems embedded
  in general purpose languages*, such as 
  [FlopC++](https://projects.coin-or.org/FlopC++) and
  [COIN-Reharse](https://github.com/coin-or/Rehearse) for C++,
  [PuLP](https://pythonhosted.org/PuLP) and
  [Pyomo](http://www.pyomo.org) for Python,
  [JuMP](https://github.com/JuliaOpt/JuMP.jl) for Julia and
  [YALMIP](https://yalmip.github.io) for Matlab. This allows to better deal
  with parts of the model that have specific structure and allow for
  specialized solution method, and thereby implement sophisticated
  structure-exploiting algorithms. However, at their hearth these systems
  typically still construct "flat" models, and have little explicit support
  for managing the structure in the model.

SMS++ tries a different approach, somehow extending the latter one but even
more deeply rooted in programming languages (after all, mathematical
optimization used to be known as "mathematical programming" for a good
reason). It is a modeling system based on a set of C++ classes, where a
model is represented as a "block", containing some collections of
"variables" and "constraints", plus one "objective function", possibly
organized (recursively) into sub-blocks. The current distribution provides
the corresponding *abstract* Block, Variable, Constraint and Objective
classes, that try to establish the minimal possible interface of such a
modeling system. In particular, Variable, Constraint and Objective make as
little assumptions as possible on the actual form of the corresponding
mathematical objects. However, a RealObjective class is already derived
from Objective in order to al least define the crucial concept of an
objective function with real values.

A first level of specification is then provided by the derived classes
ColVariable (a single real Variable, possibly with integrality
restrictions), and RowConstraint (a Constraint of the form "LHS <= ( single
real expression ) <= RHS"). Also, a general Function class is provided for
expressing a very general real-valued function concept, together with its
derived C05Function (providing linearizations, i.e., first-order information,
but not necessarily continuous one) and C15Function (providing quadratic
models, i.e., second-order information, but not necessarily continuous one).
With this, FRowConstraint and FRealObjective are defined as RowConstraint
and RealObjective taking a generic Function to implement them. Some basic
aspects of the interface of Constraint, Objective and Function are factored
out in the abstract ThinVarDepInterface class, and some other aspects are
factored out in the different abstract ThinComputeInterface class.

Then, to get to actual concrete objects one only have to define actual
Function; currently what is provided is LinearFunction, which then allows
to represent the all-important class of Mixed-Integer Linear Problems.
Other concrete functions can be easily defined. Also, the (abstract)
class OneVarConstraint is separately defined for RowConstraint depending
on only one variable (and which therefore don't really need a Function),
from of which BoxConstraint and some specialized versions (like NNConstraint
and NPConstraint for non-negativity and non-positivity Constraint,
respectively) are derived.

The system is completed by Solver, an abstract base class setting the
general interface between a Block representing a mathematical model and any
algorithm designed to solve (i.e., produce solutions for) it, together with
a slightly specialized derived CDASolver aimed at solution algorithms that
exploit convex structures (and, in particular, duality) in the model.
Solver also share the ThinComputeInterface interface, which defines how
a (potentially) computationally demanding task is to be executed, and how
(potentially, many) parameters controlling it can be provided. The latter
is done via a ComputeConfiguration object, deriving from the general
Configuration class intended to represent objects for collecting many
(structured) algorithmic parameters. The class is also the basis for
BlockConfiguration (collecting parameters for different tasks that a
Block has to accomplish), and BlockSolverConfiguration (collecting
parameters for all the Solver attached to a Block and its sub-Block,
recursively). Configuration also offers a factory (to which derived
classes have to explicitly register, say using the provided macros).

The idea is that Solver will be specialized for specifically-structured
mathematical models, corresponding to specific derived classes of Block
using specific forms of Variable, Constraint and Objective. Block is meant
to represent the general concept of "a part of a mathematical model with a
well-understood identity"; that is, derived classes will each represent a
model with a specific structure (say, a Knapsack, a Traveling Salesman
Problem, a SemiDefinite program, ...). Hence, specialized Solver will be
able to exploit this *semantically defined* structure (the "physical
representation" of the Block) to provide faster and more effective solution
methods, possibly without ever using the "abstract representation" of the
Block in terms of its Variable, Constraint and Objective. Solver can be
attached to individual sub-Blocks of a larger, composed Block (recursively),
easing the implementation of solution algorithms based on advanced
techniques (like decomposition ones). However, each specialized Block will
always be able to provide an "abstract representation" of the Block in
terms of its Variable, Constraint and Objective, in order to be able to
implement solution methods that mix general-purpose Solver for large
classes of mathematical models (say, Mixed-Integer Linear Programs) and
ad-hoc Solver for very specific structures. BlockSolverConfiguration is
specifically constructed to support the notion that "a Solver for a Block"
can actually be a large collection of Solver attached to the Block and its
sub-Block (recursively). In order to allow the Solver to be automatically
created and attached to their Block via a BlockSolverConfiguration, Solver
offers a factory (to which derived classes have to explicitly register, say
using the provided macros).

SMS++ currently supports the following general mechanisms:

- changes in the data of a Block, Variable, Constraint, Objective, Function
  ... are treated in a "fully lazy" way, i.e., are recorded into objects of
  (classed derived from the base) class Modification, that are then shipped
  to all Solver registered to the Block (and all its ancestors).

- The base Block class supports the "abstract representation" being made
  of any derived class from Variable, Constraint and Objective, using
  boost::any to support individual objects, arrays and
  multi-dimensional arrays (using boost:multi_array) of objects.

- Block provides a factory object to dynamically generate object of any
  class derived by Block just passing it the string of the classname
  (provided the class is registered to the factory, for which support
  is provided with some macros).

- Block supports (although only in the interface, not in any practical
  implementation) the concept that a Block can produce different versions
  of itself that are either equivalent (a Reformulation), a Relaxation or
  a Restriction. The interface caters for the fact that both solution
  information (see Solution) and Modification can be mapped back and forth
  from a Block to any one of its "R3 Block", although this is all predicated
  on the fact that the Block itself implements this.

- Variable and Constraint of a Block can be both static and dynamic,
  in order to allow strategies where they can be dynamically generated
  while providing the Solver with information about which are never
  going to change.

- The solution status of a Block (the value of its Variable) can be saved
  in appropriate objects derived from the base Solution class, and then
  read back from these in the Blocl. The Solution objects can possibly save
  any subset of the information (as dictated by a Configuration object) and
  dual solution if available. A very general ColVariableSolution object is
  provided for doing this only using the "abstract representation" for
  Block that only have ColVariable in them.

- there is some support for saving an entire Block as a model-description
  file and for loading it from a model-description file; currently,
  methods using the netCDF library are being tested.

SMS++ currently *does not* support a number of important aspects, such as
asynchronous execution of the computationally heavy parts (see
ThinComputeInterface). However, it is hopefully structured in such a way
that these aspects can be added later with relatively minimal disruption
of the current interface.


Software Dependencies
=====================

SMS++ requires a modern (at least C++-14 compliant) C++ compiler. It relies
on a few boost libraries

    https://www.boost.org

in particular:

- boost::any for vectors and lists of "any" kind of Constraint and
  Variable (although this may in the future be substituted with std::any)

- boost::multi_array for multi-dimensional arrays of stuff

- boost::bind, boost::function, boost::functional::factory, and
  boost::functional::forward_adapter for a factory construct

It also relies on Eigen

    http://eigen.tuxfamily.org

for sparse vectors and matrices, although the use is so sparse right now
(pun intended) that it is not completely sure the dependency will be
mantained.

Finally, it relies on the C++ interface to NetCDF

    https://www.unidata.ucar.edu/software/netcdf

for efficient serialization and de-serialization of Block and Configuration
objects on self-describing, machine-independent data files.


Legal Stuff
===========

Standard Disclaimer
-------------------

The code is currently provided free of charge for academic purposes only.
As such, it is provided "as is", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.

License
-------

This code is provided free of charge for academic purposes under the
"academic license": see the file doc/academicl.txt for further details. Because
the code is currently in the early stages of its development we prefer a
stricter licensing regime w.r.t. LGPL in order to discourage too early
fragmentation of the code. Thus, you can make changes, but we strongly suggest
that you do not distribute any modified version (although you are legally
allowed to within the terms of the license) without prior agreement with the
original developers; if your changes make good sense, please allow us to
incorporate them in the standard release. Yet, it is foreseen that the license
will be moved to LGPL as soon as the code is deemed stable enough to be widely
distributed.

Version
=======

- Current version of SMS++ is: 0.10

- Current date is:             14 - Nov - 2018

Authors
=======

Current Lead Authors
--------------------

	Antonio Frangioni
	Operations Research Group
	Dipartimento di Informatica
	Universita' di Pisa
 
	Rafael Durbano Lobato
 	Department of Applied Mathematics
	State University of Campinas, Brazil

Previous Lead Authors and Contributors
--------------------------------------

	Kostas Tavlaridis-Gyparakis
	Operations Research Group
	Dipartimento di Informatica
	Universita' di Pisa

	Utz-Uwe Haus
	Cray EMEA Research Lab


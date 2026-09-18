# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `Block` holds a group of its own for each of the four vectors of
  `boost::any` in which it keeps its Variable and its Constraint: a group
  says the type of its elements, its shape and its name, and hands them over
  without the caller having to know the type of the container they sit in,
  which is what the `un_any_*` machinery was for. `BaseGroup::for_each_as()`
  walks them with one switch per group and a loop typed on the element,
  `for_each_run_as()` gives them one run of contiguous ones at a time, which
  is what a caller mapping an element back to its position from its address
  needs, `elements_are()` answers the question on the type once for the whole
  group, and `Block::for_each_variable_group()` and
  `for_each_constraint_group()` walk the static groups and then the dynamic
  ones. THE ORDER IN WHICH THE ELEMENTS COME OUT IS THE STORAGE ORDER, and it
  is part of the contract: `tests_Group.cpp` fixes it

- A group says how to build a container of its own type and shape in another
  Block, which is what `AbstractBlock::mirror()` needed the `boost::any` for,
  and whoever allocates a container says how it goes, so that a Block
  disposes of what it owns through its own groups

- `std::vector< std::vector< Var > >` can be registered as a group of
  Variable, as it already could be as a group of Constraint: the two sides
  now have the same list of shapes

### Changed

- ⚠️ THE LAYOUT OF `Block` HAS CHANGED, and `add_static_variable()` and the
  other 35 registration methods are templates, hence they live in the
  translation unit of whoever calls them: after updating, EVERYTHING has to
  be rebuilt, not only `libSMS++`, and a stale object file is not a
  compilation error but a group without the means to copy itself, or a
  library that is a hybrid of two layouts

- `GroupAdapter.h` is gone, having been the scaffolding that read the
  `boost::any` while the consumers of them were converted one at a time, and
  so are `Block::refresh_*_group()`, which existed to rebuild a group after
  writing into `access_*()` and which nobody calls

### Changed

- `PolyhedralFunctionBlock`, in the "linearized dual" representation, issues
  the Modification of a row being added or removed inside a
  `VariableGroupMod`: a row of the PolyhedralFunction is a column of the
  abstract representation, i.e., one Variable plus one coefficient in each row
  it appears in, and the group lets a Solver having a column operation of its
  own use it instead of reading the change one row at a time, while a Solver
  that does not recognise the group takes it apart and sees exactly the
  Modification it saw before

### Fixed

- `RowConstraint::is_feasible()` on a collection of collections of
  RowConstraint, and on a `boost::multi_array` of collections, took them as
  const while each RowConstraint has to be computed, so that it did not
  compile as soon as it was used

- `BendersBFunction` kept the dual solutions of its global pool when the
  Constraint of the sub-Block changed, adding and removing alike: a removal
  takes a dual variable away, and what is left satisfies the dual constraints
  only if that variable was zero, so the pool is now invalidated there, while
  an addition keeps it, the new dual variable being feasible at zero

- `LagBFunction` marked no Solution as written in the inner Block after
  checking one of the global pool against a Block that is not
  `is_sol_feasible_physical()`: the check goes through the Variable there,
  and what is put back is only as complete as the Solution the Block hands
  out

- the check of a Solution against a Block, which is what the default
  `is_sol_feasible()` and `is_sol_optimal()` do, silently skipped putting
  back what the Variable held when the Block could not hand out its current
  Solution, leaving it with the checked Solution written in it; it now
  throws instead

## [0.7.1] - 2026-09-13

### Fixed

- a direction is checked against a quadratic row too, the sign of d^T Q d
  deciding whether the row bounds it

## [0.7.0] - 2026-09-12

### Added

- `AbstractBlock::mirror()`, which builds the AbstractBlock as a copy of the
  abstract representation of any other Block: one ColVariable per
  ColVariable, one Constraint per Constraint and an Objective, the groups
  keeping their shape and their order and the inner Block being copied
  recursively, with a Constraint of the copy written in the Variable of the
  copy and a Variable living outside the mirrored subtree shared with the
  original. The copy knows which object of the original each of its own
  corresponds to, hence it moves solution information in both directions
  [see `mirror_read()` and `mirror_write()`] and applies to itself a
  Modification the original issues [see `mirror_forward_Modification()`],
  which is what an `UpdateSolver` attached to the original does by itself.
  What cannot be written on other Variable, which is any Function but a
  LinearFunction and a DQuadFunction, is left out and recorded [see
  `get_mirror_issues()`], so that the copy is then a relaxation and whoever
  asked for it can see that it is; the objects of each group are counted on
  both sides, so that a group of a shape the copy does not reproduce is
  recorded as well rather than quietly holding fewer objects

- the default implementation of `Block::map_back_solution()`,
  `Block::map_forward_solution()` and `Block::map_forward_Modification()`
  serves the AbstractBlock that has mirrored the Block, so that every Block
  has a R3 Block of itself without having had to write a line for it

- `Block::access_static_variable()` and its three companions, which give the
  group of Variable or Constraint as the boost::any holding it, so that code
  building a Block out of another one can install a group whose type it only
  knows at run time

- `ThinComputeInterface::print_parameters()`, which prints the name, the
  current value and the default one of every parameter, walking the six
  index spaces; for a class whose index space extends over that of a
  wrapped ThinComputeInterface it shows what the wrapped one has been
  given, which is what one needs to see when a parameter is suspected of
  not arriving where it was meant to

- `QuadFunction::remove_variables( Subset )`, so that a bunch of Variable can
  be removed in one call rather than one at a time

- `Block::set_structure()`, which decides the *structure* of a Block, i.e.,
  its tree of sub-Block, as opposed to its *formulation*, i.e., what its
  abstract representation encodes; `set_BlockConfig()` calls it as soon as it
  has digested the BlockConfig, hence before anything is generated and before
  any Solver is attached

- `BlockConfig::f_structure_Configuration`, the Configuration that
  `set_structure()` reads, which is the first one of a BlockConfig, since the
  structure comes before everything else

### Changed

- the text format of a BlockConfig carries a version number right after the
  differential flag, so that a file in the previous format is refused with a
  message rather than being read with all its Configuration shifted by one
  slot; every BlockConfig file has to be converted

- the version of the module is the git tag of its repository, or the
  VERSION.txt of a release tarball, and the shared library carries it: its
  SONAME is major.minor while the major is 0, and it is installed with an
  RPATH relative to itself, so that an installed tree keeps working wherever
  it is moved

### Fixed

- `LagBFunction::cleanup_inner_objective()` restored the original costs by
  rewriting the whole vector of coefficients, i.e. a Range spanning every
  variable of the inner Block. A :Block is entitled to refuse a change on
  some of its own, and ThermalUnitBlock does so for the schedule-deviation
  variables on the range rather than on the value: it therefore refused a
  restore that left those coefficients exactly where they were, which is
  what the AC instances of the test battery died on. Only the coefficients
  that actually differ are written now, as a Range when they are contiguous
  and as a Subset otherwise, mirroring how the Lagrangian costs are written

- `QuadFunction::remove_variables( Range )` computed the shift of the
  non-diagonal terms out of one Variable more than it was removing, kept the
  terms of the last removed one and, whenever an Observer was listening, spun
  in an infinite loop while collecting the names of the removed Variable

## [0.6.0] - 2025-12-12

### Added 

- SimpleConfiguration< std::pair< std::string ,
  Configuration * > >

- set\_caller() method to DataMapping

- strLogFileName string parameter in Solver

- ::deserialize for std::vector< std::string >

- [huge] new parameter intPushCostToOwner in LagBFunction
  allows to decide whether the Lagrangian term is added
  to the "root" inner Block (as previously) or rather
  in the [Linear/DQuad]Function of the Block where the
  ColVariable is defined
  
- added full netCDF file support for State

- un\_any\_thing\_OneVarConstraint\_*

- added full netCDF file support for Configuration

- updated interface to handle two-nested std::vector

- range_index field in AbstractPath

- updated SMSTypedefs to handle
  boost::multi\_array< std::vector< T > , K >

- added couple SimpleConfiguration

- added full netCDF file support for Solution

- support for arm64 under MacOS

- parameter to select the Solver of the inner Block in
  BendersBFunctio

- Block::almost\_deserialize()

- [huge] QuadFunction for non-diagonal general quadratic
  functions

- DQuadFunctionModVarsAddd

- support for reading .lp and .mps files in AbstractBlock,
  comprised handling of QP problems

### Changed 

- parameter in set\_ComputeConfig() is now const

- extended CLANG_1200_0_32_27_PATCH to all
  un\_any\_thing\_*, not only un\_any\_thing\_0

- Solution is no longer pure virtual (useful to define
  empty placeholder Solution)

- removed support for x86-64 under MacOS

- almost complete rehaul of makefiles (but more is needed)

- moved vectors for parameters inside methods

### Fixed 

- fixed conceptual flaw in LagBFunction: getting an empty
  Solution from the Block and accumulating all the
  Solution of a convex combination was not the right
  approach according to Solution definition, and in fact
  it was miserably failing with UnitBlockSolution

- removed memory leak in LagBFunction

- catastrophic bug in RBlockSolverConfig::apply()

- g++ issue with SMSpp_ensure_load

- dynamic destructor of SimpleConfiguration< stuff >

- map_active() in most *Function (added in BendersBFunction)

- many minor and major flaws

## [0.5.3] - 2024-02-29

### Added 

- "father of LagBFunction" mechanism

- AbstractBlock::read_lp()

- AbstractBlock::read_mps()

### Changed 

- adapted to new CMake / makefile organisation

### Fixed 

- flaw in DQuadFunction

- template arguments handling in SMSTypedefs.h

- BendersBFunction::delete_rows()

- badly mangled LagBFunction::InnerSolver

## [0.5.2] - 2023-05-17

### Added

- C05Function uses Function::set_par
- RowConstraint::is_feasible()

### Changed

- definition of RowConstraint::rel_viol()
- feasibility check in AbstractBlock

### Removed

- Constraint::is_feasible()

### Fixed

- dynamic cast in put_State() (BendersBFunction, LagBFunction, and
  PolyhedralFunction)
- copy constructor of BlockConfig

## [0.5.1] - 2022-06-28

### Added

- added ThinVarDepInterface::map\_index()

- added `is_feasible()` and `clear()` also for std::list,
  std::vector< std::list > and boost::multi_array< std::list >

- added generic `is_feasible` methods for Variable and Constraint types

- added RelaxationSolve concept

- added Change concept

- added ::deserialize() for std::string

- added Objective::eUndef as default value for get\_objective\_sense()

- added not\_ModBlock and un\_ModBlock

- added `clear_constraints` methods

- added INFRange

- added Solver::pop() and Solver::mod_clear()

- added ::deserialize() for matrix with fixed-length and variable-length rows

- added get\_constant\_term() in RealObjective

- computation of Lipschitz constant in LagBFunction

- passthrough" feature in LagBFunction where the set_par() and related
  mechanisms can be used to directly set the parameters in the inner Solver

### Changed

- dynamic Constraint not clear()-ed on deletion if they are shipped to a
  BlockModRmv; clearing is now done in the destructor of the BlockModRmv,
  so that the information about the set of active variable can still be
  "seen" by the Solver handling it. this turns out to be crucial for
  CPXMILPSolver to handle deletion of OneVarConstraint, but may be useful
  in general

- reworked channel management: now open\_channel() and close\_channel()
  suffice. added open\_if\_needed() and close\_if\_needed() versions

- avoided un-necessary Modification in PolyhedralFunctionBlock

- changed load/print stream interface

- restored default true to optional in SMSTypedefs.h

- significant rehaul of ::deserialize() functions

- better management of ::deserialize() functions with checks about the
  types that are being deserialized

- complete redesign and significant enhancement in BoxSolver: now properly
  handles multi-level f_Block and Variable not belonging to it

### Fixed

- too many bugs to count

## [0.5.0] - 2021-12-08

### Added

- AbstractBlock can read MPS files.
- State (representation of the state of a ThinComputeInterface).
- BendersBFunctionState, LagBFunctionState, and PolyhedralFunctionState.
- Two new SimpleConfiguration.
- VariableGroupMod.

### Changed

- Computation of linearization constant in BendersBFunction.

### Fixed

- Bugs in LagBFunction.
- Multiplier in sum() of RowConstraintSolution and ColVariableSolution.
- get_*_index()/element() in BlockInspection.
- Bugs in BoxSolver.
- Flaw in AbstractBlock::is_feasible().

## [0.4.0] - 2021-02-05

### Added

- Some unit tests.

- Configuration header SMSppConfig.h.

- Rather large changes in LagBFunction, added LagBFunctionMod.

- Added VariableMod::old_type() and supporting methods.

- Cleaner style for un_any_thing macros.

- Implemented AbstractBlock::is_correct().

- Significant rehaul of handling "stealth" obj variables addition in
  LagBFunction: Variable addition is now performed batch in compute()
  rather than real-time when dealing with Modification.

- Added ColRowSolution.

- Significant improvements in load()-ing of Configurations,
  graciously terminating if the stream eof()-s.

- Added vectors in Configurations.

- Defined SMSpp\_classname\_normalise().

- LagBFunction now supports DQuadFunction Objective.

- Added InnrSlvr parameter in LagBFunction.

- Updated DataMapping Dealing with the in which SetFrom is empty when
  producing error messages.

- ThinVarDepInterface now has get_Block().

- Added BoxSolver.

- Updated serialization of BendersBFunction.

### Fixed

- Compilation error in DataMapping with GCC.

- Too many individual fixes to list.

## [0.3.2] - 2020-09-16

### Fixed

- Compilation error in DataMapping.

## [0.3.1] - 2020-09-16

### Fixed

- Bug in *BlockConfig::clear().

## [0.3.0] - 2020-09-16

### Added

- Support for concurrency.
- [O][C][R]BlockConfig for configuring also the Objective, Constraint, and
  sub-Block, recursively.
- RBlockSolverConfig for configuring the Solver of the sub-Block, recursively.

### Changed

- New configuration framework.

## [0.2.0] - 2020-03-06

### Added

- Name to Block

## [0.1.1] - 2020-02-10

### Fixed

- Compilation error in unit tests.

## [0.1.0] - 2020-02-10

### Added

- First test release.

[Unreleased]: https://gitlab.com/smspp/smspp/-/compare/0.7.0...develop
[0.7.0]: https://gitlab.com/smspp/smspp/-/compare/0.6.0...0.7.0
[0.6.0]: https://gitlab.com/smspp/smspp/-/compare/0.5.3...0.6.0
[0.5.3]: https://gitlab.com/smspp/smspp/-/compare/0.5.2...0.5.3
[0.5.2]: https://gitlab.com/smspp/smspp/-/compare/0.5.1...0.5.2
[0.5.1]: https://gitlab.com/smspp/smspp/-/compare/0.5.0...0.5.1
[0.5.0]: https://gitlab.com/smspp/smspp/-/compare/0.4.0...0.5.0
[0.4.0]: https://gitlab.com/smspp/smspp/-/compare/0.3.2...0.4.0
[0.3.2]: https://gitlab.com/smspp/smspp/-/compare/0.3.1...0.3.2
[0.3.1]: https://gitlab.com/smspp/smspp/-/compare/0.3.0...0.3.1
[0.3.0]: https://gitlab.com/smspp/smspp/-/compare/0.2.0...0.3.0
[0.2.0]: https://gitlab.com/smspp/smspp/-/compare/0.1.1...0.2.0
[0.1.1]: https://gitlab.com/smspp/smspp/-/compare/0.1.0...0.1.1
[0.1.0]: https://gitlab.com/smspp/smspp/-/tags/0.1.0

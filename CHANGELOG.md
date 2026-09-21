# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added

- `AbstractBlock::write_is()`, which `print( out , 'I' )` dispatches: after a
  `CDASolver` has proved the model unfeasible and `get_dual_direction()` has
  written the unbounded dual direction into the Block, it writes the rows
  whose multiplier is not zero, each with its multiplier and its name, and
  the bounds of the columns the same way. It asks nothing of any Solver, the
  ray being in the Constraint of the Block; what it writes is the certificate
  the Solver has left and not the smallest set of rows with that property

- `AbstractBlock::write_mps()`, which writes what the Block holds as the MPS
  file `read_mps()` reads, `print( out , 'M' )` having thrown "not implemented
  yet" as well. It says what `write_lp()` says and in the same names, with the
  two differences the format makes: a row with both sides finite and different
  is one row with its second side in `RANGES` rather than two rows, and a row
  whose two sides are both infinite is left out, the format having no way of
  saying it. A number is written with the fewest digits that read back as
  itself, in both writers, the six digits a stream gives by default not being
  enough to make the trip

- `AbstractBlock::write_lp()`, which writes what the Block holds as the LP
  file `read_lp()` reads, `print( out , 'L' )` having thrown "not implemented
  yet"; `serialize()` uses it to fill the netCDF variable `Model` that
  `deserialize()` has always read, with `ModelType = 'L'`. The Objective, the
  rows that are `FRowConstraint` on a `LinearFunction`, the bounds a column
  has of its own tightened by the `:OneVarConstraint` written on it and the
  integer columns are written; a row with both sides finite and different
  goes twice, `<name>_up` and `<name>_lo`, the format having no two-sided
  row. The names are those the groups give, with the indices joined by
  underscores, so `name_of_cell()` and `for_each_named_as()` take the two
  separators as a parameter. An LP file has no notion of groups, so what
  travels is the model and not the way it is grouped

- `serialize()` and `deserialize()` of `ColVariableSolution`,
  `RowConstraintSolution` and `ColRowSolution`, which threw "not ready yet":
  the values of the static stuff go in `StaticValues` (`StaticDuals` for the
  duals) with `StaticValuesStart` saying where each group begins, which is
  how SMS++ writes a matrix with rows of different length; the dynamic ones
  have one level more, the cells of all the groups going in one such matrix
  and `DynamicCellsStart` saying which of those cells each group begins at. A
  `ColRowSolution` writes its two halves in the groups `VariableSolution` and
  `ConstraintSolution`, and the Solution of a nested Block goes in
  `NestedSolution_<i>`

- `inspection::name_of()` gives a Variable or a Constraint the name of the
  group it sits in, or the index of that group when it has none, followed by
  the indices of its cell in the grid and, when the cells are collections, by
  its position inside its own cell; `inspection::for_each_named_as()` walks a
  whole group handing over each element with its name, reading the shape of
  the grid once rather than once per element, and
  `inspection::for_each_named_as_any_of()` does it running a cascade of
  concrete types. `AbstractBlock::print()` prints the Variable and the
  Constraint that way, which is what tells which one of them a row of the
  model is written on

- `Block::get_size_variable()` and `Block::set_size_variable()`, through
  which a :Block declares a column standing for a size parameter that it
  writes into its own rows, a column it owns or one it is given, normally by
  its father, a Solver seeing only ordinary Variable and Constraint; the
  size parameter that is a datum goes through the methods factory instead.
  `PolyhedralFunctionBlock` takes one, the multiplier of `set_lambda()`,
  before or after its abstract representation exists, and keeps its
  coefficient in step with the global scale, in place

- the methods factory takes the data of a setter as a `std::span` as well
  (`MF_dbl_sp`, `MF_int_sp` and the `MS_sp_*` signatures), which lets the
  setter check the length of what it is given instead of reading past its
  end, and has query families, `QueryType` with the `MS_qry_*` signatures
  and `get_query_fs()`, through which a caller reads data back from a Block
  it knows by name only; the forms taking an iterator stay, and are meant to
  go once the setters of every module take a span

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

- a group knows the type of the container it views, not only that of its
  elements, and `get_container_as< C >()` hands it back when it is a `C` and
  `nullptr` when it is not: that is what tells a `std::vector< T >` from a
  `boost::multi_array< T , 1 >`, which have the same elements, the same
  layout and the same rank, and it is what the typed accessors of `Block`
  ask instead of `boost::any_cast`

- A group says how to build a container of its own type and shape in another
  Block, which is what `AbstractBlock::mirror()` needed the `boost::any` for,
  and whoever allocates a container says how it goes, so that a Block
  disposes of what it owns through its own groups

- `std::vector< std::vector< Var > >` can be registered as a group of
  Variable, as it already could be as a group of Constraint: the two sides
  now have the same list of shapes

### Changed

- ⚠️ A `Variable` AND A `Constraint` POINT TO THEIR GROUP: the field that held
  the pointer to the Block holds the pointer to the group the element is in,
  the lowest bit telling the two apart, and `get_Block()` answers through the
  group; `get_Group()` gives the group, `nullptr` for an element in none. The
  Block sets it when it registers a group, when an element is added to a
  dynamic list it has registered, and takes it away when the group is
  replaced or reset and when the element is removed; `set_Block()` with the
  Block of the group leaves the element in it, any other Block takes it out,
  and a copy of a `Variable` has the Block of the original and no group.
  `inspection::get_element_index()` looks in the group of the element only.
  ⚠️ `f_Block` IS NO LONGER A PROTECTED FIELD OF `Variable` AND `Constraint`:
  a derived class reads `get_Block()`. ⚠️ A CONTAINER HAS TO BE THERE,
  POSSIBLY EMPTY, WHEN ITS GROUP IS REPLACED OR RESET, since the Block walks
  it to take its elements out of the group: clearing it first, as every
  `:Block` of the umbrella does, is fine, deleting it first is not

- ⚠️ THE FOUR `std::vector< boost::any >` OF `Block` ARE GONE, and so are the
  four vectors of the names beside them: a Block keeps its Variable and its
  Constraint in its four vectors of groups alone. `get_static_variables()`,
  `get_dynamic_variables()`, `get_static_constraints()`,
  `get_dynamic_constraints()` and the four `get_*_name()` that returned the
  whole vector of the names are gone with them, replaced by
  `get_static_variable_groups()` and its three fellows;
  `get_s_const_name( i )` and `get_s_const_index( name )`, and the six like
  them, stay and read the name of the group. The typed accessors,
  `get_static_variable< T >( i )` and the 23 like them, keep their signature
  and read the group instead of the `boost::any`, so their callers do not
  change. ⚠️ ONE OF THEM ASKED FOR THE WRONG TYPE NOW ANSWERS `nullptr`,
  WHICH IS WHAT THEIR DOCUMENTATION HAS ALWAYS PROMISED, INSTEAD OF THROWING
  `boost::bad_any_cast`: whoever was finding a mistake of type out of the
  exception now gets a null pointer, and finds it out later and elsewhere.
  `Vec_any`, `c_Vec_any` and `Vec_any_it` are gone from `SMSTypedefs.h`,
  which no longer includes `<boost/any.hpp>`

- `PolyhedralFunctionBlock::set_lambda()` is `set_size_variable()` with the
  checks it had, and adds the multiplier to the normalization constraint in
  place instead of giving the constraint a new LinearFunction, so that the
  constraint and its LinearFunction stay the objects they were

- ⚠️ THE LAYOUT OF `Block` HAS CHANGED, and `add_static_variable()` and the
  other 35 registration methods are templates, hence they live in the
  translation unit of whoever calls them: after updating, EVERYTHING has to
  be rebuilt, not only `libSMS++`, and a stale object file is not a
  compilation error but a group without the means to copy itself, or a
  library that is a hybrid of two layouts

- `GroupAdapter.h` is gone, having been the scaffolding that read the
  `boost::any` while the consumers of them were converted one at a time, and
  so are `Block::refresh_*_group()`, which existed to rebuild a group after
  it was written into through its `boost::any`, and which nobody calls

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

- `Observer::new_channel_name()`, when reusing a freed name, dereferenced
  `rend()` and erased the lowest free name rather than the one it handed
  out, which only shows when channels are closed out of order, as the
  grouped Modification of UCBlock do, and made the parallel tests of
  InvestmentBlock fail now and then with "Observer: wrong channel name"

- a `boost::multi_array` stored in the order of Fortran, or with indices not
  starting at 0, was accepted as a group and then read as if it were not:
  its cells were named after the wrong indices and its copy had another
  shape. Registering one now throws `std::invalid_argument`; no module
  registers one

- the unit tests check what they assert in every build type: the Release one
  defines NDEBUG, which turned each of their `assert()` into nothing, so that
  they passed whatever happened, and `AbstractPath_test` did not even walk
  the Block, the walk being inside an `assert()`; now that it does, it
  writes and reads back through netCDF one path in 8, many thousands to a
  file, rather than every path in a file of its own, which took 9 minutes

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

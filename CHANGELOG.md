# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

### Added 

### Changed 

### Fixed 

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

[Unreleased]: https://gitlab.com/smspp/smspp/-/compare/0.5.3...develop
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

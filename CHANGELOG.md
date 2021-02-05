# Changelog

All notable changes to this project will be documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.0.0/),
and this project adheres to [Semantic Versioning](https://semver.org/spec/v2.0.0.html).

## [Unreleased]

## [0.4.0] - 2021-02-05

### Added

- Some unit tests.

- Configuration header SMSppConfig.h.

- rather large changes in LagBFunction, added LagBFunctionMod

- added VariableMod::old_type() and supporting methods

- Cleaner style for un_any_thing macros

- implemented AbstractBlock::is_correct()

- significant rehaul of handling "stealth" obj variables addition in
  LagBFunction: Variable addition is now performed batch in compute()
  rather than real-time when dealing with Modification

- Added ColRowSolution

- significant improvements in load()-ing of Configurations,
  graciously terminating if the stream eof()-s

- added vectors in Configurations

- defined SMSpp\_classname\_normalise()

- LagBFunction now supports DQuadFunction Objective

- added InnrSlvr parameter in LagBFunction

- Updated DataMapping Dealing with the in which SetFrom is empty when
  producing error messages.

- ThinVarDepInterface now has get_Block()

- Added BoxSolver

- Updated serialization of BendersBFunction


### Fixed

- Compilation error in DataMapping with GCC

- Too many individual fixes to list

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

[Unreleased]: https://gitlab.com/smspp/smspp/-/compare/0.4.0...develop
[0.4.0]: https://gitlab.com/smspp/smspp/-/compare/0.3.2...0.4.0
[0.3.2]: https://gitlab.com/smspp/smspp/-/compare/0.3.1...0.3.2
[0.3.1]: https://gitlab.com/smspp/smspp/-/compare/0.3.0...0.3.1
[0.3.0]: https://gitlab.com/smspp/smspp/-/compare/0.2.0...0.3.0
[0.2.0]: https://gitlab.com/smspp/smspp/-/compare/0.1.1...0.2.0
[0.1.1]: https://gitlab.com/smspp/smspp/-/compare/0.1.0...0.1.1
[0.1.0]: https://gitlab.com/smspp/smspp/-/tags/0.1.0

# test

Unit tests for the core SMS++ library. Each test is registered as a separate
`ctest` target and exercises one of the core abstractions in isolation:

- `ClassFactory_test` checks the class factories used to construct `Block`,
  `Configuration`, `Solver`, `Solution` and `State` objects by name: each
  name is found, gives an object of the class asked for and of the same
  `classname()`, and a name that is not there throws.
- `Configuration_unit_test` covers `SimpleConfiguration`, `ComputeConfig` with
  its cascade override, `BlockConfig` and `OCRBlockConfig`, and
  `BlockSolverConfig`: read from text, written and read back in netCDF,
  cloned, got from a `Block` and applied to another, and cleared.
- `AbstractBlock_test` covers `Block` and `AbstractBlock`: the groups it
  registers, the LP, MPS and certificate writers, the netCDF round trip of
  an empty `Block`, of dynamic groups, of `OneVarConstraint`, of nested
  `Block` and of an `Objective` that is not linear (which is refused),
  `set_objective()` replacing an `Objective`, `mirror()` of an empty
  `Block`, and `read_lp()` on an empty model, on constants and bounds at
  the edges of the format and on malformed input.
- `ColVariable_test` covers `Variable` and `ColVariable`.
- `Constraint_unit_test` covers `FRowConstraint` with a `LinearFunction`, with
  the value of a row of every sense, the `RowConstraintMod` and
  `FRowConstraintMod` it issues and the registration in the `Variable` of
  its `Function`, and the `OneVarConstraint` family, with the sides each
  of them fixes and the setters it refuses.
- `LinearConstraint_unit_test` covers `LinearConstraint`, whose header no file
  of the library includes, so that the test is also what compiles it.
- `Function_test` covers the `Function` interface.
- `LinearFunction_test` covers `LinearFunction`: its value and
  linearization, the changes of its Variable and coefficients, and the
  `Modification` it issues from an `FRowConstraint` or an `FRealObjective`
  of a `Block`.
- `QuadFunction_test` covers `QuadFunction` and `DQuadFunction`, the latter
  both on its own and as the diagonal part of the former: value, gradient,
  Hessian, convexity, the changes of the coefficients and the `Modification`
  they issue.
- `C05SumFunction_test` covers `C05SumFunction`: the translation of the
  `Modification` of its members into its own, and its value and
  linearization as the sums of those of the members.
- `AbstractPath_test` covers `AbstractPath`, and in particular its edges:
  empty ranges and subsets on the last node, indices out of range, paths to
  `OneVarConstraint`, paths after a dynamic removal and an empty vector of
  paths through netCDF.
- `PolyhedralFunction_unit_test` covers `PolyhedralFunction`, i.e., its value
  and linearizations, the Modification each of its mutators issues, its
  `State` and its netCDF format, and the primal and dual abstract
  representations of a `PolyhedralFunctionBlock` kept in step with it.
- `SizeVariable_test` covers the size variable of a
  `PolyhedralFunctionBlock`, given before or after its abstract
  representation exists and kept in step by a change of the global scale.
- `Group_test` covers the groups of `Variable` and `Constraint` of a `Block`,
  and the two consumers that copy them, the `Solution` and the abstract copy
  of a `Block`; also grids of rank 3 and with a zero extent, indices and
  names after a dynamic removal, groups asked for out of range and walks of
  empty groups.
- `Modification_test` covers the `Modification` issued by the modifying
  methods of `ColVariable`, `FRowConstraint`, the `OneVarConstraint`
  family, `FRealObjective`, `LinearFunction` and the dynamic `Variable` and
  `Constraint` of a `Block`: whether the change is done and what is issued
  under each value of the `ModParam`, with or without a `Solver` listening
  and on a channel, and the type and content of the `Modification`.
- `BlockModification_unit_test` covers the Modification a `Block` issues when
  dynamic `Variable` and `Constraint` are added and removed, the parameter
  that says if, how and where a Modification is issued, the channels that
  pack them into a `GroupModification`, and the way they reach the `Solver`
  of the `Block` and of its ancestors.
- `Objective_unit_test` covers `Objective` and `FRealObjective`: the change of
  sense, the replacement of the `Function`, the Modification of the
  `Function` reaching the `Solver`, and the removal of its `Variable`.
- `Solution_test` covers what a `Solution` does when the dynamic
  `RowConstraint` of the `Block` it was read from are removed, i.e., the dual
  values it has to drop for whoever holds a dual solution to see whether what
  is left of it is still feasible; and the netCDF round trip of
  `RowConstraintSolution` and `ColRowSolution`, `clone()`, `scale()`,
  `sum()`, `is_direction()` through all of them, the refusal to be written
  into a `Block` of another shape, and the factory.
- `LagBFunction_unit_test` covers `LagBFunction` over a box inner Block
  solved by `BoxSolver`: an empty Lagrangian term, the term removed all at
  once and given again, the Modification these changes issue, the value and
  the linearizations against their closed form (on the kinks too), and the
  copy of the global pool a State holds; and the by-column representation
  of the Lagrangian term that the Lagrangian costs are computed from, as
  `get_A_by_col()` gives it, when the Lagrangian pairs are set (once or
  twice), added and removed (all of them, a `Range`, a `Subset`, a single
  one), and on the edge cases of those methods.
- `BendersBFunction_unit_test` covers `BendersBFunction` and `BendersBlock`: the
  rows added, modified and deleted with the Modification they issue, the
  sides written by `compute()` over a box inner Block solved by `BoxSolver`,
  and the serialization with the matrix in dense and sparse form.
- `Misc_unit_test` covers `GlobalInformation`, `SimpleDataMapping`, `Change` and
  `GroupChange`, `BoxSolver` (box LPs and QPs in both senses, empty boxes,
  unbounded directions, dual values), `UpdateSolver`, and what the base
  `Solver` does by itself: registration, the queue of Modification and the
  tables of the parameters.

These are built and run through CMake / ctest (there is no makefile here);
all of them passing is a good sign that no regressions have been introduced
in the SMS++ core.

The checks of a test are `assert()`, and they hold in every build type, the
Release one included: each test includes `TestAssert.h` after every header of
the library, which undefines NDEBUG for the test alone while the library
headers are read as the library was compiled.


## Authors

- **Donato Meoli**  
  Dipartimento di Informatica  
  Università di Pisa

- **Niccolò Iardella**  
  Dipartimento di Informatica  
  Università di Pisa

- **Rafael Durbano Lobato**  
  Dipartimento di Informatica  
  Università di Pisa

- **Wim van Ackooij**  
  EDF Lab Paris-Saclay


## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html),
see the [LICENSE](../LICENSE) file for details.

# test

Unit tests for the core SMS++ library. Each test is registered as a separate
`ctest` target and exercises one of the core abstractions in isolation:

- `ClassFactory_test` checks the class factory used to construct `Block`,
  `Solver` and `Configuration` objects by name.
- `AbstractBlock_test` covers `Block` and `AbstractBlock`.
- `ColVariable_test` covers `Variable` and `ColVariable`.
- `Function_test` covers the `Function` interface.
- `LinearFunction_test` covers `LinearFunction`.
- `QuadFunction_test` covers `DQuadFunction`.
- `AbstractPath_test` covers `AbstractPath`.
- `SizeVariable_test` covers the size variable of a
  `PolyhedralFunctionBlock`, given before or after its abstract
  representation exists and kept in step by a change of the global scale.
- `Group_test` covers the groups of `Variable` and `Constraint` of a `Block`,
  and the two consumers that copy them, the `Solution` and the abstract copy
  of a `Block`.

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

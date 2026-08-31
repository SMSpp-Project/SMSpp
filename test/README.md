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

These are built and run through CMake / ctest (there is no makefile here);
all of them passing is a good sign that no regressions have been introduced
in the SMS++ core.


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

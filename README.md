# SMS++

![To boldly model (and solve) what no one has modeled (and solved) before](doxygen/SMSpp_logo_mid_noback.png)

SMS++ is a set of C++ classes intended to provide a system for modeling
complex, block-structured mathematical models (in particular, but not
exclusively, single-real-objective optimization problems), and solving them
via sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

For further information on the SMS++ Project and the SMS++ core library,
you can see:

- The [SMS++ Project website](https://smspp.gitlab.io/)

- The [SMS++ API Reference](https://smspp.gitlab.io/smspp-project/)

- The [SMS++ Project Wiki](https://gitlab.com/smspp/smspp-project/-/wikis/home)

> **Note:** This is the repository of the *SMS++ core library*.
> If you are looking for the *SMS++ Project* repository, you will find it
> [here](https://gitlab.com/smspp/smspp-project).


## Getting started

These instructions will let you build and install SMS++ on your system.
If you encounter issues, see the [troubleshooting
section](https://gitlab.com/smspp/smspp-project/-/wikis/Troubleshooting).


### Requirements

- [Boost](https://www.boost.org)

- [Eigen](http://eigen.tuxfamily.org)

- [netCDF-C++](https://www.unidata.ucar.edu/software/netcdf)

For further details on software dependencies, see [this
page](https://gitlab.com/smspp/smspp-project/-/wikis/About-SMS++-requirements).
If you can't or wont install the required libraries, you will need to specify
their custom path, see [here](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration).


### Build and install with CMake

Configure and build the library with:

```sh
mkdir build
cd build
cmake ..
make
```

Some configuration options are available, see
[here](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration).

Optionally, install the library in the system with:

```sh
sudo make install
```

### Usage with CMake

After the library is built, you can use it in your CMake project with:

```cmake
find_package(SMS++)
target_link_libraries(<my_target> SMS++::SMS++)
```

### Running the tests with CMake

Some unit tests will be built with the library. Launch `ctest` from
the build directory to run them. To disable them, set the option
`BUILD_TESTING` to `OFF`.

> **Note:**
> The test `AbstractPath_test` may take 10 minutes or more.

### Build and install with makefiles

Carefully hand-crafted makefiles have also been developed for those unwilling
to use CMake. Makefiles build the executable in-source (in the same directory
tree where the code is) as opposed to out-of-source (in the copy of the
directory tree constructed in the build/ folder) and therefore it is more
convenient when having to recompile often, such as when developing/debugging
a new module, as opposed to the compile-and-forget usage envisioned by CMake.

A makefile for building the "core" SMS++ library in available in

```sh
SMS++/lib/makefile-lib
```

The makefile allow to choose the compiler name and the optimization/debug,
and builds the `lib/libSMS++` library that can be linked upon. This is
useful for compile-and-forget of a stable version of the "core" SMS++
library. Alternatively, the

```sh
SMS++/lib/makefile-inc
```

file is provided for allowing external makefiles to ensure that the library
is up-to-date, which is useful in case one is actually developing it. Each
executable using SMS++ will necessarily have to include one of those for
the "core" SMS++, plus that of the other modules required to model and solve
some specific optimization problems (the "core" SMS++ itself does not provide
a ton of useful `:Block` and `:Solver`, but it does provide the "plumbing"
for developing problem-specific ones). In fact, typically the makefiles for
the other modules already include those for the "core" SMS++ (but there is
a difference between `makefile-c` and `makefile-s`, check the individual
modules) so that one does not have to do it at the level of the "main
makefile". The simplest way to learn how to use it is to check e.g. the
makefiles of the modules in the [System Test
repository](https://gitlab.com/smspp/tests). The makefiles in turn
recursively include all the required other makefiles, hence one should
only need to edit the "main makefile" for compilation type (C++ compiler
and its options) and it all should be good to go. In case some of the
external libraries are not at their default location, it should only be
necessary to create the `../extlib/makefile-paths` out of the
`extlib/makefile-default-paths-*` for your OS `*` and edit the relevant
bits (commenting out all the rest).

Check the [SMS++ installation wiki](https://gitlab.com/smspp/smspp-project/-/wikis/Customize-the-configuration#location-of-required-libraries)
for further details.


## Getting help

If you need support, you want to submit bugs or propose a new feature, you can
[open a new issue](https://gitlab.com/smspp/smspp/-/issues/new).


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of
conduct, and the process for submitting merge requests to us.

## Authors

### Current Lead Authors

- **Antonio Frangioni**  
  Dipartimento di Informatica  
  Università di Pisa

- **Rafael Durbano Lobato**  
  Dipartimento di Informatica  
  Università di Pisa

### Previous Lead Authors and Contributors

- **Kostas Tavlaridis-Gyparakis**  
  Dipartimento di Informatica  
  Università di Pisa

- **Utz-Uwe Haus**  
  Cray EMEA Research Lab

- **Niccolò Iardella**  
  Dipartimento di Informatica  
  Università di Pisa


## License

This code is provided free of charge under the [GNU Lesser General Public
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html) -
see the [LICENSE](LICENSE) file for details.


## Disclaimer

The code is currently provided free of charge under an open-source license.
As such, it is provided "*as is*", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.


## Acknowledgements

For a list of contributions to the development of this module, see the
corresponding section in the
[SMS++ umbrella project](https://gitlab.com/smspp/smspp-project)

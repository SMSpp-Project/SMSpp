# SMS++

![To boldly model (and solve) what no one has modeled (and solved) before](doxygen/SMSpp_logo_mid_noback.png)

SMS++ is a set of C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them via
sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

For a more detailed description of SMS++ see
[the Wiki pages](https://gitlab.com/smspp/smspp/wikis/home).

> **Note:** This is the repository of the *SMS++ core library*.
> If you are looking for the *SMS++ Project*, you will find it
> [here](https://gitlab.com/smspp/smspp-project).


## Getting started

These instructions will let you build and install SMS++ on your system.
If you encounter issues, see the troubleshooting
section [here](https://gitlab.com/smspp/smspp/wikis/troubleshooting).


### Requirements

- [Boost](https://www.boost.org)

- [Eigen](http://eigen.tuxfamily.org)

- [netCDF-C++](https://www.unidata.ucar.edu/software/netcdf)

For further details on software dependencies, see
[this page](https://gitlab.com/smspp/smspp/wikis/requirements).
If you can't or wont install the required libraries, you will need to specify
their custom path, see [here](https://gitlab.com/smspp/smspp/wikis/custom).


### Build and install with CMake

Configure and build the library with:

```sh
mkdir build
cd build
cmake ..
make
```

Some configuration options are available, see
[here](https://gitlab.com/smspp/smspp/wikis/custom).

Optionally, install the library in the system with:

```sh
sudo make install
```


### Usage with CMake

After the library is built, you can use it in your CMake project with:

```cmake
find_package(SMSpp)
target_link_libraries(<my_target> SMS++::SMSpp)
```

### Running the tests with CMake

Some unit tests will be built with the library.
Launch `ctest` from the build directory to run them.
To disable them, set the option `BUILD_TESTING` to `OFF`.


### Build and install with makefiles

Carefully hand-crafted makefiles have also been developed for those unwilling
to use CMake. General instructions are:

- The arrangements of folders must be that envisioned by the
  [Umbrella SMS++ Project](https://gitlab.com/smspp/smspp-project)

- The main step is to edit the makefiles into ../extlib/. There is one for
  each of the external libraries that any module requires, starting with
  Boost, Eigen and netCDF-C++. Setting the

```make
lib*INC = -I<paths to include files directories>
lib*LIB = -L<paths to lib files directories> -l<libs>
```

  in each allows one to set any non-standard path if the library is not
  installed in the system (or leave them empty if they are).

- A makefile for building the "core" SMS++ library in available in

```sh
SMS++/lib/makefile-lib
```

  The makefile allow to choose the compiler name and the optimization/debug.
  This builds the lib/libSMS++.a that can be linked upon. Also, the

```sh
SMS++/lib/makefile-inc
```

  file is provided for allowing external makefiles to ensure that the library
  is up-to-date (useful in case one is actually developing it). The simplest
  way to learn how to use it is to check e.g. the makefiles of the tester

```sh
test/ClassFactory/makefile
```

  Note that the "basic" makefile macros

```make
CC =
SW =
```

  for setting the c++ compiler and its options are "automatically forwarded"
  from the makefile to these of the other SMS++ components, and therefore
  (possibly at the cost of a make clean) ensure consistency during the
  building process.


## Contributing

Please read [CONTRIBUTING.md](CONTRIBUTING.md) for details on our code of
conduct, and the process for submitting merge requests to us.

## Authors

### Current Lead Authors

- **Antonio Frangioni**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Rafael Durbano Lobato**  
  Department of Applied Mathematics  
  State University of Campinas, Brazil

### Previous Lead Authors and Contributors

- **Kostas Tavlaridis-Gyparakis**  
  *Operations Research Group*  
  Dipartimento di Informatica  
  Università di Pisa

- **Utz-Uwe Haus**  
  Cray EMEA Research Lab


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

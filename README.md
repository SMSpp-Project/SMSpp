# The "core" SMS++

![To boldly model (and solve) what no one has modeled (and solved) before](doxygen/SMSpp_logo_mid_noback.png)

SMS++ is a set of C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them via
sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

For a more detailed description of SMS++ see [the Wiki pages](https://gitlab.com/frangio68/sms_plus_plus/wikis/home).

## Getting started

These instructions will let you build and install SMS++ on your system.
If you encounter issues, see the troubleshooting section [here](https://gitlab.com/frangio68/sms_plus_plus/wikis/troubleshooting.md).

### Requirements

SMS++ requires a modern (at least C++-14 compliant) C++ compiler. It relies
on a few [Boost](https://www.boost.org) libraries, in particular:

- `boost::any` for vectors and lists of "any" kind of Constraint and
  Variable (although this may in the future be substituted with std::any)

- `boost::multi_array` for multi-dimensional arrays of stuff

- `boost::bind`, `boost::function`, `boost::functional::factory`, and
  `boost::functional::forward_adapter` for a factory construct

It also relies on [Eigen](http://eigen.tuxfamily.org) for sparse vectors and
matrices, although the use is so sparse right now (pun intended) that it is not
completely sure the dependency will be maintained.

Finally, it relies on the C++ interface to [netCDF](https://www.unidata.ucar.edu/software/netcdf)
for efficient serialization and de-serialization of Block and Configuration
objects on self-describing, machine-independent data files.

Optionally, SMS++ relies on [CMake](https://cmake.org) for building and installing.

### Build and install

- Clone the project from the repository and navigate inside its main directory.

- Configure the project with:
```sh
mkdir build
cd build
cmake ..
```

  If you can't or wont install the required libraries, you will need to specify
  their custom path, see [here](https://gitlab.com/frangio68/sms_plus_plus/wikis/custom.md).

- Build the library with:
```sh
make
```

- Optionally, you can install the library with:
```sh
sudo make install
```

## Usage

After the library is configured and built, you can use it in your CMake project with:
```cmake
find_package(SMSpp)
target_link_libraries(<my_target> SMS++::SMSpp)
```

## Running the tests

If `enable_testing()` is not commented in the customization file, some simple Google Tests will
be built with the library. You can run them with the following command from the `build` directory:
```sh
ctest
```
## Legal Stuff

### Standard Disclaimer

The code is currently provided free of charge for academic purposes only.
As such, it is provided "as is", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.

### License

This code is provided free of charge for academic purposes under the
"academic license": see the file[`doc/academicl.txt`](doc/academicl.txt) for further details. Because
the code is currently in the early stages of its development we prefer a
stricter licensing regime w.r.t. LGPL in order to discourage too early
fragmentation of the code. Thus, you can make changes, but we strongly suggest
that you do not distribute any modified version (although you are legally
allowed to within the terms of the license) without prior agreement with the
original developers; if your changes make good sense, please allow us to
incorporate them in the standard release. Yet, it is foreseen that the license
will be moved to LGPL as soon as the code is deemed stable enough to be widely
distributed.

## Version

- Current version of SMS++ is: 0.10

- Current date is:             14 - Nov - 2018

## Authors

### Current Lead Authors

	Antonio Frangioni
	Operations Research Group
	Dipartimento di Informatica
	Università di Pisa

	Rafael Durbano Lobato
 	Department of Applied Mathematics
	State University of Campinas, Brazil

### Previous Lead Authors and Contributors

	Kostas Tavlaridis-Gyparakis
	Operations Research Group
	Dipartimento di Informatica
	Università di Pisa

	Utz-Uwe Haus
	Cray EMEA Research Lab

# SMS++

![To boldly model (and solve) what no one has modeled (and solved) before](doxygen/SMSpp_logo_mid_noback.png)

SMS++ is a set of C++ classes intended to provide a system for modeling complex,
block-structured mathematical models (in particular, but not exclusively,
single-real-objective optimization problems), and solving them via
sophisticated, structure-exploiting algorithms (in particular, but not
exclusively, decomposition approaches and structured Interior-Point methods).

For a more detailed description of SMS++ see
[the Wiki pages](https://gitlab.com/smspp/smspp/wikis/home).

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

### Build and install

Configure and build the library with:
```sh
mkdir build
cd build
cmake ..
make
```

Optionally, install the library in the system with:
```sh
sudo make install
```

## Usage

After the library is configured and built, you can use it in your CMake project with:
```cmake
find_package(SMSpp)
target_link_libraries(<my_target> SMS++::SMSpp)
```

### Tools and examples

We provide some tools and examples in the [`tools`](tools) directory.
See [`tools/README.md`](tools/README.md) for details on how to build and use them.

## Running the tests

By default, some simple unit tests are built with the library.
To run them, launch `ctest` from the build directory.

## Contributing

This section is not ready yet (see license below).

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

## Disclaimer

The code is currently provided free of charge for academic purposes only.
As such, it is provided "*as is*", without any explicit or implicit warranty
that it will properly behave or it will suit your needs. The Authors of
the code cannot be considered liable, either directly or indirectly, for
any damage or loss that anybody could suffer for having used it. More
details about the non-warranty attached to this code are available in the
license description file.

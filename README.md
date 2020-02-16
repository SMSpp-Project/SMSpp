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

## Running the tests

Some simple unit tests will be built with the library,
to run them, launch `ctest` from the build directory.
To disable them, configure the library with the option `-DBUILD_TESTING=OFF`.


## Contributing

This section is not ready yet.


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
License version 3.0](https://opensource.org/licenses/lgpl-3.0.html).

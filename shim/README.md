# shim

The code that the build inserts into its private copies of the headers of
Boost, written once and in C++ rather than inside the build files.

Boost is taken from the platform, and the build rewrites a private copy of
one of its headers, which it puts on the include path before the system
ones; both the CMake build and the makefiles insert the code at the same
anchor, so that the two agree by construction:

| file | anchor | what it is |
|---|---|---|
| `stable_vector_splice.inc` | the documentation of `swap()` in `boost/container/stable_vector.hpp` | the three `splice()` of `std::list` for `boost::container::stable_vector`, i.e., all of another stable_vector, one element of it or a range of it are moved before a position, the nodes passing from one index to the other without being copied, moved or destroyed, so that the addresses of the elements do not change |

The anchor is a line of Boost as it is released, the same from 1.72 to
1.90: if a new version of Boost moves it, the build finds nothing to
replace. `markers.txt` lists what the rewritten header has to contain, and
both builds check it and stop, naming the text that is missing, rather than
go on with a stable_vector that has no `splice()`.

The rewrite is skipped on a Boost whose stable_vector already has a
`splice()`, so that the shim does nothing once Boost has the feature.

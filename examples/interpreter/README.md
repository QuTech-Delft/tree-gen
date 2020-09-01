# Directory tree example

This example illustrates the tree system with a Windows-like directory tree
structure. The `System` root node consists of one or more drives, each of which
has a drive letter and a root directory with named files and subdirectories. To
make things a little more interesting, a symlink-like "mount" is added as a file
type, which links to another directory.

Refer to the comments in `main.cpp` or the ReadTheDocs page generated from this
example and its output for more information.

## Build instructions

You can build and run this example by means of the usual

    mkdir build
    cd build
    cmake ..
    cmake -B .
    ctest

from this directory.

It will also be built and run by the root CMakeLists if tests are enabled using
`-DTREE_GEN_BUILD_TESTS=ON`, its few assert statements serving as a rudimentary
test.

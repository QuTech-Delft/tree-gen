# Directory tree example

This example illustrates the tree system with a Windows-like directory tree
structure. The `System` root node consists of one or more drives, each of which
has a drive letter and a root directory with named files and subdirectories. To
make things a little more interesting, a symlink-like "mount" is added as a file
type, which links to another directory.

Together with the program's stdout, `main.cpp` is written more or less like a
tutorial to show off some of the features of tree-gen. The `CMakeLists.txt` file
shows how to use tree-gen in your own project.

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

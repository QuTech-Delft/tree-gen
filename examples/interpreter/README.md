# Interpreter example

This example discusses some of the more advanced features of tree-gen, using an
interpreter for a very simple language as an example. This is, however, not a
completely integrated or even functional interpreter; we just go over some of
the concepts and things you may run into when you would make such an
interpreter.

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

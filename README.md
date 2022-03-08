# tree-gen

[![Documentation Status](https://readthedocs.org/projects/tree-gen/badge/?version=latest)](https://tree-gen.readthedocs.io/en/latest)
[![GitHub Actions Build Status](https://github.com/QuTech-Delft/tree-gen/workflows/Test/badge.svg)](https://github.com/qutech-delft/tree-gen/actions)

`tree-gen` is a C++ and Python code generator for tree-like structures common in
parser and compiler codebases. The tree is described using a simple and concise
language, so all the repetitive C++ and Python stuff can be generated. This
includes serialization and deserialization to [CBOR](https://cbor.io/), allowing
easy interoperation between C++ and Python.

## Usage and "installation"

`tree-gen` is a very lightweight tool, intended to be compiled along with your
project rather than to actually be installed. The assumption is that your main
project will be written in C++ and that you're using CMake. If these assumptions
are invalid, you'll have to do some extra work on your own. If they *are* valid,
however, it should be really easy. Just include `tree-gen` as a submodule or
copy it into your project, then do something like

```cmake
add_subdirectory(tree-gen)

# To generate the files from your tree description file:
generate_tree_py(
    "${CMAKE_CURRENT_SOURCE_DIR}/input-tree-specification.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-header-file.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-source-file.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-python-file.py"
)

# To add generated-source-file.cpp to your program:
add_executable/add_library(
    my-software
    "${CMAKE_CURRENT_BINARY_DIR}/generated-source-file.cpp"
)

# To add generated-header-file.hpp to the search path:
target_include_directories(
    my-software
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

# To use tree-gen's support library (you could substitute your own if you feel
# up to the task):
target_link_libraries(my-software tree-lib)
```

and CMake *Should*™ handle everything for you.

`tree-gen` does have some dependencies:

 - A compiler with C++11 support (MSVC, GCC, and Clang are tested in CI);
 - Flex 2.6.1+
 - Bison 3.0+

If you're on a POSIX system and Flex/Bison are too old or not installed, the
buildsystem will attempt to download and build them from source for you.

For usage information beyond this, [Read The Docs](https://tree-gen.readthedocs.io/).

## Project structure

 - `generator`: source and private header files for the generator program.
 - `src`: source and private header files for the support library.
 - `include`: public header files for the support library.
 - `cmake`: contains CMake helper modules for building flex/bison from source
   if they are not installed.
 - `examples`: examples showing how to use `tree-gen`. These are also used as
   tests, and their content and output is directly used to generate the
   ReadTheDocs documentation.
 - `tests`: additional test cases for things internal to `tree-gen`.
 - `doc`: documentation generation for ReadTheDocs.

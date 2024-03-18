# tree-gen

[![Documentation Status](https://readthedocs.org/projects/tree-gen/badge/?version=latest)](https://tree-gen.readthedocs.io/en/latest)
[![GitHub Actions Build Status](https://github.com/QuTech-Delft/tree-gen/workflows/Test/badge.svg)](https://github.com/qutech-delft/tree-gen/actions)

`tree-gen` is a C++ and Python code generator for tree-like structures common in parser and compiler codebases.
The tree is described using a simple and concise language, so all the repetitive C++ and Python stuff can be generated.
This includes serialization and deserialization to [CBOR](https://cbor.io/),
allowing easy interoperation between C++ and Python.

## File organization

 - `generator`: source and private header files for the generator program.
 - `src`: source and private header files for the support library.
 - `include`: public header files for the support library.
 - `examples`: examples showing how to use `tree-gen`. These are also used as tests,
   and their content and output is directly used to generate the ReadTheDocs documentation.
 - `tests`: additional test cases for things internal to `tree-gen`.
 - `doc`: documentation generation for ReadTheDocs.

## Dependencies

* C++ compiler with C++17 support (gcc 11, clang 14, msvc 17)
* `CMake` >= 3.12
* `git`
* `Python` 3.x plus `pip`, with the following package:
  * `conan` >= 2.0
  
### ARM specific dependencies

We are having problems when using the `m4` and `zulu-opendjk` Conan packages on an ARMv8 architecture.
`m4` is required by Flex/Bison and `zulu-openjdk` provides the Java JRE required by the ANTLR generator.
So, for the time being, we are installing Flex/Bison and Java manually for this platform.

* `Flex` >= 2.6.4
* `Bison` >= 3.0
* `Java JRE` >= 11

## Usage and "installation"

`tree-gen` is a very lightweight tool, intended to be requested as a Conan package from a C++ project.

```python
def requirements(self):
    self.requires("tree-gen/1.0.7")
```

And then used something like:

```cmake
find_program(TREE_GEN_EXECUTABLE tree-gen REQUIRED)

# To generate the files from your tree description file:
generate_tree_py(
    "${TREE_GEN_EXECUTABLE}"
    "${CMAKE_CURRENT_SOURCE_DIR}/input-tree-specification.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-header-file.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-source-file.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/generated-python-file.py"
)

# To add generated-source-file.cpp to your program:
add_executable/add_library(my-software "${CMAKE_CURRENT_BINARY_DIR}/generated-source-file.cpp")

# To add generated-header-file.hpp to the search path:
target_include_directories(my-software PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")

target_link_libraries(my-software PRIVATE tree-gen::tree-gen)
```

and CMake *Should*™ handle everything for you.

For usage information beyond this, [Read The Docs](https://tree-gen.readthedocs.io/).

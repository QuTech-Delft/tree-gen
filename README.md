# tree-gen

Tree-gen is a code generator for tree-like structures common in parser and
compiler codebases. The tree is described using a simple and concise language,
so all the repetitive C++ stuff can be generated.

This is torn out of the new API for libqasm to be turned into a submodule, so
it can be reused more sanely by other projects in the future (notably OpenQL).
Until it is merged back into libqasm, this repo should be considered a work in
progress.

## File list

 - `generator`: source and private header files for the generator program.

 - `src`: source and private header files for the support library.

 - `include`: public header files for the support library.

 - `cmake`: contains CMake helper modules for building flex/bison from source
   if they are not installed.

 - `doc`: documentation generation for ReadTheDocs.

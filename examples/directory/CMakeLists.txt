cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(directory-example CXX)

# Add the tree-gen repository root directory. Normally, your project would be
# located outside of tree-gen's root, which means you can omit the second
# argument.
add_subdirectory(../.. tree-gen)

# Generates the files for the directory tree.
generate_tree_py(
    "${CMAKE_CURRENT_SOURCE_DIR}/directory.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/directory.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/directory.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/directory.py"
)

add_executable(
    directory-example
    "${CMAKE_CURRENT_BINARY_DIR}/directory.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_include_directories(
    directory-example
    # This directory for primitives.hpp:
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    # Binary directory for directory.hpp:
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(directory-example tree-lib)

# The following lines only serve to register the example as a test, so you can
# run it using `make test` or `ctest` as well. You don't need them as such in
# your own project.
enable_testing()
add_test(directory-example directory-example)

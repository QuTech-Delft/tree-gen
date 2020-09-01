cmake_minimum_required(VERSION 3.1 FATAL_ERROR)

project(interpreter-example CXX)

# Add the tree-gen repository root directory. Normally, your project would be
# located outside of tree-gen's root, which means you can omit the second
# argument.
add_subdirectory(../.. tree-gen)

# Generates the files for the value tree.
generate_tree(
    "${CMAKE_CURRENT_SOURCE_DIR}/value.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/value.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/value.cpp"
)

# Generates the files for the program tree.
generate_tree(
    "${CMAKE_CURRENT_SOURCE_DIR}/program.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/program.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/program.cpp"
)

add_executable(
    interpreter-example
    "${CMAKE_CURRENT_BINARY_DIR}/value.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/program.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_include_directories(
    interpreter-example
    # This directory for primitives.hpp:
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"
    # Binary directory for directory.hpp:
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(interpreter-example tree-lib)

# The following lines only serve to register the example as a test, so you can
# run it using `make test` or `ctest` as well. You don't need them as such in
# your own project.
enable_testing()
add_test(interpreter-example interpreter-example)

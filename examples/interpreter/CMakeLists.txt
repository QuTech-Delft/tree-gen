cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

project(interpreter-example CXX)

include(../../cmake/generate_tree.cmake)

# Generates the files for the value tree
generate_tree_py(
    tree-gen
    "${CMAKE_CURRENT_SOURCE_DIR}/value.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/value.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/value.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/value.py"
)

# Generates the files for the program tree
generate_tree_py(
    tree-gen
    "${CMAKE_CURRENT_SOURCE_DIR}/program.tree"
    "${CMAKE_CURRENT_BINARY_DIR}/program.hpp"
    "${CMAKE_CURRENT_BINARY_DIR}/program.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/program.py"
)

add_executable(interpreter-example
    "${CMAKE_CURRENT_BINARY_DIR}/value.cpp"
    "${CMAKE_CURRENT_BINARY_DIR}/program.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
)

target_include_directories(
    interpreter-example
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}"  # Current directory for primitives.hpp
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"  # Binary directory for interpreter.hpp
)

target_link_libraries(interpreter-example
    PRIVATE tree-gen-lib
)

# The following lines only serve to register the example as a test,
# so you can run it using `make test` or `ctest` as well
# You don't need them as such in your own project
enable_testing()
add_test(interpreter-example interpreter-example)

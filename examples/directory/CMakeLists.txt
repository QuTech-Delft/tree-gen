cmake_minimum_required(VERSION 2.8.12 FATAL_ERROR)

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
add_test(
    NAME directory-example
    COMMAND directory-example
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
)

# Only add the Python test if CMake is new enough for us to not have to bother
# with FindPythonInterp.
if(NOT ${CMAKE_VERSION} VERSION_LESS "3.12")
    find_package(Python3 COMPONENTS Interpreter)
    if(${Python3_FOUND})
        add_test(
            NAME directory-example-py
            COMMAND ${Python3_EXECUTABLE} main.py ${CMAKE_CURRENT_BINARY_DIR}
            WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        )
    endif()
endif()

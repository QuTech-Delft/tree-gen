cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Convenience function to add a test.
function(add_tree_lib_test name source workdir)
    add_executable("${name}" "${CMAKE_CURRENT_SOURCE_DIR}/${source}")
    target_link_libraries("${name}" tree-lib)
    add_test(
        NAME "${name}"
        WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${workdir}"
        COMMAND "${name}"
    )
endfunction()

add_tree_lib_test(test-cbor test-cbor.cpp .)
add_tree_lib_test(test-annotatable test-annotatable.cpp .)

# Utility function for generating a C++-only tree with tree-gen.
function(generate_tree TREE_GEN_EXECUTABLE TREE HDR SRC)
    # Get the directory for the header file and make sure it exists.
    get_filename_component(HDR_DIR "${HDR}" PATH)
    file(MAKE_DIRECTORY "${HDR_DIR}")

    # Get the directory for the source file and make sure it exists.
    get_filename_component(SRC_DIR "${SRC}" PATH)
    file(MAKE_DIRECTORY "${SRC_DIR}")

    # Add a command to do the generation.
    add_custom_command(
        COMMAND "${TREE_GEN_EXECUTABLE}" "${TREE}" "${HDR}" "${SRC}"
        OUTPUT "${HDR}" "${SRC}"
        DEPENDS "${TREE}" "${TREE_GEN_EXECUTABLE}"
    )
endfunction()

# Utility function for generating a C++ and Python tree with tree-gen.
function(generate_tree_py TREE_GEN_EXECUTABLE TREE HDR SRC PY)
    # Get the directory for the header file and make sure it exists.
    get_filename_component(HDR_DIR "${HDR}" PATH)
    file(MAKE_DIRECTORY "${HDR_DIR}")

    # Get the directory for the source file and make sure it exists.
    get_filename_component(SRC_DIR "${SRC}" PATH)
    file(MAKE_DIRECTORY "${SRC_DIR}")

    # Get the directory for the python file and make sure it exists.
    get_filename_component(PY_DIR "${PY}" PATH)
    file(MAKE_DIRECTORY "${PY_DIR}")

    # Add a command to do the generation.
    add_custom_command(
        COMMAND "${TREE_GEN_EXECUTABLE}" "${TREE}" "${HDR}" "${SRC}" "${PY}"
        OUTPUT "${HDR}" "${SRC}" "${PY}"
        DEPENDS "${TREE}" "${TREE_GEN_EXECUTABLE}"
    )
endfunction()
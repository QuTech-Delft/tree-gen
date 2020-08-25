cmake_minimum_required(VERSION 2.8.11 FATAL_ERROR)
if(CMAKE_VERSION VERSION_GREATER 3.0)
    CMAKE_POLICY(SET CMP0048 NEW)
endif()

project(tree-gen CXX)

# If tree-gen was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET tree-gen)

# Require C++11 compiler support.
if(CMAKE_VERSION VERSION_LESS "3.1")
    if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=gnu++11")
    endif()
else()
    set(CMAKE_CXX_STANDARD 11)
    set(CMAKE_CXX_STANDARD_REQUIRED ON)
    set(CMAKE_CXX_EXTENSIONS ON)
endif()

# Require flex/bison; if not installed, this will try to build from source.
set(FLEX_VERSION_REQUIRED 2.6.1)
set(BISON_VERSION_REQUIRED 3.0)
include(cmake/flex-bison.cmake)

# Generate the lexer.
if(WIN32)
    set(flex_win_compat --wincompat)
else()
    set(flex_win_compat)
endif()
flex_target(
    tree-gen-lexer
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/lexer.l"
    "${CMAKE_CURRENT_BINARY_DIR}/lexer.cpp"
    COMPILE_FLAGS
        "--header-file=${CMAKE_CURRENT_BINARY_DIR}/lexer.hpp ${flex_win_compat}"
)

# Generate the parser.
bison_target(
    tree-gen-parser
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/parser.y"
    "${CMAKE_CURRENT_BINARY_DIR}/parser.cpp"
    COMPILE_FLAGS
        "-rall --report-file=${CMAKE_CURRENT_BINARY_DIR}/parser.txt -Werror=conflicts-sr -Werror=conflicts-rr"
)

# Dependency between lexer and parser.
add_flex_bison_dependency(
    tree-gen-lexer
    tree-gen-parser
)

# Add rules for the tree-gen executable.
add_executable(
    tree-gen
    ${BISON_tree-gen-parser_OUTPUTS}
    ${FLEX_tree-gen-lexer_OUTPUTS}
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen.cpp"
)

target_include_directories(
    tree-gen
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/generator"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

# Utility function for generating a tree with tree-gen.
function(generate_tree TREE HDR SRC)

    # Get the directory for the header file and make sure it exists.
    get_filename_component(HDR_DIR "${HDR}" PATH)
    file(MAKE_DIRECTORY "${HDR_DIR}")

    # Get the directory for the source file and make sure it exists.
    get_filename_component(SRC_DIR "${SRC}" PATH)
    file(MAKE_DIRECTORY "${SRC_DIR}")

    # Add a command to do the generation.
    add_custom_command(
        COMMAND tree-gen "${TREE}" "${HDR}" "${SRC}"
        OUTPUT "${HDR}" "${SRC}"
        DEPENDS "${TREE}" tree-gen
    )

endfunction()

# Install target.
if (${CMAKE_PROJECT_NAME} STREQUAL tree-gen)
    include(GNUInstallDirs)
    install(
        TARGETS tree-gen
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.hpp"
    )
endif()

endif() # NOT TARGET tree-gen

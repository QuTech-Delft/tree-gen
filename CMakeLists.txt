cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(tree-gen CXX)

# If tree-gen was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET tree-gen)

# Loads up the appropriate directories for installing stuff.
include(GNUInstallDirs)


#=============================================================================#
# Configuration options                                                       #
#=============================================================================#

# Library type option. Default is a shared object, because for CMake it doesn't
# matter, but outside of CMake dependency information is lost for static
# libraries. That requires the user to link all of ql's direct and transitive
# dependencies as well, which is terribly ugly. setup.py *has* to do this
# however, because "pip install ." builds this in a temporary directory, so the
# shared objects that get built and installed and are then depended on by the
# Python lib get deleted by pip after the install.
option(
    BUILD_SHARED_LIBS
    "Whether libraries should be built as a shared object or as a static library"
    OFF
)

# Whether tests should be built.
option(
    TREE_GEN_BUILD_TESTS
    "Whether the tests should be built and added to `make test`"
    OFF
)


#=============================================================================#
# CMake weirdness and compatibility                                           #
#=============================================================================#

# On Windows builds, CMake complains that the CMake internal variable
# "CMAKE_MODULE_LINKER_FLAGS_MAINTAINER" is not defined *the first time you
# configure*. Weirdly, all is good with the world if you then just try again.
# It seems to have to do with the "maintainer" build type in MSVC, but there
# is no documentation whatsoever. In any case, this just mimics what CMake
# does automatically the second time you invoke it, so it works around the
# issue.
if(NOT DEFINED CMAKE_MODULE_LINKER_FLAGS_MAINTAINER)
    set(
        CMAKE_MODULE_LINKER_FLAGS_MAINTAINER ""
        CACHE STRING "Flags used by the linker during the creation of modules during MAINTAINER builds."
    )
endif()


#=============================================================================#
# Global build configuration                                                  #
#=============================================================================#

# Since we have multiple libraries to link together, we unfortunately have to
# worry about RPATH handling on Linux and OSX. See
# https://gitlab.kitware.com/cmake/community/-/wikis/doc/cmake/RPATH-handling
set(CMAKE_SKIP_BUILD_RPATH FALSE)
set(CMAKE_BUILD_WITH_INSTALL_RPATH FALSE)
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_FULL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)

# Everything needs C++11 support.
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Windows weirdness: need a .lib file to link against a DLL at compile-time
# (I think), but only the .dll is generated when there are no exported symbols.
# This sort of fixes that (https://stackoverflow.com/questions/1941443)
SET(CMAKE_WINDOWS_EXPORT_ALL_SYMBOLS ON)


#=============================================================================#
# Package dependencies                                                        #
#=============================================================================#
include(FetchContent)

# Require flex/bison; if not installed, this will try to build from source.
set(FLEX_VERSION_REQUIRED 2.6.1)
set(BISON_VERSION_REQUIRED 3.0)
set(BISON_VERSION_TO_BUILD 3.8)
include(cmake/flex-bison.cmake)

# fmt 10.1.1
FetchContent_Declare(fmt
    GIT_REPOSITORY https://github.com/fmtlib/fmt.git
    GIT_TAG "f5e54359df4c26b6230fc61d38aa294581393084"
)
FetchContent_MakeAvailable(fmt)


#=============================================================================#
# tree-lib support library target                                             #
#=============================================================================#

# Create the library.
set(
    TREE_LIB_SRCS
    "${CMAKE_CURRENT_SOURCE_DIR}/src/tree-all.cpp"
)
set(TREE_LIB_SRCS ${TREE_LIB_SRCS} PARENT_SCOPE)
add_library(tree-lib-obj OBJECT ${TREE_LIB_SRCS})
set_property(TARGET tree-lib-obj PROPERTY POSITION_INDEPENDENT_CODE ON)

# tree-lib has a global, which (thanks to MSVC declspec nonsense) requires a
# header file to be different when the library is compiled compared to when it
# is used.
set(TREE_LIB_PRIVATE_DEFS BUILDING_TREE_LIB)
set(TREE_LIB_PRIVATE_DEFS ${TREE_LIB_PRIVATE_DEFS} PARENT_SCOPE)
target_compile_definitions(tree-lib-obj PRIVATE ${TREE_LIB_PRIVATE_DEFS})

# Add the appropriate include paths.
set(TREE_LIB_PRIVATE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set(TREE_LIB_PRIVATE_INCLUDE ${TREE_LIB_PRIVATE_INCLUDE} PARENT_SCOPE)
set(TREE_LIB_PUBLIC_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include/")
set(TREE_LIB_PUBLIC_INCLUDE ${TREE_LIB_PUBLIC_INCLUDE} PARENT_SCOPE)
target_include_directories(
    tree-lib-obj
    PRIVATE ${TREE_LIB_PRIVATE_INCLUDE}
    PUBLIC ${TREE_LIB_PUBLIC_INCLUDE}
)
target_link_libraries(tree-lib-obj PUBLIC fmt::fmt)

# Main tree-gen library in shared or static form as managed by cmake's
# internal BUILD_SHARED_LIBS variable.
add_library(tree-lib $<TARGET_OBJECTS:tree-lib-obj>)
target_include_directories(tree-lib PUBLIC $<TARGET_PROPERTY:tree-lib-obj,INTERFACE_INCLUDE_DIRECTORIES>)
target_link_libraries(tree-lib PUBLIC $<TARGET_PROPERTY:tree-lib-obj,LINK_LIBRARIES>)


#=============================================================================#
# tree-gen code generator tool                                                #
#=============================================================================#

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
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen-cpp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen-python.cpp"
)
if(MSVC)
    target_compile_options(tree-gen PRIVATE
        /D_CRT_NONSTDC_NO_DEPRECATE
        /D_CRT_SECURE_NO_WARNINGS
    )
endif()

target_include_directories(
    tree-gen
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/generator"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(tree-gen PRIVATE fmt::fmt)

# Utility function for generating a C++-only tree with tree-gen.
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

# Utility function for generating a C++ and Python tree with tree-gen.
function(generate_tree_py TREE HDR SRC PY)

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
        COMMAND tree-gen "${TREE}" "${HDR}" "${SRC}" "${PY}"
        OUTPUT "${HDR}" "${SRC}" "${PY}"
        DEPENDS "${TREE}" tree-gen
    )

endfunction()


#=============================================================================#
# Testing                                                                     #
#=============================================================================#

# Include the tests directory if requested.
if(TREE_GEN_BUILD_TESTS)
    enable_testing()

    # Include the directories containing tests.
    add_subdirectory(tests)
    add_subdirectory(examples)

endif()


#=============================================================================#
# Installation                                                                #
#=============================================================================#

# Install the support library.
install(
    TARGETS tree-lib
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
    DIRECTORY "$<TARGET_PROPERTY:tree-lib,INTERFACE_INCLUDE_DIRECTORIES>"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp"
)

# Install the generator tool only if this is the toplevel project.
if (${CMAKE_PROJECT_NAME} STREQUAL tree-gen)
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

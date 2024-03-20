cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

if(POLICY CMP0048)
    cmake_policy(SET CMP0048 NEW)
endif()

project(tree-gen LANGUAGES CXX)

# If tree-gen was already included elsewhere in the project, don't include it
# again. There should be only one place for it and one version per project.
if(NOT TARGET tree-gen)

# Loads up the appropriate directories for installing stuff.
include(GNUInstallDirs)

# Include the generate_tree functions
include(cmake/generate_tree.cmake)


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

# Everything needs C++17 support.
set(CMAKE_CXX_STANDARD 17)
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

find_package(BISON 3.0)
find_package(FLEX 2.6.4)
if(WIN32)
    set(flex_win_compat --wincompat)
else()
    set(flex_win_compat)
endif()

# fmt
find_package(fmt)
if(NOT fmt_FOUND)
    message(STATUS "Fetching fmt")
    FetchContent_Declare(fmt
        GIT_REPOSITORY https://github.com/fmtlib/fmt.git
        GIT_TAG "e69e5f977d458f2650bb346dadf2ad30c5320281"
    )
    FetchContent_MakeAvailable(fmt)
endif()

# range-v3
find_package(range-v3)
if(NOT range-v3_FOUND)
    message(STATUS "Fetching range-v3")
    FetchContent_Declare(range-v3
        GIT_REPOSITORY https://github.com/ericniebler/range-v3
        GIT_TAG "a81477931a8aa2ad025c6bda0609f38e09e4d7ec"
    )
    FetchContent_MakeAvailable(range-v3)
endif()


#=============================================================================#
# tree-gen-lib support library target                                         #
#=============================================================================#

# Create the library.
set(TREE_GEN_LIB_SRCS "${CMAKE_CURRENT_SOURCE_DIR}/src/tree-all.cpp")

add_library(tree-gen-lib-obj OBJECT
    ${TREE_GEN_LIB_SRCS}
)
set_target_properties(tree-gen-lib-obj
    PROPERTIES POSITION_INDEPENDENT_CODE ON
)

# tree-gen-lib has a global, which (thanks to MSVC declspec nonsense) requires a
# header file to be different when the library is compiled compared to when it
# is used.
set(TREE_GEN_LIB_PRIVATE_DEFS BUILDING_TREE_GEN_LIB)
target_compile_definitions(tree-gen-lib-obj PRIVATE ${TREE_GEN_LIB_PRIVATE_DEFS})

# Add the appropriate include paths.
set(TREE_GEN_LIB_PRIVATE_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set(TREE_GEN_LIB_PUBLIC_INCLUDE "${CMAKE_CURRENT_SOURCE_DIR}/include/")
target_include_directories(
    tree-gen-lib-obj
    PRIVATE ${TREE_GEN_LIB_PRIVATE_INCLUDE}
    PUBLIC ${TREE_GEN_LIB_PUBLIC_INCLUDE}
)
target_link_libraries(tree-gen-lib-obj
    PRIVATE fmt::fmt
    PRIVATE range-v3::range-v3
)

# Set variables at possible parent projects.
get_directory_property(hasParent PARENT_DIRECTORY)
if(hasParent)
    set(TREE_GEN_LIB_SRCS ${TREE_GEN_LIB_SRCS} PARENT_SCOPE)
    set(TREE_GEN_LIB_PRIVATE_DEFS ${TREE_GEN_LIB_PRIVATE_DEFS} PARENT_SCOPE)
    set(TREE_GEN_LIB_PRIVATE_INCLUDE ${TREE_GEN_LIB_PRIVATE_INCLUDE} PARENT_SCOPE)
    set(TREE_GEN_LIB_PUBLIC_INCLUDE ${TREE_GEN_LIB_PUBLIC_INCLUDE} PARENT_SCOPE)
endif()

# Main tree-gen library in shared or static form as managed by CMake's internal BUILD_SHARED_LIBS variable.
add_library(tree-gen-lib
    $<TARGET_OBJECTS:tree-gen-lib-obj>
)
target_include_directories(tree-gen-lib PUBLIC
    $<TARGET_PROPERTY:tree-gen-lib-obj,INTERFACE_INCLUDE_DIRECTORIES>
)
target_link_libraries(tree-gen-lib PUBLIC
    $<TARGET_PROPERTY:tree-gen-lib-obj,LINK_LIBRARIES>
)
set_target_properties(tree-gen-lib
    PROPERTIES OUTPUT_NAME "tree-gen"
)

# Address sanitizer
if(ASAN_ENABLED)
    if(MSVC AND CMAKE_BUILD_TYPE STREQUAL "Debug")
        string(REGEX REPLACE "/RTC(su|[1su])" "" CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
        message(STATUS "CMAKE_CXX_FLAGS: ${CMAKE_CXX_FLAGS}\n")

        # /MTd is needed to link clang_rt.asan-i386.lib statically
        # Otherwise the path to clang_rt.asan-i386.dll should be provided
        add_compile_options(/fsanitize=address /MTd)
    elseif(${CMAKE_CXX_COMPILER_ID} STREQUAL "Clang" OR ${CMAKE_CXX_COMPILER_ID} STREQUAL "GNU")
        add_compile_options(-fsanitize=address,undefined -fno-omit-frame-pointer)
        add_link_options(-fsanitize=address,undefined)
    endif()
endif()


#=============================================================================#
# tree-gen code generator tool                                                #
#=============================================================================#

# Generate the lexer.
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
add_executable(tree-gen
    ${BISON_tree-gen-parser_OUTPUTS}
    ${FLEX_tree-gen-lexer_OUTPUTS}
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/format_utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen-cpp.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/generator/tree-gen-python.cpp"
)

target_include_directories(tree-gen
    PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/generator"
    PRIVATE "${CMAKE_CURRENT_BINARY_DIR}"
)

target_link_libraries(tree-gen
    PRIVATE fmt::fmt
    PRIVATE range-v3::range-v3
)

if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(tree-gen PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -fPIC
        -Wno-error=deprecated-declarations
        -Wno-error=restrict
        -Wno-error=sign-compare
    )
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(tree-gen PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -fPIC
        -Wno-error=sign-compare
        -Wno-error=unused
    )
elseif(MSVC)
    target_compile_options(tree-gen PRIVATE
        /W3 /w34996  # not using /WX at the moment because the flex generated code has some warnings
        /D_CRT_NONSTDC_NO_DEPRECATE
        /D_CRT_SECURE_NO_WARNINGS
        /D_UNICODE /DUNICODE
        /diagnostics:column /EHsc /FC /fp:precise /Gd /GS /MP /sdl /utf-8 /Zc:inline
    )
else()
    message(SEND_ERROR "Unknown compiler!")
endif()


#=============================================================================#
# Testing                                                                     #
#=============================================================================#

# Include the tests directory if requested
if(TREE_GEN_BUILD_TESTS)
    enable_testing()
    include(CTest)
    set(CMAKE_CTEST_ARGUMENTS "--output-on-failure")

    add_subdirectory(test)
    add_subdirectory(examples)
endif()


#=============================================================================#
# Installation                                                                #
#=============================================================================#

# Install the support library.
install(TARGETS tree-gen-lib
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)
install(
    DIRECTORY "$<TARGET_PROPERTY:tree-gen-lib,INTERFACE_INCLUDE_DIRECTORIES>"
    DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}"
    FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inc"
)

# Install the generator tool only if this is the toplevel project.
if (${CMAKE_PROJECT_NAME} STREQUAL tree-gen)
    install(TARGETS tree-gen
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    )
    install(
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include
        DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
        FILES_MATCHING PATTERN "*.h" PATTERN "*.hpp" PATTERN "*.inc"
    )
endif()


endif() # NOT TARGET tree-gen

cmake_minimum_required(VERSION 3.12 FATAL_ERROR)

# Packages
include(FetchContent)

# gtest
find_package(GTest)
if(NOT GTest_FOUND)
    FetchContent_Declare(gtest
        GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG "61720589cbef9707b3c5247d02141a7c31e2876f"
    )
    set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
    FetchContent_MakeAvailable(gtest)
    add_library(gtest::gtest ALIAS gtest)
endif()
include(GoogleTest)

# Test executable
add_executable(${PROJECT_NAME}_test)

# Test sources
target_sources(${PROJECT_NAME}_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_annotatable.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_cbor.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/test_format_utils.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../generator/format_utils.cpp"
)

# Target include directories
target_include_directories(${PROJECT_NAME}_test PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/../generator/"
    "${CMAKE_CURRENT_SOURCE_DIR}/../include/"
)

# Target options
target_link_libraries(${PROJECT_NAME}_test PRIVATE
    PRIVATE tree-gen-lib
    PRIVATE gtest::gtest
)
if(CMAKE_COMPILER_IS_GNUCXX)
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -Wno-error=restrict
        -Wno-error=deprecated-declarations
    )
elseif("${CMAKE_CXX_COMPILER_ID}" MATCHES "Clang")
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        -Wall -Wextra -Werror -Wfatal-errors
        -Wno-error=unused-private-field
        -Wno-error=unused-but-set-variable
    )
elseif(MSVC)
    target_compile_options(${PROJECT_NAME}_test PRIVATE
        /MP /D_USE_MATH_DEFINES /EHsc /bigobj
    )
else()
    message(SEND_ERROR "Unknown compiler!")
endif()

gtest_discover_tests(${PROJECT_NAME}_test
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
)

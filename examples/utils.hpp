#pragma once

#include <fmt/core.h>
#include <stdexcept>  // runtime_error

#define ASSERT(x)                                                                                                       \
    do {                                                                                                                \
        if (!(x)) {                                                                                                     \
            throw std::runtime_error{ fmt::format("assertion failed: {} is false at {}:{}", #x, __FILE__, __LINE__) };  \
        }                                                                                                               \
    } while (0)

#define ASSERT_RAISES(exc, x)                                                                                           \
    do {                                                                                                                \
        try {                                                                                                           \
            x;                                                                                                          \
            throw std::runtime_error{ fmt::format("assertion failed: no exception at {}:{}", __FILE__, __LINE__) } ;    \
        } catch (exc &e) {                                                                                              \
            fmt::print("{} exception message: {}\n", #exc, e.what());                                                   \
        }                                                                                                               \
    } while (0)

#define MARKER fmt::print("###MARKER###\n");

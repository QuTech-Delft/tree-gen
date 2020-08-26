#pragma once

#include <ios>
#include <cstdlib>

/**
 * Check whether predicate is true, printing an error on failure.
 */
#define CHECK(predicate)                                        \
    if (predicate) {} else {                                    \
        std::cerr << "Check failed at "                         \
                  << __FILE__ << ":" << __LINE__ << ": "        \
                  << "(" << #predicate << ") != true"           \
                  << std::endl;                                 \
    }

/**
 * Check whether predicate is true, printing an error and terminating on
 * failure.
 */
#define ASSERT(predicate)                                       \
    if (predicate) {} else {                                    \
        std::cerr << "Check failed at "                         \
                  << __FILE__ << ":" << __LINE__ << ": "        \
                  << "(" << #predicate << ") != true"           \
                  << std::endl;                                 \
        std::exit(1);                                           \
    }

/**
 * Check whether lhs equals rhs, printing on error on failure.
 */
#define CHECK_EQ(lhs, rhs)                                      \
    do {                                                        \
        auto l = lhs;                                           \
        auto r = rhs;                                           \
        if (l != r) {                                           \
            std::cerr << "Check failed at "                     \
                      << __FILE__ << ":" << __LINE__ << ": "    \
                      << "(" << #lhs << ") != (" << #rhs ")"    \
                      << " -> " << l << " != " << r             \
                      << std::endl;                             \
        }                                                       \
    } while (0)

/**
 * Check whether lhs equals rhs, printing on error on failure.
 */
#define ASSERT_EQ(lhs, rhs)                                     \
    do {                                                        \
        auto l = lhs;                                           \
        auto r = rhs;                                           \
        if (l != r) {                                           \
            std::cerr << "Check failed at "                     \
                      << __FILE__ << ":" << __LINE__ << ": "    \
                      << "(" << #lhs << ") != (" << #rhs ")"    \
                      << " -> " << l << " != " << r             \
                      << std::endl;                             \
            std::exit(1);                                       \
        }                                                       \
    } while (0)


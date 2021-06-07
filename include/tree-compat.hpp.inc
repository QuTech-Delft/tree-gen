/** \file
 * Generalized contents of tree-compat.hpp.
 */

#include <sys/types.h>

#ifndef _WIN32
TREE_NAMESPACE_BEGIN
#endif

/**
 * ssize_t is not defined on Windows, so we replace it with a long long.
 */
#ifdef _WIN32
typedef long long signed_size_t;
#else
typedef ssize_t signed_size_t;
#endif

TREE_NAMESPACE_END

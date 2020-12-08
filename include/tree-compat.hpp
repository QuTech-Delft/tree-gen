/** \file
 * Contains cross-platform compatibility logic.
 */

#pragma once

namespace tree {

/**
 * ssize_t is not defined on Windows, so we replace it with a long long.
 */
#ifdef _WIN32
typedef long long signed_size_t;
#else
typedef ssize_t signed_size_t;
#endif

} // namespace tree

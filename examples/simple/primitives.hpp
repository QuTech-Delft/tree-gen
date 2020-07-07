/** \file
 * Defines primitives used in the generated directory tree structure.
 */

#pragma once

#include <string>

/**
 * Namespace with primitives used in the generated directory tree structure.
 */
namespace primitives {

/**
 * Letter primitive, used to represent drive letters.
 */
using Letter = char;

/**
 * Strings, used to represent filenames and file contents.
 */
using String = std::string;

/**
 * Initialization function. This must be specialized for any types used as
 * primitives in a tree that are actual C primitives (int, char, bool, etc),
 * as these are not initialized by the T() construct.
 */
template <class T>
T initialize() { return T(); };

/**
 * Declare the default initializer for drive letters. It's declared inline
 * to avoid having to make a cpp file just for this.
 */
template <>
inline Letter initialize<Letter>() {
    return 'A';
}

} // namespace primitives

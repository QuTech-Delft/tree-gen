/** \file
 * Header file for tree-gen-cpp.cpp.
 */

#ifndef _TREE_GEN_CPP_HPP_INCLUDED_
#define _TREE_GEN_CPP_HPP_INCLUDED_

#include "tree-gen.hpp"

namespace tree_gen {

/**
 * Namespace for C++ code generation.
 */
namespace cpp {

/**
 * Generate the complete C++ code (source and header).
 */
void generate(
    const std::string &header_filename,
    const std::string &source_filename,
    Specification &specification
);

} // namespace cpp
} // namespace tree_gen

#endif

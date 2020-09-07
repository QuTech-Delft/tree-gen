/** \file
 * Header file for tree-gen-python.cpp.
 */

#ifndef _TREE_GEN_PYTHON_HPP_INCLUDED_
#define _TREE_GEN_PYTHON_HPP_INCLUDED_

#include "tree-gen.hpp"

namespace tree_gen {

/**
 * Namespace for Python code generation.
 */
namespace python {

/**
 * Generates the complete Python code.
 */
void generate(
    const std::string &python_filename,
    Specification &specification
);

} // namespace python
} // namespace tree_gen

#endif

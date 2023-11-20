#pragma once

#include <string>
#include <string_view>


/**
 * Namespace for C++ code generation.
 */
namespace tree_gen::cpp {

/**
 * Formats string literal containing C++ code.
 *
 * string_literal is the input string.
 * indent is the indentation, in number of spaces, of the output string.
 * remove_first_line will be true if we want the first line of the input string to be removed, false otherwise.
 */
std::string format_string_literal(std::string_view string_literal, size_t indent, bool remove_firstline = false);

/*
 * Suffix operators for formatting a string literal containing a C++ code.
 *
 * indent is the indentation, in number of spaces, of the output string.
 * drop is the number of lines to be dropped from the front of the string.
 */
std::string operator""_indent_0(const char* str, size_t len);
std::string operator""_indent_0_drop_1(const char* str, size_t len);
std::string operator""_indent_4(const char* str, size_t len);
std::string operator""_indent_4_drop_1(const char* str, size_t len);
std::string operator""_indent_8(const char* str, size_t len);
std::string operator""_indent_8_drop_1(const char* str, size_t len);
std::string operator""_indent_12_drop_1(const char* str, size_t len);

} // namespace tree_gen::cpp

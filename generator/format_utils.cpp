#include "format_utils.hpp"

#include <algorithm>  // min
#include <limits>  // size_t::max
#include <numeric>  // accumulate
#include <range/v3/all.hpp>


namespace tree_gen::cpp {

/**
 * Formats string literal containing C++ code.
 *
 * string_literal is the input string.
 * indent is the indentation, in number of spaces, of the output string.
 * remove_first_new_line will be true if we want the first new line of the input string to be removed, false otherwise.
 */
std::string format_string_literal(std::string_view string_literal, size_t indent, bool remove_first_line) {
    auto string_lines = ranges::views::split(string_literal, '\n')
        | ranges::to<std::vector<std::string>>();
    if (remove_first_line && !string_lines.empty()) {
        string_lines.erase(string_lines.begin());
    }
    if (string_lines.empty()) {
        return {};
    }
    auto current_min_indentation = std::accumulate(string_lines.begin(), string_lines.end(),
        std::numeric_limits<size_t>::max(), [](auto total, auto &line) {
        return std::min(total, line.find_first_not_of(" "));
    });
    return std::accumulate(string_lines.begin(), string_lines.end(), std::string{},
        [indent, current_min_indentation](auto total, auto &line) {
            return total + std::string(indent, ' ') + line.substr(current_min_indentation) + '\n';
    });
}

/*
 * Suffix operators for formatting a string literal containing a C++ code.
 *
 * indent is the indentation, in number of spaces, of the output string.
 * drop is the number of lines to be dropped from the front of the string.
 */
std::string operator""_indent_0_drop_1(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 0, true);
}
std::string operator""_indent_4(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 4);
}
std::string operator""_indent_4_drop_1(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 4, true);
}
std::string operator""_indent_8(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 8);
}
std::string operator""_indent_8_drop_1(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 8, true);
}
std::string operator""_indent_12_drop_1(const char* str, size_t len) {
    return format_string_literal(std::string_view{str, len}, 12, true);
}

}  // namespace tree_gen::cpp

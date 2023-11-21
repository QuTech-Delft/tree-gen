#include "format_utils.hpp"

#include <algorithm>  // min
#include <cassert>  // assert
#include <numeric>  // accumulate
#include <range/v3/all.hpp>
#include <regex>  // regex_replace


namespace tree_gen::cpp {

/**
 * Formats a string literal containing C++ code to be used as an fmt format string.
 *
 * Preconditions: fmt placeholders have to be of the form {0}, {1}, and so on, i.e., the number is mandatory.
 *
 * string_literal is the input string.
 * indent is the indentation, in number of spaces, of the output string.
 * remove_first_line will be true if we want the first line of the input string to be removed, false otherwise.
 */
std::string to_fmt_format_string(std::string_view string_literal, size_t indent, bool remove_first_line) {
    // Remove fist line
    auto string_literal_lines = ranges::views::split(string_literal, '\n')
        | ranges::to<std::vector<std::string>>();
    if (remove_first_line && !string_literal_lines.empty()) {
        string_literal_lines.erase(string_literal_lines.begin());
    }
    if (string_literal_lines.empty()) {
        return {};
    }

    // Calculate current minimum indentation
    auto current_min_indentation = std::accumulate(string_literal_lines.begin(), string_literal_lines.end(),
        std::string::npos, [](auto total, auto &line) {
            return std::min(total, line.find_first_not_of(" "));
    });

    // Rewrite each line with the expected indentation
    auto ret = std::accumulate(string_literal_lines.begin(), string_literal_lines.end(), std::string{},
        [indent, current_min_indentation](auto total, auto &line) {
            assert(current_min_indentation <= line.size());
            return total + std::string(indent, ' ') + line.substr(current_min_indentation) + '\n';
    });

    // fmt strings need to have curly braces doubled, except for the case when they refer to a placeholder
    // Notice this function requires fmt placeholders to use an index, e.g., {0}, {1}, and so on
    ret = std::regex_replace(ret, std::regex{ R"(\{)" }, "{{");  // first double all the curly braces
    ret = std::regex_replace(ret, std::regex{ R"(\})" }, "}}");
    return std::regex_replace(ret, std::regex{ R""(\{(\d+)\})"" }, "$1");  // then undo for the fmt placeholders
}

/*
 * Suffix operators for formatting a string literal containing a C++ code.
 *
 * indent is the indentation, in number of spaces, of the output string.
 * remove_first_line will be true if we want the first line of the input string to be removed, false otherwise.
 */
std::string operator""_indent_0(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 0);
}
std::string operator""_indent_0_remove_first_line(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 0, true);
}
std::string operator""_indent_4(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 4);
}
std::string operator""_indent_4_remove_first_line(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 4, true);
}
std::string operator""_indent_8(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 8);
}
std::string operator""_indent_8_remove_first_line(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 8, true);
}
std::string operator""_indent_12_remove_first_line(const char* str, size_t len) {
    return to_fmt_format_string(std::string_view{str, len}, 12, true);
}

}  // namespace tree_gen::cpp

#include "format_utils.hpp"

#include <gmock/gmock.h>

using namespace tree_gen::cpp;


// Indent 0, normal literals
TEST(indent_0, empty_literal) { EXPECT_EQ(""_indent_0, ""); }
TEST(indent_0, one_line_literal) { EXPECT_EQ("abc"_indent_0, "abc\n"); }
TEST(indent_0, one_line_literal_with_quotes) { EXPECT_EQ("abc\"123\""_indent_0, "abc\"123\"\n"); }
TEST(indent_0, two_lines_literal) { EXPECT_EQ("abc\n123"_indent_0, "abc\n123\n"); }
TEST(indent_0, two_lines_literal_with_quotes) { EXPECT_EQ("abc\n\"123\""_indent_0, "abc\n\"123\"\n"); }

// Indent 0, removing first line
TEST(indent_0_remove_first_line, empty_literal) { EXPECT_EQ(""_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, one_line_literal) { EXPECT_EQ("abc"_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, one_line_literal_with_quotes) {
    EXPECT_EQ("abc\"123\""_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, two_lines_literal) {
    EXPECT_EQ("abc\n123"_indent_0_remove_first_line, "123\n"); }
TEST(indent_0_remove_first_line, two_lines_literal_with_quotes) {
    EXPECT_EQ("abc\n\"123\""_indent_0_remove_first_line, "\"123\"\n"); }

// Indent 0, removing first line, raw literals
TEST(indent_0_remove_first_line, empty_raw_literal) { EXPECT_EQ(R"()"_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, one_line_raw_literal) { EXPECT_EQ(R"(abc)"_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, one_line_raw_literal_with_quotes) {
    EXPECT_EQ(R"(abc"123")"_indent_0_remove_first_line, ""); }
TEST(indent_0_remove_first_line, one_line_raw_literal_with_quotes_and_escaped_quotes) {
    auto formatted_raw_literal = R"(abc"\"123\"")"_indent_0_remove_first_line;
    EXPECT_EQ(formatted_raw_literal, ""); }
TEST(indent_0_remove_first_line, two_lines_raw_literal) {
    EXPECT_EQ(R"(abc
123)"_indent_0_remove_first_line, "123\n"); }
TEST(indent_0_remove_first_line, two_lines_raw_literal_with_quotes) {
    EXPECT_EQ(R"(abc
"123")"_indent_0_remove_first_line, R"("123")""\n"); }
TEST(indent_0_remove_first_line, two_lines_raw_literal_with_quotes_and_escaped_quotes) {
    auto formatted_raw_literal = R"(abc
"\"123\"")"_indent_0_remove_first_line;
    auto expected_formatted_string_literal = R"("\"123\"")""\n";
    EXPECT_EQ(formatted_raw_literal, expected_formatted_string_literal); }

// Indent 4, normal literals
TEST(indent_4, empty_literal) { EXPECT_EQ(""_indent_4, ""); }
TEST(indent_4, one_line_literal) { EXPECT_EQ("abc"_indent_4, "    abc\n"); }
TEST(indent_4, two_lines_literal) { EXPECT_EQ("abc\n123"_indent_4, "    abc\n    123\n"); }

// Indent 4, raw literals
TEST(indent_4, empty_raw_literal) { EXPECT_EQ(R"()"_indent_4, ""); }
TEST(indent_4, one_line_raw_literal) { EXPECT_EQ(R"(abc)"_indent_4, "    abc\n"); }
TEST(indent_4, one_line_raw_literal_with_indent) { EXPECT_EQ(R"(  abc)"_indent_4, "    abc\n"); }
TEST(indent_4, two_lines_raw_literal) { EXPECT_EQ(R"(abc
123)"_indent_4, "    abc\n    123\n"); }
TEST(indent_4, two_lines_raw_literal_with_indent) { EXPECT_EQ(R"(  abc
  123)"_indent_4, "    abc\n    123\n"); }

// Braces
TEST(braces, open_curly_brace) { EXPECT_EQ("{"_indent_0, "{{\n"); }
TEST(braces, close_curly_brace) { EXPECT_EQ("}"_indent_0, "}}\n"); }
TEST(braces, open_close_curly_brace) { EXPECT_EQ("{}"_indent_0, "{{}}\n"); }
TEST(braces, fmt_placeholder_one_digit) { EXPECT_EQ("{2}"_indent_0, "{2}\n"); }
TEST(braces, fmt_placeholder_many_digits) { EXPECT_EQ("{123}"_indent_0, "{123}\n"); }
TEST(braces, fmt_placeholder_with_one_char) { EXPECT_EQ("{a}"_indent_0, "{{a}}\n"); }
TEST(braces, fmt_placeholder_with_two_chars) { EXPECT_EQ("{ab}"_indent_0, "{{ab}}\n"); }
TEST(braces, fmt_placeholder_with_three_chars) { EXPECT_EQ("{abc}"_indent_0, "{{abc}}\n"); }
TEST(braces, open_curly_brace_and_number) { EXPECT_EQ("{2"_indent_0, "{{2\n"); }
TEST(braces, number_and_close_curly_brace) { EXPECT_EQ("2}"_indent_0, "2}}\n"); }
TEST(braces, open_curly_brace_and_text) { EXPECT_EQ("{aaa"_indent_0, "{{aaa\n"); }
TEST(braces, text_and_close_curly_brace) { EXPECT_EQ("aaa}"_indent_0, "aaa}}\n"); }

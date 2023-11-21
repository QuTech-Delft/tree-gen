#include "format_utils.hpp"

#include <gmock/gmock.h>

using namespace tree_gen::cpp;


TEST(indent_0_drop_1, empty_literal) { EXPECT_EQ(""_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, one_line_literal) { EXPECT_EQ("abc"_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, one_line_literal_with_quotes) { EXPECT_EQ("abc\"123\""_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, two_lines_literal) { EXPECT_EQ("abc\n123"_indent_0_drop_1, "123\n"); }
TEST(indent_0_drop_1, two_lines_literal_with_quotes) { EXPECT_EQ("abc\n\"123\""_indent_0_drop_1, "\"123\"\n"); }

TEST(indent_0_drop_1, empty_raw_literal) { EXPECT_EQ(R"()"_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, one_line_raw_literal) { EXPECT_EQ(R"(abc)"_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, one_line_raw_literal_with_quotes) { EXPECT_EQ(R"(abc"123")"_indent_0_drop_1, ""); }
TEST(indent_0_drop_1, one_line_raw_literal_with_quotes_and_escaped_quotes) {
    auto formatted_raw_literal = R"(abc"\"123\"")"_indent_0_drop_1;
    EXPECT_EQ(formatted_raw_literal, "");
}
TEST(indent_0_drop_1, two_lines_raw_literal) { EXPECT_EQ(R"(abc
123)"_indent_0_drop_1, "123\n"); }
TEST(indent_0_drop_1, two_lines_raw_literal_with_quotes) { EXPECT_EQ(R"(abc
"123")"_indent_0_drop_1, R"("123")""\n"); }
TEST(indent_0_drop_1, two_lines_raw_literal_with_quotes_and_escaped_quotes) {
    auto formatted_raw_literal = R"(abc
"\"123\"")"_indent_0_drop_1;
    auto expected_formatted_string_literal = R"("\"123\"")""\n";
    EXPECT_EQ(formatted_raw_literal, expected_formatted_string_literal);
}
TEST(indent_4, empty_literal) { EXPECT_EQ(""_indent_4, ""); }
TEST(indent_4, one_line_literal) { EXPECT_EQ("abc"_indent_4, "    abc\n"); }
TEST(indent_4, two_lines_literal) { EXPECT_EQ("abc\n123"_indent_4, "    abc\n    123\n"); }

TEST(indent_4, empty_raw_literal) { EXPECT_EQ(R"()"_indent_4, ""); }
TEST(indent_4, one_line_raw_literal) { EXPECT_EQ(R"(abc)"_indent_4, "    abc\n"); }
TEST(indent_4, one_line_raw_literal_with_indent) { EXPECT_EQ(R"(  abc)"_indent_4, "    abc\n"); }
TEST(indent_4, two_lines_raw_literal) { EXPECT_EQ(R"(abc
123)"_indent_4, "    abc\n    123\n"); }
TEST(indent_4, two_lines_raw_literal_with_indent) { EXPECT_EQ(R"(  abc
  123)"_indent_4, "    abc\n    123\n"); }

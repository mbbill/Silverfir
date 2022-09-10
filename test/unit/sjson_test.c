/*
 * Copyright 2022 Bai Ming
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/*
MIT License

Copyright (c) 2016 Nicolas Seriot

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "sjson.h"

#include <cmocka.h>
#include <cmocka_private.h>

// The following test cases are modified from the JSONTestSuite project.
// Thanks to Nicolas Seriot for building a great test suite.

static const char * good_json[] = {
    //y_array_arraysWithSpaces.json
    "[[]   ]",
    //y_array_empty-string.json
    "[]",
    //y_array_empty.json
    "[]",
    //y_array_ending_with_newline.json
    "[\"a\"]",
    //y_array_false.json
    "[false]",
    //y_array_heterogeneous.json
    "[null, 1, \"1\", {}]",
    //y_array_null.json
    "[null]",
    //y_array_with_1_and_newline.json
    "[1\n]",
    //y_array_with_leading_space.json
    " [1]",
    //y_array_with_several_null.json
    "[1,null,null,null,2]",
    //y_array_with_trailing_space.json
    "[2] ",
    //y_number.json
    "[123e65]",
    //y_number_0e+1.json
    "[0e+1]",
    //y_number_0e1.json
    "[0e1]",
    //y_number_after_space.json
    "[ 4]",
    //y_number_double_close_to_zero.json
    "[-0.000000000000000000000000000000000000000000000000000000000000000000000000000001]",
    //y_number_int_with_exp.json
    "[20e1]",
    //y_number_minus_zero.json
    "[-0]",
    //y_number_negative_int.json
    "[-123]",
    //y_number_negative_one.json
    "[-1]",
    //y_number_negative_zero.json
    "[-0]",
    //y_number_real_capital_e.json
    "[1E22]",
    //y_number_real_capital_e_neg_exp.json
    "[1E-2]",
    //y_number_real_capital_e_pos_exp.json
    "[1E+2]",
    //y_number_real_exponent.json
    "[123e45]",
    //y_number_real_fraction_exponent.json
    "[123.456e78]",
    //y_number_real_neg_exp.json
    "[1e-2]",
    //y_number_real_pos_exponent.json
    "[1e+2]",
    //y_number_simple_int.json
    "[123]",
    //y_number_simple_real.json
    "[123.456789]",
    //y_object.json
    "{\"asd\":\"sdf\", \"dfg\":\"fgh\"}",
    //y_object_basic.json
    "{\"asd\":\"sdf\"}",
    //y_object_duplicated_key.json
    "{\"a\":\"b\",\"a\":\"c\"}",
    //y_object_duplicated_key_and_value.json
    "{\"a\":\"b\",\"a\":\"b\"}",
    //y_object_empty.json
    "{}",
    //y_object_empty_key.json
    "{\"\":0}",
    //y_object_escaped_null_in_key.json
    "{\"foo\\u0000bar\": 42}",
    //y_object_extreme_numbers.json
    "{ \"min\": -1.0e+28, \"max\": 1.0e+28 }",
    //y_object_long_strings.json
    "{\"x\":[{\"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}], \"id\": \"xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx\"}",
    //y_object_simple.json
    "{\"a\":[]}",
    //y_object_string_unicode.json
    "{\"title\":\"\\u041f\\u043e\\u043b\\u0442\\u043e\\u0440\\u0430 \\u0417\\u0435\\u043c\\u043b\\u0435\\u043a\\u043e\\u043f\\u0430\" }",
    //y_object_with_newlines.json
    "{\n\"a\": \"b\"\n}",
    //y_string_1_2_3_bytes_UTF-8_sequences.json
    "[\"\\u0060\\u012a\\u12AB\"]",
    //y_string_accepted_surrogate_pair.json
    "[\"\\uD801\\udc37\"]",
    //y_string_accepted_surrogate_pairs.json
    "[\"\\ud83d\\ude39\\ud83d\\udc8d\"]",
    //y_string_allowed_escapes.json
    "[\"\\\"\\\\\\/\\b\\f\\n\\r\\t\"]",
    //y_string_backslash_and_u_escaped_zero.json
    "[\"\\\\u0000\"]",
    //y_string_backslash_doublequotes.json
    "[\"\\\"\"]",
    //y_string_comments.json
    "[\"a/*b*/c/*d//e\"]",
    //y_string_double_escape_a.json
    "[\"\\\\a\"]",
    //y_string_double_escape_n.json
    "[\"\\\\n\"]",
    //y_string_escaped_control_character.json
    "[\"\\u0012\"]",
    //y_string_escaped_noncharacter.json
    "[\"\\uFFFF\"]",
    //y_string_in_array.json
    "[\"asd\"]",
    //y_string_in_array_with_leading_space.json
    "[ \"asd\"]",
    //y_string_last_surrogates_1_and_2.json
    "[\"\\uDBFF\\uDFFF\"]",
    //y_string_nbsp_uescaped.json
    "[\"new\\u00A0line\"]",
    //y_string_null_escape.json
    "[\"\\u0000\"]",
    //y_string_one-byte-utf-8.json
    "[\"\\u002c\"]",
    //y_string_simple_ascii.json
    "[\"asd \"]",
    //y_string_space.json
    " ",
    //y_string_surrogates_U+1D11E_MUSICAL_SYMBOL_G_CLEF.json
    "[\"\\uD834\\uDd1e\"]",
    //y_string_three-byte-utf-8.json
    "[\"\\u0821\"]",
    //y_string_two-byte-utf-8.json
    "[\"\\u0123\"]",
    //y_string_uEscape.json
    "[\"\\u0061\\u30af\\u30EA\\u30b9\"]",
    //y_string_uescaped_newline.json
    "[\"new\\u000Aline\"]",
    //y_string_unicode.json
    "[\"\\uA66D\"]",
    //y_string_unicodeEscapedBackslash.json
    "[\"\\u005C\"]",
    //y_string_unicode_U+10FFFE_nonchar.json
    "[\"\\uDBFF\\uDFFE\"]",
    //y_string_unicode_U+1FFFE_nonchar.json
    "[\"\\uD83F\\uDFFE\"]",
    //y_string_unicode_U+200B_ZERO_WIDTH_SPACE.json
    "[\"\\u200B\"]",
    //y_string_unicode_U+2064_invisible_plus.json
    "[\"\\u2064\"]",
    //y_string_unicode_U+FDD0_nonchar.json
    "[\"\\uFDD0\"]",
    //y_string_unicode_U+FFFE_nonchar.json
    "[\"\\uFFFE\"]",
    //y_string_unicode_escaped_double_quote.json
    "[\"\\u0022\"]",
    //y_structure_lonely_false.json
    "false",
    //y_structure_lonely_int.json
    "42",
    //y_structure_lonely_negative_real.json
    "-0.1",
    //y_structure_lonely_null.json
    "null",
    //y_structure_lonely_string.json
    "\"asd\"",
    //y_structure_lonely_true.json
    "true",
    //y_structure_string_empty.json
    "",
    //y_structure_trailing_newline.json
    "[\"a\"]",
    //y_structure_true_in_array.json
    "[true]",
    //y_structure_whitespace_array.json
    " []",
};

static const char * bad_json[] = {
    "[\"\\x00\"]",
    //n_array_1_true_without_comma.json
    "[1 true]",
    //n_array_a_invalid_utf8.json
    "[aå]",
    //n_array_colon_instead_of_comma.json
    "[\"\": 1]",
    //n_array_comma_after_close.json
    "[\"\"],",
    //n_array_comma_and_number.json
    "[,1]",
    //n_array_double_comma.json
    "[1,,2]",
    //n_array_double_extra_comma.json
    "[\"x\",,]",
    //n_array_extra_close.json
    "[\"x\"]]",
    //n_array_extra_comma.json
    "["
    ",]",
    //n_array_incomplete.json
    "[\"x\"",
    //n_array_incomplete_invalid_value.json
    "[x",
    //n_array_inner_array_no_comma.json
    "[3[4]]",
    //n_array_invalid_utf8.json
    "[ÿ]",
    //n_array_items_separated_by_semicolon.json
    "[1:2]",
    //n_array_just_comma.json
    "[,]",
    //n_array_just_minus.json
    "[-]",
    //n_array_missing_value.json
    "[   , "
    "]",
    //n_array_number_and_comma.json
    "[1,]",
    //n_array_number_and_several_commas.json
    "[1,,]",
    //n_array_newlines_unclosed.json
    "[\"a\",\n4\n,1",
    //n_array_star_inside.json
    "[*]",
    //n_array_unclosed.json
    "["
    "",
    //n_array_unclosed_trailing_comma.json
    "[1,",
    //n_array_unclosed_with_object_inside.json
    "[{}",
    //n_incomplete_false.json
    "[fals]",
    //n_incomplete_null.json
    "[nul]",
    //n_incomplete_true.json
    "[tru]",
    //n_number_++.json
    "[++1234]",
    //n_number_+1.json
    "[+1]",
    //n_number_+Inf.json
    "[+Inf]",
    //n_number_-01.json
    "[-01]",
    //n_number_-1.0..json
    "[-1.0.]",
    //n_number_-2..json
    "[-2.]",
    //n_number_-NaN.json
    "[-NaN]",
    //n_number_.-1.json
    "[.-1]",
    //n_number_.2e-3.json
    "[.2e-3]",
    //n_number_0.1.2.json
    "[0.1.2]",
    //n_number_0.3e+.json
    "[0.3e+]",
    //n_number_0.3e.json
    "[0.3e]",
    //n_number_0.e1.json
    "[0.e1]",
    //n_number_0_capital_E+.json
    "[0E+]",
    //n_number_0_capital_E.json
    "[0E]",
    //n_number_0e+.json
    "[0e+]",
    //n_number_0e.json
    "[0e]",
    //n_number_1.0e+.json
    "[1.0e+]",
    //n_number_1.0e-.json
    "[1.0e-]",
    //n_number_1.0e.json
    "[1.0e]",
    //n_number_1_000.json
    "[1 000.0]",
    //n_number_1eE2.json
    "[1eE2]",
    //n_number_2.e+3.json
    "[2.e+3]",
    //n_number_2.e-3.json
    "[2.e-3]",
    //n_number_2.e3.json
    "[2.e3]",
    //n_number_9.e+.json
    "[9.e+]",
    //n_number_Inf.json
    "[Inf]",
    //n_number_NaN.json
    "[NaN]",
    //n_number_U+FF11_fullwidth_digit_one.json
    "[ï¼]",
    //n_number_expression.json
    "[1+2]",
    //n_number_hex_1_digit.json
    "[0x1]",
    //n_number_hex_2_digits.json
    "[0x42]",
    //n_number_infinity.json
    "[Infinity]",
    //n_number_invalid+-.json
    "[0e+-1]",
    //n_number_invalid-negative-real.json
    "[-123.123foo]",
    //n_number_invalid-utf-8-in-bigger-int.json
    "[123å]",
    //n_number_invalid-utf-8-in-exponent.json
    "[1e1å]",
    //n_number_invalid-utf-8-in-int.json
    "[0å]",
    //n_number_minus_infinity.json
    "[-Infinity]",
    //n_number_minus_sign_with_trailing_garbage.json
    "[-foo]",
    //n_number_minus_space_1.json
    "[- 1]",
    //n_number_neg_int_starting_with_zero.json
    "[-012]",
    //n_number_neg_real_without_int_part.json
    "[-.123]",
    //n_number_neg_with_garbage_at_end.json
    "[-1x]",
    //n_number_real_garbage_after_e.json
    "[1ea]",
    //n_number_real_with_invalid_utf8_after_e.json
    "[1eå]",
    //n_number_real_without_fractional_part.json
    "[1.]",
    //n_number_starting_with_dot.json
    "[.123]",
    //n_number_with_alpha.json
    "[1.2a-3]",
    //n_number_with_alpha_char.json
    "[1.8011670033376514H-308]",
    //n_number_with_leading_zero.json
    "[012]",
    //n_object_bad_value.json
    "[\"x\", truth]",
    //n_object_bracket_key.json
    "{[: \"x\"}",
    //n_object_comma_instead_of_colon.json
    "{\"x\", null}",
    //n_object_double_colon.json
    "{\"x\"::\"b\"}",
    //n_object_garbage_at_end.json
    "{\"a\":\"a\" 123}",
    //n_object_key_with_single_quotes.json
    "{key: 'value'}",
    //n_object_lone_continuation_byte_in_key_and_trailing_comma.json
    "{\"¹\":\"0\",}",
    //n_object_missing_colon.json
    "{\"a\" b}",
    //n_object_missing_key.json
    "{:\"b\"}",
    //n_object_missing_semicolon.json
    "{\"a\" \"b\"}",
    //n_object_missing_value.json
    "{\"a\":",
    //n_object_no-colon.json
    "{\"a\"",
    //n_object_non_string_key.json
    "{1:1}",
    //n_object_non_string_key_but_huge_number_instead.json
    "{9999E9999:1}",
    //n_object_repeated_null_null.json
    "{null:null,null:null}",
    //n_object_several_trailing_commas.json
    "{\"id\":0,,,,,}",
    //n_object_single_quote.json
    "{'a':0}",
    //n_object_trailing_comma.json
    "{\"id\":0,}",
    //n_object_trailing_comment.json
    "{\"a\":\"b\"}/**/",
    //n_object_trailing_comment_open.json
    "{\"a\":\"b\"}/**//",
    //n_object_trailing_comment_slash_open.json
    "{\"a\":\"b\"}//",
    //n_object_trailing_comment_slash_open_incomplete.json
    "{\"a\":\"b\"}/",
    //n_object_two_commas_in_a_row.json
    "{\"a\":\"b\",,\"c\":\"d\"}",
    //n_object_unquoted_key.json
    "{a: \"b\"}",
    //n_object_unterminated-value.json
    "{\"a\":\"a",
    //n_object_with_single_string.json
    "{ \"foo\" : \"bar\", \"a\" }",
    //n_object_with_trailing_garbage.json
    "{\"a\":\"b\"}#",
    //n_single_space.json
    //" ",
    //n_string_1_surrogate_then_escape.json
    "[\"\\uD800\\\"]"
    //n_string_1_surrog,ate_then_escape_u.json
    "[\"\\uD800\\u\"]",
    //n_string_1_surrogate_then_escape_u1.json
    "[\"\\uD800\\u1\"]",
    //n_string_1_surrogate_then_escape_u1x.json
    "[\"\\uD800\\u1x\"]",
    //n_string_escape_x.json
    "[\"\\x00\"]",
    //n_string_escaped_backslash_bad.json
    "[\"\\\\\\\"]",
    //n_string_escaped_ctrl_char_tab.json
    "[\"\\\t\"]",
    //n_string_incomplete_escape.json
    "[\"\\\"]",
    //n_string_incomplete_escaped_character.json
    "[\"\\u00A\"]",
    //n_string_incomplete_surrogate.json
    "[\"\\uD834\\uDd\"]",
    //n_string_incomplete_surrogate_escape_invalid.json
    "[\"\\uD800\\uD800\\x\"]",
    //n_string_invalid_backslash_esc.json
    "[\"\\a\"]",
    //n_string_invalid_unicode_escape.json
    "[\"\\uqqqq\"]",
    //n_string_leading_uescaped_thinspace.json
    "[\\u0020\"asd\"]",
    //n_string_no_quotes_with_bad_escape.json
    "[\\n]",
    //n_string_single_doublequote.json
    "\"",
    //n_string_single_quote.json
    "['single quote']",
    //n_string_single_string_no_double_quotes.json
    "abc",
    //n_string_start_escape_unclosed.json
    "[\"\\",
    //n_string_unescaped_newline.json
    "[\"new\nline\"]",
    //n_string_unescaped_tab.json
    "[\"\t\"]",
    //n_string_unicode_CapitalU.json
    "\\UA66D",
    //n_string_with_trailing_garbage.json
    "\"\"x",
    //n_structure_angle_bracket_..json
    "<.>",
    //n_structure_angle_bracket_null.json
    "[<null>]",
    //n_structure_array_trailing_garbage.json
    "[1]x",
    //n_structure_array_with_extra_array_close.json
    "[1]]",
    //n_structure_array_with_unclosed_string.json
    "[\"asd]",
    //n_structure_capitalized_True.json
    "[True]",
    //n_structure_close_unopened_array.json
    "1]",
    //n_structure_comma_instead_of_closing_brace.json
    "{\"x\": true,",
    //n_structure_double_array.json
    "[][]",
    //n_structure_end_array.json
    "]",
    //n_structure_lone-open-bracket.json
    "[",
    //n_structure_number_with_trailing_garbage.json
    "2@",
    //n_structure_object_followed_by_closing_object.json
    "{}}",
    //n_structure_object_unclosed_no_value.json
    "{\"\":",
    //n_structure_object_with_comment.json
    "{\"a\":/*comment*/\"b\"}",
    //n_structure_object_with_trailing_garbage.json
    "{\"a\": true} \"x\"",
    //n_structure_open_array_apostrophe.json
    "['",
    //n_structure_open_array_comma.json
    "[,",
    //n_structure_open_array_open_object.json
    "[{",
    //n_structure_open_array_open_string.json
    "[\"a",
    //n_structure_open_array_string.json
    "[\"a\"",
    //n_structure_open_object.json
    "{",
    //n_structure_open_object_close_array.json
    "{]",
    //n_structure_open_object_comma.json
    "{,",
    //n_structure_open_object_open_array.json
    "{[",
    //n_structure_open_object_open_string.json
    "{\"a",
    //n_structure_open_object_string_with_apostrophes.json
    "{'a'",
    //n_structure_open_open.json
    "[\"\\{[\"\\{[\"\\{[\"\\{",
    //n_structure_single_star.json
    "*",
    //n_structure_trailing_#.json
    "{\"a\":\"b\"}#{}",
    //n_structure_uescaped_LF_before_string.json
    "[\\u000A\"\"]",
    //n_structure_unclosed_array.json
    "[1",
    //n_structure_unclosed_array_partial_null.json
    "[ false, nul",
    //n_structure_unclosed_array_unfinished_false.json
    "[ true, fals",
    //n_structure_unclosed_array_unfinished_true.json
    "[ false, tru",
    //n_structure_unclosed_object.json
    "{\"asd\":\"asd\"",
    // opening array
    "[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[[",
    // ...
    "[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[{"":[",
};

static void sjson_test1(void ** state) {
    r_json_value json_value;

    ARRAY_FOR_EACH(good_json, const char *, iter) {
        json_value = json_parse(s_p(*iter));
        assert_true(is_ok(json_value));
        json_free_value(&json_value.value);
    }

    ARRAY_FOR_EACH(bad_json, const char *, iter) {
        json_value = json_parse(s_p(*iter));
        assert_false(is_ok(json_value));
        json_free_value(&json_value.value);
    }
}

struct CMUnitTest sjson_tests[] = {
    cmocka_unit_test(sjson_test1),
};

const size_t sjson_tests_count = array_len(sjson_tests);

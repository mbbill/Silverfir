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

#include "compiler.h"
#include "option.h"

#include <cmocka.h>
#include <cmocka_private.h>

option_u32 get_none() {
    return none(option_u32);
}

option_u32 get_some() {
    return some(option_u32, 123);
}

option_u32 get_larger_than_10(u32 v) {
    if (v > 10) {
        return some(option_u32, v);
    } else {
        return none(option_u32);
    }
}

static void option_test1(void ** state) {
    option_u32 o;
    o = get_none();
    assert_false(is_some(o));

    o = get_some();
    assert_true(is_some(o));
    assert_int_equal(value(o), 123);

    o = get_larger_than_10(11);
    assert_true(is_some(o));
    assert_int_equal(value(o), 11);

    o = get_larger_than_10(9);
    assert_false(is_some(o));
}

struct CMUnitTest option_tests[] = {
    cmocka_unit_test(option_test1),
};

const size_t option_tests_count = array_len(option_tests);

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
#include "smath.h"
#include "test_containers.h"

#include <cmocka.h>
#include <cmocka_private.h>


static void smath_test1(void ** state) {
    assert_int_equal(s_clz64(0xffffffffffffffffu), 0);
    assert_int_equal(s_clz64(0), 64);
    assert_int_equal(s_clz64(0x00008000u), 48);
    assert_int_equal(s_clz64(0xffu), 56);
    assert_int_equal(s_clz64(0x8000000000000000u), 0);
    assert_int_equal(s_clz64(1), 63);
    assert_int_equal(s_clz64(0x7fffffffffffffffu), 1);
}

struct CMUnitTest smath_tests[] = {
    cmocka_unit_test(smath_test1),
};

const size_t smath_tests_count = array_len(smath_tests);


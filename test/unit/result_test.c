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

#include "result.h"
#include "types.h"

#include <assert.h>
#include <cmocka.h>
#include <cmocka_private.h>

////////////////////////////////////////////////

r foo_r(i32 a) {
    check_prep(r);
    if (a == 0) {
        return ok_r;
    } else {
        return err(e_general, "1234");
    }
}

static void result_test_r(void ** state) {
    r res;

    res = foo_r(1234);
    assert_false(is_ok(res));

    res = foo_r(0);
    assert_true(is_ok(res));
}

////////////////////////////////////////////////

r_i32 foo_plain_types(i32 a) {
    check_prep(r_i32);
    if (a) {
        return ok(a);
    }
    return err(e_general, "999");
}

r_u64 bar_plain_types_forward(i32 a) {
    check_prep(r_u64);

    unwrap(i32, ra, foo_plain_types(123));
    assert_int_equal(ra, 123);

    r_i32 rb = foo_plain_types(0);
    assert_false(is_ok(rb));

    unwrap_drop(i32, foo_plain_types(a));
    return ok((u64)(a));
}

static void result_test_plain_types(void ** state) {
    r res;

    res = foo_r(1234);
    assert_false(is_ok(res));

    res = foo_r(0);
    assert_true(is_ok(res));
}

////////////////////////////////////////////////

typedef struct t1 {
    i32 a;
} t1;
RESULT_TYPE_DECL(t1)

r_t1 foo_struct_types(i32 a) {
    check_prep(r_t1);
    if (a > 0) {
        t1 res;
        res.a = a;
        return ok(res);
    } else {
        return err(e_general, "123");
    }
}

typedef struct t2 {
    double b;
    char c;
} t2;
RESULT_TYPE_DECL(t2)

r_t2 bar_struct_types(i32 a) {
    check_prep(r_t2);

    unwrap(t1, res, foo_struct_types(a));

    t2 t = { .b = res.a };
    if (t.b > 10.0) {
        return ok(t);
    } else {
        return err(e_general, "9999");
    }
}

static void result_test_struct_types(void ** state) {
    r_t2 res = bar_struct_types(-999);
    assert_int_equal(res.value.b, 0);

    res = bar_struct_types(1);
    assert_int_equal(res.value.b, 0);

    res = bar_struct_types(11);
    assert_int_equal(res.value.b, 11);
}

////////////////////////////////////////////////

struct CMUnitTest result_tests[] = {
    cmocka_unit_test(result_test_r),
    cmocka_unit_test(result_test_plain_types),
    cmocka_unit_test(result_test_struct_types),
};

const size_t result_tests_count = array_len(result_tests);

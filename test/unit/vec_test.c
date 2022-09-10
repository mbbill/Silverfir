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

#include "vec.h"
#include "test_containers.h"

#include <cmocka.h>
#include <cmocka_private.h>

static void vec_test1(void ** state) {
    vec_i32 v = {0};
    r ret;

    ret = vec_push_i32(&v, 123);
    assert_true(is_ok(ret));
    assert_int_equal(vec_size_i32(&v), 1);
    assert_non_null(v._data);
    // assuming the default _capacity isn't a very small value, which is obvious.
    assert_int_not_equal(vec_size_i32(&v), v._capacity);

    ret = vec_push_i32(&v, 456);
    assert_true(is_ok(ret));
    assert_int_equal(vec_size_i32(&v), 2);
    assert_int_not_equal(vec_size_i32(&v), v._capacity);

    // push to the _capacity
    for (size_t i = vec_size_i32(&v); i < v._capacity; i++) {
        ret = vec_push_i32(&v, (i32)i);
        assert_true(is_ok(ret));
    }
    assert_int_equal(vec_size_i32(&v), v._capacity);

    // push one more
    ret = vec_push_i32(&v, -1);
    assert_true(is_ok(ret));
    assert_int_not_equal(vec_size_i32(&v), v._capacity);
    assert_int_equal(-1, *vec_back_i32(&v));

    // push to the _capacity again
    for (size_t i = vec_size_i32(&v); i < v._capacity; i++) {
        ret = vec_push_i32(&v, (i32)i);
        assert_true(is_ok(ret));
    }
    assert_int_equal(vec_size_i32(&v), v._capacity);

    // push one more
    ret = vec_push_i32(&v, -1);
    assert_true(is_ok(ret));
    assert_int_not_equal(vec_size_i32(&v), v._capacity);

    ret = vec_shrink_to_fit_i32(&v);
    assert_true(is_ok(ret));
    assert_int_equal(vec_size_i32(&v), v._capacity);

    vec_popall_i32(&v);
    assert_int_equal(vec_size_i32(&v), 0);
    assert_int_not_equal(v._capacity, 0);

    // empty vector will shrink to 1
    ret = vec_shrink_to_fit_i32(&v);
    assert_true(is_ok(ret));
    assert_int_equal(v._capacity, 1);

    vec_clear_i32(&v);
    assert_ptr_equal(v._data, NULL);
    assert_int_equal(v._size, 0);
    assert_int_equal(v._capacity, 0);

    for (int i = 100; i < 200; i++) {
        ret = vec_push_i32(&v, i);
        assert_true(is_ok(ret));
    }
    assert_int_equal(vec_size_i32(&v), 100);
    int j = 100;
    VEC_FOR_EACH(&v, i32, iter) {
        assert_int_equal(j, *iter);
        j++;
    }

    for (int k = 0; k < 100; k++) {
        vec_pop_i32(&v);
    }
    assert_int_equal(vec_size_i32(&v), 0);
    assert_int_not_equal(v._capacity, 0);

    vec_clear_i32(&v);
    assert_ptr_equal(v._data, NULL);
    assert_int_equal(v._size, 0);
    assert_int_equal(v._capacity, 0);
}

static void vec_test2(void ** state) {
#if (__STDC_VERSION__ ) == 201112L
    vec_i32 v = {0};
    r ret;

    ret = vec_reserve(&v, 4);
    assert_int_equal(v._capacity, 4);

    i32 values[] = {123, 456, 789};
    ret = vec_push(&v, values[0]);
    assert_true(is_ok(ret));

    ret = vec_push(&v, values[1]);
    assert_true(is_ok(ret));

    ret = vec_push(&v, values[2]);
    assert_true(is_ok(ret));

    int i = 0;
    VEC_FOR_EACH(&v, i32, iter) {
        assert_int_equal(values[i], *iter);
        i++;
    }
    vec_clear(&v);
    assert_ptr_equal(v._data, NULL);
#endif
}

static void vec_test3(void ** state) {
    r ret;
    vec_u64 v = {0};
    ret = vec_resize_u64(&v, 100);
    assert_true(is_ok(ret));
    assert_non_null(v._data);
    assert_int_equal(v._size, 100);
    assert_true(v._capacity >= v._size);
    VEC_FOR_EACH(&v, u64, iter) {
        assert_int_equal(*iter, 0);
        *iter = 123;
    }
    ret = vec_resize_u64(&v, 200);
    assert_true(is_ok(ret));
    assert_non_null(v._data);
    assert_int_equal(v._size, 200);
    assert_true(v._capacity >= v._size);
    int counter = 0;
    VEC_FOR_EACH(&v, u64, iter) {
        if (counter < 100) {
            assert_int_equal(*iter, 123);
            *iter = 321;
        } else {
            assert_int_equal(*iter, 0);
        }
        counter++;
    }
    assert_int_equal(counter, 200);
    ret = vec_resize_u64(&v, 50);
    assert_true(is_ok(ret));
    assert_non_null(v._data);
    assert_int_equal(v._size, 50);
    assert_true(v._capacity >= v._size);
    VEC_FOR_EACH(&v, u64, iter) {
        assert_int_equal(*iter, 321);
    }
    ret = vec_resize_u64(&v, 0);
    assert_true(is_ok(ret));
    assert_non_null(v._data);
    assert_int_equal(v._size, 0);
    assert_true(v._capacity >= v._size);

    vec_clear_u64(&v);
}

static void vec_test4(void ** state) {
    r ret;
    vec_u32 v = {0};
    for (u32 i = 10; i < 20; i++) {
        ret = vec_push_u32(&v, i);
        assert_true(is_ok(ret));
    }
    // non-overlapping move
    ret = vec_memmove_u32(&v, 0, 5, 5);
    assert_true(is_ok(ret));
    assert_int_equal(*vec_at_u32(&v, 0), 15);
    assert_int_equal(*vec_at_u32(&v, 1), 16);
    assert_int_equal(*vec_at_u32(&v, 2), 17);
    assert_int_equal(*vec_at_u32(&v, 3), 18);
    assert_int_equal(*vec_at_u32(&v, 4), 19);
    assert_int_equal(*vec_at_u32(&v, 5), 15);
    assert_int_equal(*vec_at_u32(&v, 6), 16);
    assert_int_equal(*vec_at_u32(&v, 7), 17);
    assert_int_equal(*vec_at_u32(&v, 8), 18);
    assert_int_equal(*vec_at_u32(&v, 9), 19);
    vec_clear_u32(&v);

    // overlapping move
    for (u32 i = 10; i < 20; i++) {
        ret = vec_push_u32(&v, i);
        assert_true(is_ok(ret));
    }
    ret = vec_memmove_u32(&v, 0, 1, 9);
    assert_true(is_ok(ret));
    assert_int_equal(*vec_at_u32(&v, 0), 11);
    assert_int_equal(*vec_at_u32(&v, 1), 12);
    assert_int_equal(*vec_at_u32(&v, 2), 13);
    assert_int_equal(*vec_at_u32(&v, 3), 14);
    assert_int_equal(*vec_at_u32(&v, 4), 15);
    assert_int_equal(*vec_at_u32(&v, 5), 16);
    assert_int_equal(*vec_at_u32(&v, 6), 17);
    assert_int_equal(*vec_at_u32(&v, 7), 18);
    assert_int_equal(*vec_at_u32(&v, 8), 19);
    assert_int_equal(*vec_at_u32(&v, 9), 19);
    vec_clear_u32(&v);
    // oversize
    for (u32 i = 10; i < 20; i++) {
        ret = vec_push_u32(&v, i);
        assert_true(is_ok(ret));
    }
    ret = vec_memmove_u32(&v, 0, 1, 10);
    assert_false(is_ok(ret));
    vec_clear_u32(&v);
}

static void vec_test_custom(void ** state) {
    vec_t1 v = {0};

    t1 t = (t1){
        .a = 123,
        .b = -123,
    };
    vec_push_t1(&v, t);
    assert_int_equal(vec_size_t1(&v), 1);

    vec_clear_t1(&v);
    assert_ptr_equal(v._data, NULL);
    assert_int_equal(v._size, 0);
    assert_int_equal(v._capacity, 0);
}

struct CMUnitTest vec_tests[] = {
    cmocka_unit_test(vec_test1),
    cmocka_unit_test(vec_test2),
    cmocka_unit_test(vec_test3),
    cmocka_unit_test(vec_test4),
    cmocka_unit_test(vec_test_custom),
};

const size_t vec_tests_count = array_len(vec_tests);

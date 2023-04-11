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

#include "list.h"
#include "list_impl.h"
#include "test_containers.h"

#include <cmocka.h>
#include <cmocka_private.h>

static void list_test1(void ** state) {
    list_u32 l = {0};
    r ret;
    UNUSED(ret);
    {
        ret = list_push_u32(&l, 123);
        assert(is_ok(ret));
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(l._head->_data, 123);

        list_clear_u32(&l);
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
    {
        ret = list_push_front_u32(&l, 456);
        assert(is_ok(ret));
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(l._head->_data, 456);

        list_pop_u32(&l);
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
    {
        ret = list_push_u32(&l, 789);
        assert(is_ok(ret));
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(l._head->_data, 789);

        list_pop_front_u32(&l);
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
    {
        ret = list_push_u32(&l, 789);
        assert(is_ok(ret));
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(l._head->_data, 789);

        list_iter_u32 iter = list_front_u32(&l);
        assert_non_null(iter);
        r_list_iter_u32 ru32;
        ru32 = list_insert_u32(&l, iter, 999);
        assert(is_ok(ru32));
        assert_non_null(ru32.value);
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_not_equal(l._head, l._tail);
        assert_int_equal(l._size, 2);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._tail->_next, NULL);
        assert_int_equal(l._head->_data, 999);
        assert_int_equal(*(ru32.value), 999);

        iter = list_erase_u32(&l, list_back_u32(&l));
        assert_null(iter);
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(l._head->_data, 999);

        iter = list_erase_u32(&l, list_front_u32(&l));
        assert_null(iter);
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
}

static void list_test2(void ** state) {
    list_u32 l = {0};
    r ret;
    {
        u32 u = 123;
        for (int i = 0; i < 999; i++) {
            ret = list_push_u32(&l, u++);
            assert_true(is_ok(ret));
            assert_int_equal(list_size_u32(&l), i+1);
        }
        u32 k = 123;
        LIST_FOR_EACH(&l, u32, iter) {
            assert_non_null(iter);
            assert_int_equal(*iter, k++);
        }
        for (int i = 999; i> 0; i--) {
            list_iter_u32 iter = list_back_u32(&l);
            assert_non_null(iter);
            assert_int_equal(*iter, --u);
            assert_int_equal(list_size_u32(&l), i);
            list_pop_u32(&l);
        }
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
    {
        u32 u = 123;
        for (int i = 0; i < 999; i++) {
            ret = list_push_front_u32(&l, u++);
            assert_true(is_ok(ret));
            assert_int_equal(list_size_u32(&l), i+1);
        }
        u32 k = 123;
        LIST_FOR_EACH_REVERSE(&l, u32, iter) {
            assert_int_equal(*iter, k++);
        }
        for (int i = 999; i> 0; i--) {
            list_iter_u32 iter = list_front_u32(&l);
            assert_non_null(iter);
            assert_int_equal(*iter, --u);
            assert_int_equal(list_size_u32(&l), i);
            list_pop_front_u32(&l);
        }
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
}

static void list_test3(void ** state) {
    list_t1 l = {0};
    r ret;
    UNUSED(ret);
    {
        ret = list_push_t1(&l, (t1){1,2});
        assert(is_ok(ret));
        assert_non_null(l._head);
        assert_non_null(l._tail);
        assert_ptr_equal(l._head, l._tail);
        assert_int_equal(l._size, 1);
        assert_ptr_equal(l._head->_prev, NULL);
        assert_ptr_equal(l._head->_next, NULL);
        assert_int_equal(list_front_t1(&l)->a, 1);

        list_pop_front_t1(&l);
        assert_null(l._head);
        assert_null(l._tail);
        assert_int_equal(l._size, 0);
    }
}

// Generated by Chatgpt
// Define a custom data type for testing purposes
typedef struct {
    int x;
    float y;
} custom_data_t;

// Instantiate lists for built-in and custom types
LIST_DECL_FOR_TYPE(int)
LIST_IMPL_FOR_TYPE(int)
LIST_DECL_FOR_TYPE(float)
LIST_IMPL_FOR_TYPE(float)
LIST_DECL_FOR_TYPE(custom_data_t)
LIST_IMPL_FOR_TYPE(custom_data_t)

// Test the push, pop, push_front, and pop_front functions for integer lists
static void test_list_push_pop_int(void **state) {
    (void) state; // Unused

    list_int my_list = {0};

    list_push_int(&my_list, 1);
    list_push_int(&my_list, 2);
    list_push_int(&my_list, 3);
    assert_int_equal(list_size_int(&my_list), 3);

    list_push_front_int(&my_list, 0);
    assert_int_equal(list_size_int(&my_list), 4);

    list_pop_int(&my_list);
    assert_int_equal(list_size_int(&my_list), 3);
    assert_int_equal(*list_back_int(&my_list), 2);

    list_pop_front_int(&my_list);
    assert_int_equal(list_size_int(&my_list), 2);
    assert_int_equal(*list_front_int(&my_list), 1);

    list_clear_int(&my_list);
}

// Test the insert and erase functions for integer lists
static void test_list_insert_erase_int(void **state) {
    (void) state; // Unused

    list_int my_list = {0};

    // Fill the list with some values
    for (int i = 0; i < 5; ++i) {
        list_push_int(&my_list, i);
    }

    list_iter_int iter = list_front_int(&my_list);
    iter = list_iter_next_int(iter); // Move to the second element
    list_insert_int(&my_list, iter, 42); // Insert 42 before the second element
    assert_int_equal(my_list._size, 6);
    assert_int_equal(*list_iter_prev_int(list_iter_next_int(iter)), 1);

    iter = list_erase_int(&my_list, iter); // Erase the second element, the iterator should point to the third element
    assert_int_equal(*iter, 2);
    assert_int_equal(my_list._size, 5);

    list_clear_int(&my_list);
}

// Test the clear function for integer lists
static void test_list_clear_int(void **state) {
    (void) state; // Unused

    list_int my_list = {0};

    // Fill the list with some values
    for (int i = 0; i < 5; ++i) {
        list_push_int(&my_list, i);
    }

    list_clear_int(&my_list);
    assert_int_equal(list_size_int(&my_list), 0);
    assert_null(list_front_int(&my_list));
    assert_null(list_back_int(&my_list));

    list_clear_int(&my_list);
}

// Test the push, pop, push_front, and pop_front functions for float lists
static void test_list_push_pop_float(void **state) {
    (void) state; // Unused

    list_float my_list = {0};

    list_push_float(&my_list, 1.0f);
    list_push_float(&my_list, 2.0f);
    list_push_float(&my_list, 3.0f);
    assert_int_equal(list_size_float(&my_list), 3);

    list_push_front_float(&my_list, 0.0f);
    assert_int_equal(list_size_float(&my_list), 4);

    list_pop_float(&my_list);
    assert_int_equal(list_size_float(&my_list), 3);
    assert_float_equal(*list_back_float(&my_list), 2.0f, 0.001f);

    list_pop_front_float(&my_list);
    assert_int_equal(list_size_float(&my_list), 2);
    assert_float_equal(*list_front_float(&my_list), 1.0f, 0.001f);

    list_clear_float(&my_list);
}

// Test the insert and erase functions for float lists
static void test_list_insert_erase_float(void ** state) {
    (void)state; // Unused

    list_float my_list = {0};

    // Fill the list with some values
    for (int i = 0; i < 5; ++i) {
        list_push_float(&my_list, (float)i);
    }

    list_iter_float iter = list_front_float(&my_list);
    iter = list_iter_next_float(iter); // Move to the second element
    list_insert_float(&my_list, iter, 42.0f); // Insert 42.0f before the second element
    assert_int_equal(my_list._size, 6);
    assert_float_equal(*list_iter_prev_float(list_iter_next_float(iter)), 1.0f, 0.001f);

    iter = list_erase_float(&my_list, iter); // Erase the second element
    assert_float_equal(*iter, 2.0f, 0.001f);
    assert_int_equal(my_list._size, 5);

    list_clear_float(&my_list);
}

// Test the clear function for float lists
static void test_list_clear_float(void **state) {
    (void) state; // Unused

    list_float my_list = {0};

    // Fill the list with some values
    for (int i = 0; i < 5; ++i) {
        list_push_float(&my_list, (float)i);
    }

    list_clear_float(&my_list);
    assert_int_equal(list_size_float(&my_list), 0);
    assert_null(list_front_float(&my_list));
    assert_null(list_back_float(&my_list));

    list_clear_float(&my_list);
}

struct CMUnitTest list_tests[] = {
    cmocka_unit_test(list_test1),
    cmocka_unit_test(list_test2),
    cmocka_unit_test(list_test3),
    cmocka_unit_test(test_list_push_pop_int),
    cmocka_unit_test(test_list_insert_erase_int),
    cmocka_unit_test(test_list_clear_int),
    cmocka_unit_test(test_list_push_pop_float),
    cmocka_unit_test(test_list_insert_erase_float),
    cmocka_unit_test(test_list_clear_float),
};

const size_t list_tests_count = array_len(list_tests);

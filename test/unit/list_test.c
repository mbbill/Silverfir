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

struct CMUnitTest list_tests[] = {
    cmocka_unit_test(list_test1),
    cmocka_unit_test(list_test2),
    cmocka_unit_test(list_test3),
};

const size_t list_tests_count = array_len(list_tests);

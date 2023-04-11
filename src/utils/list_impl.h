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

// list_impl.h is a bit special.
// Please refer to the vec_impl.h for more information.

#pragma once

#include "alloc.h"
#include "list.h"
#include "silverfir.h"

#if !defined(NDEBUG)
#define LIST_IS_VALID_IMPL(type, _)                                                                     \
    INLINE bool list_is_valid_##type(list_##type * l) {                                                 \
        return ((l->_size) && (l->_head) && (l->_tail)) || (!(l->_size) && !(l->_head) && !(l->_tail)); \
    }
#else
#define LIST_IS_VALID_IMPL(type, _)
#endif

#define LIST_PUSH_IMPL(type, _)                                     \
    r list_push_##type(list_##type * l, type v) {                   \
        check_prep(r);                                              \
        assert(l);                                                  \
        assert(list_is_valid_##type(l));                            \
        list_item_##type * item = array_alloc(list_item_##type, 1); \
        if (!item) {                                                \
            return err(e_general, "list: out of memory");                      \
        }                                                           \
        *item = (list_item_##type){                                 \
            ._data = v,                                             \
        };                                                          \
        if (unlikely(!(l->_head))) {                                \
            l->_head = l->_tail = item;                             \
        } else {                                                    \
            l->_tail->_next = item;                                 \
            item->_prev = l->_tail;                                 \
            l->_tail = item;                                        \
        }                                                           \
        l->_size++;                                                 \
        return ok_r;                                                \
    }

#define LIST_POP_IMPL(type, _)              \
    void list_pop_##type(list_##type * l) { \
        assert(l);                          \
        assert(list_is_valid_##type(l));    \
        if (unlikely(l->_size == 1)) {      \
            array_free(l->_tail);           \
            *l = (list_##type){0};          \
            return;                         \
        }                                   \
        l->_tail = l->_tail->_prev;         \
        array_free(l->_tail->_next);        \
        l->_tail->_next = NULL;             \
        l->_size--;                         \
        return;                             \
    }

#define LIST_PUSH_FRONT_IMPL(type, _)                               \
    r list_push_front_##type(list_##type * l, type v) {             \
        check_prep(r);                                              \
        assert(l);                                                  \
        assert(list_is_valid_##type(l));                            \
        list_item_##type * item = array_alloc(list_item_##type, 1); \
        if (!item) {                                                \
            return err(e_general, "list: out of memory");                      \
        }                                                           \
        *item = (list_item_##type){                                 \
            ._data = v,                                             \
        };                                                          \
        if (unlikely(!(l->_head))) {                                \
            l->_head = l->_tail = item;                             \
        } else {                                                    \
            l->_head->_prev = item;                                 \
            item->_next = l->_head;                                 \
            l->_head = item;                                        \
        }                                                           \
        l->_size++;                                                 \
        return ok_r;                                                \
    }

#define LIST_POP_FRONT_IMPL(type, _)              \
    void list_pop_front_##type(list_##type * l) { \
        assert(l);                                \
        assert(list_is_valid_##type(l));          \
        if (unlikely(l->_size == 1)) {            \
            array_free(l->_head);                 \
            *l = (list_##type){0};                \
            return;                               \
        }                                         \
        l->_head = l->_head->_next;               \
        array_free(l->_head->_prev);              \
        l->_head->_prev = NULL;                   \
        l->_size--;                               \
    }

#define LIST_INSERT_IMPL(type, _)                                                        \
    r_list_iter_##type list_insert_##type(list_##type * l, list_iter_##type i, type v) { \
        check_prep(r_list_iter_##type);                                                  \
        assert(i);                                                                       \
        list_item_##type * wrapper = LIST_ITER_TO_WRAPPER(type, i);                      \
        list_item_##type * item = array_alloc(list_item_##type, 1);                      \
        if (!item) {                                                                     \
            return err(e_general, "list: out of memory");                                           \
        }                                                                                \
        *item = (list_item_##type){                                                      \
            ._data = v,                                                                  \
        };                                                                               \
        item->_next = wrapper;                                                           \
        item->_prev = wrapper->_prev;                                                    \
        if (wrapper->_prev) {                                                            \
            wrapper->_prev->_next = item;                                                \
        }                                                                                \
        wrapper->_prev = item;                                                           \
        if (l->_head == wrapper) {                                                       \
            l->_head = item;                                                             \
        }                                                                                \
        l->_size++;                                                                      \
        return ok(&(item->_data));                                                       \
    }

#define LIST_ERASE_IMPL(type, _)                                                       \
    list_iter_##type list_erase_##type(list_##type * l, list_iter_##type i) {          \
        assert(i);                                                                     \
        list_item_##type * wrapper = LIST_ITER_TO_WRAPPER(type, i);                    \
        if (wrapper->_prev) {                                                          \
            wrapper->_prev->_next = wrapper->_next;                                    \
        }                                                                              \
        if (wrapper->_next) {                                                          \
            wrapper->_next->_prev = wrapper->_prev;                                    \
        }                                                                              \
        if (l->_head == wrapper) {                                                     \
            l->_head = wrapper->_next;                                                 \
        }                                                                              \
        if (l->_tail == wrapper) {                                                     \
            l->_tail = wrapper->_prev;                                                 \
        }                                                                              \
        l->_size--;                                                                    \
        list_iter_##type iter_next = wrapper->_next ? &(wrapper->_next->_data) : NULL; \
        array_free(wrapper);                                                           \
        return iter_next;                                                              \
    }

#define LIST_CLEAR_IMPL(type, _)                       \
    void list_clear_##type(list_##type * l) {          \
        assert(l);                                     \
        while (l->_head) {                             \
            list_item_##type * next = l->_head->_next; \
            array_free(l->_head);                      \
            l->_head = next;                           \
        }                                              \
        *l = (list_##type){0};                         \
    }

#define LIST_IMPL_FOR_TYPE_BUILTIN(type, _) \
    LIST_IS_VALID_IMPL(type, _)             \
    LIST_PUSH_IMPL(type, _)                 \
    LIST_POP_IMPL(type, _)                  \
    LIST_PUSH_FRONT_IMPL(type, _)           \
    LIST_POP_FRONT_IMPL(type, _)            \
    LIST_INSERT_IMPL(type, _)               \
    LIST_ERASE_IMPL(type, _)                \
    LIST_CLEAR_IMPL(type, _)

#define LIST_IMPL_FOR_TYPE(type) LIST_IMPL_FOR_TYPE_BUILTIN(type, _)

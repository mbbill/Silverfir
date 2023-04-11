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

// A generic doubly-linked list implementation
// The object is copyable (moveable), which means you can copy it around but can only
// manipulate the newest copied one.
// In the end, make sure to call list_erase_##type to prevent memory leak.

#pragma once

#include "compiler.h"
#include "result.h"
#include "silverfir.h"
#include "types.h"

#include <assert.h>

#define LIST_TYPE_DECL(type, _)          \
    struct list_##type;                  \
    typedef struct list_item_##type {    \
        struct list_item_##type * _prev; \
        struct list_item_##type * _next; \
        type _data;                      \
    } list_item_##type;                  \
    typedef type * list_iter_##type;     \
    RESULT_TYPE_DECL(list_iter_##type)   \
    typedef struct list_##type {         \
        list_item_##type * _head;        \
        list_item_##type * _tail;        \
        size_t _size;                    \
    } list_##type;

#define LIST_ITER_TO_WRAPPER(type, iter) TO_WRAPPER(list_item_##type, _data, iter)

// push back
#define LIST_PUSH_DECL(type, _) \
    r list_push_##type(list_##type * l, type v);

#define LIST_POP_DECL(type, _) \
    void list_pop_##type(list_##type * l);

#define LIST_PUSH_FRONT_DECL(type, _) \
    r list_push_front_##type(list_##type * l, type v);

#define LIST_POP_FRONT_DECL(type, _) \
    void list_pop_front_##type(list_##type * l);

// insert element before the element at the iter, and return the newly inserted element's iter
#define LIST_INSERT_DECL(type, _) \
    r_list_iter_##type list_insert_##type(list_##type * l, list_iter_##type i, type v);

// return the iterator to the next element, if exists.
#define LIST_ERASE_DECL(type, _) \
    list_iter_##type list_erase_##type(list_##type * l, list_iter_##type i);

// clear the storage and reset the list to empty.
#define LIST_CLEAR_DECL(type, _) \
    void list_clear_##type(list_##type * l);

#define LIST_SIZE_DECL(type, _)                       \
    INLINE size_t list_size_##type(list_##type * l) { \
        return l->_size;                              \
    }

#define LIST_FRONT_DECL(type, _)                                 \
    INLINE list_iter_##type list_front_##type(list_##type * l) { \
        assert(l);                                               \
        return l->_head ? &(l->_head->_data) : NULL;             \
    }

#define LIST_BACK_DECL(type, _)                                 \
    INLINE list_iter_##type list_back_##type(list_##type * l) { \
        assert(l);                                              \
        return l->_tail ? &(l->_tail->_data) : NULL;            \
    }

#define LIST_ITER_NEXT_DECL(type, _)                                                                         \
    INLINE list_iter_##type list_iter_next_##type(list_iter_##type i) {                                      \
        assert(i);                                                                                           \
        return LIST_ITER_TO_WRAPPER(type, i)->_next ? &(LIST_ITER_TO_WRAPPER(type, i)->_next->_data) : NULL; \
    }

#define LIST_ITER_PREV_DECL(type, _)                                                                         \
    INLINE list_iter_##type list_iter_prev_##type(list_iter_##type i) {                                      \
        assert(i);                                                                                           \
        return LIST_ITER_TO_WRAPPER(type, i)->_prev ? &(LIST_ITER_TO_WRAPPER(type, i)->_prev->_data) : NULL; \
    }

// Unchecked!!
#define LIST_FOR_EACH(list, type, iter)                                              \
    for (list_iter_##type iter = (((list)->_head) ? &((list)->_head->_data) : NULL); \
         iter;                                                                       \
         iter = LIST_ITER_TO_WRAPPER(type, iter)->_next ? &(LIST_ITER_TO_WRAPPER(type, iter)->_next->_data) : NULL)

#define LIST_FOR_EACH_REVERSE(list, type, iter)                                      \
    for (list_iter_##type iter = (((list)->_tail) ? &((list)->_tail->_data) : NULL); \
         iter;                                                                       \
         iter = LIST_ITER_TO_WRAPPER(type, iter)->_prev ? &(LIST_ITER_TO_WRAPPER(type, iter)->_prev->_data) : NULL)

#define LIST_DECL_FOR_TYPE_BUILTIN(type, _) \
    UNUSED_FUNCTION_WARNING_PUSH            \
    LIST_TYPE_DECL(type, _)                 \
    LIST_PUSH_DECL(type, _)                 \
    LIST_POP_DECL(type, _)                  \
    LIST_PUSH_FRONT_DECL(type, _)           \
    LIST_POP_FRONT_DECL(type, _)            \
    LIST_INSERT_DECL(type, _)               \
    LIST_ERASE_DECL(type, _)                \
    LIST_CLEAR_DECL(type, _)                \
    LIST_SIZE_DECL(type, _)                 \
    LIST_FRONT_DECL(type, _)                \
    LIST_BACK_DECL(type, _)                 \
    LIST_ITER_NEXT_DECL(type, _)            \
    LIST_ITER_PREV_DECL(type, _)            \
    UNUSED_FUNCTION_WARNING_POP

#define LIST_DECL_FOR_TYPE(type) LIST_DECL_FOR_TYPE_BUILTIN(type, _)
FOR_EACH_TYPE(LIST_DECL_FOR_TYPE_BUILTIN, _)

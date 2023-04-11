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

#include "alloc.h"
#include "silverfir.h"
#include "types.h"
#include "vec.h"

#include <string.h>

// This file is not supposed to be used as a general header. Instead,
// It should be included only in the custom vector type implementation sources.
// Common vector types has been instantiated in the containers_impl.c
// You need to create your own custom_vec.c and include this header.
// See the unit tests for more info.

#define VEC_CLEAR_IMPL(type, _)             \
    void vec_clear_##type(vec_##type * v) { \
        assert(vec_is_valid_##type(v));     \
        if ((v)->_data) {                   \
            array_free((v)->_data);         \
        }                                   \
        *(v) = (vec_##type){0};             \
        return;                             \
    }

#define VEC_POPALL_IMPL(type, _)             \
    void vec_popall_##type(vec_##type * v) { \
        assert(vec_is_valid_##type(v));      \
        if (unlikely((v)->_size == 0)) {     \
            return;                          \
        }                                    \
        (v)->_size = 0;                      \
    }

#define VEC_SHRINK_TO_FIT_IMPL(type, _)                                        \
    r vec_shrink_to_fit_##type(vec_##type * v) {                               \
        assert(vec_is_valid_##type(v));                                        \
        check_prep(r);                                                         \
        if ((v)->fixed) {                                                      \
            return err(e_general, "vector is fixed");                                     \
        }                                                                      \
        if (unlikely(!((v)->_data))) {                                         \
            return ok_r;                                                       \
        }                                                                      \
        if (((v)->_size == (v)->_capacity) || ((v)->_capacity == 1)) {         \
            return ok_r;                                                       \
        }                                                                      \
        /* if the vector is empty, shrink to 1 element. */                     \
        size_t new_capacity = (v)->_size ? (v)->_size : 1;                     \
        vec_iter_##type _data = array_realloc(type, (v)->_data, new_capacity); \
        if (!_data) {                                                          \
            return err(e_general, "vector: realloc failed");                              \
        }                                                                      \
        (v)->_capacity = new_capacity;                                         \
        (v)->_data = _data;                                                    \
        return ok_r;                                                           \
    }

#define VEC_RESERVE_IMPL(type, _)                                  \
    r vec_reserve_##type(vec_##type * v, size_t reserve_size) {    \
        assert(vec_is_valid_##type(v));                            \
        check_prep(r);                                             \
        if ((v)->fixed) {                                          \
            return err(e_general, "vector is fixed");                         \
        }                                                          \
        if (reserve_size <= (v)->_capacity) {                      \
            return ok_r;                                           \
        }                                                          \
        vec_iter_##type _data;                                     \
        if ((v)->_data) {                                          \
            _data = array_realloc(type, (v)->_data, reserve_size); \
            if (!_data) {                                          \
                return err(e_general, "vector: realloc failed");              \
            }                                                      \
        } else {                                                   \
            _data = array_alloc(type, reserve_size);               \
            if (!_data) {                                          \
                return err(e_general, "vector: out of memory");               \
            }                                                      \
        }                                                          \
        (v)->_capacity = reserve_size;                             \
        (v)->_data = _data;                                        \
        return ok_r;                                               \
    }

#define VEC_RESIZE_IMPL(type, _)                                                \
    r vec_resize_##type(vec_##type * v, size_t size) {                          \
        assert(vec_is_valid_##type(v));                                         \
        check_prep(r);                                                          \
        if (size <= (v)->_size) {                                               \
            (v)->_size = size;                                                  \
            return ok_r;                                                        \
        }                                                                       \
        if (size > (v)->_capacity) {                                            \
            check(vec_reserve_##type(v, size));                                 \
        }                                                                       \
        memset((v)->_data + (v)->_size, 0, (size - (v)->_size) * sizeof(type)); \
        (v)->_size = size;                                                      \
        return ok_r;                                                            \
    }

#define VEC_MEMMOVE_IMPL(type, _)                                               \
    r vec_memmove_##type(vec_##type * v, size_t dst, size_t src, size_t size) { \
        assert(vec_is_valid_##type(v));                                         \
        check_prep(r);                                                          \
        if ((src + size > (v)->_size) || (dst + size > (v)->_size)) {           \
            return err(e_general, "vector: memmove exceeds limit");                        \
        }                                                                       \
        memmove((v)->_data + dst, (v)->_data + src, size * sizeof(type));       \
        return ok_r;                                                            \
    }

#define VEC_DUP_IMPL(type, _)                                  \
    r vec_dup_##type(vec_##type * dst, vec_##type * src) {     \
        assert(dst);                                           \
        assert(src);                                           \
        assert(!dst->_capacity && !dst->_data && !dst->_size); \
        check_prep(r);                                         \
        if (!src->_size) {                                     \
            return ok_r;                                       \
        }                                                      \
        vec_iter_##type _data = array_alloc(type, src->_size); \
        if (!_data) {                                          \
            return err(e_general, "vector: out of memory");               \
        }                                                      \
        memcpy(_data, src->_data, src->_size * sizeof(type));  \
        dst->_data = _data;                                    \
        dst->_capacity = src->_size;                           \
        dst->_size = src->_size;                               \
        return ok_r;                                           \
    }

#define VEC_IMPL_FOR_TYPE_BUILTIN(type, _) \
    VEC_CLEAR_IMPL(type, _)                \
    VEC_POPALL_IMPL(type, _)               \
    VEC_SHRINK_TO_FIT_IMPL(type, _)        \
    VEC_RESERVE_IMPL(type, _)              \
    VEC_MEMMOVE_IMPL(type, _)              \
    VEC_DUP_IMPL(type, _)                  \
    VEC_RESIZE_IMPL(type, _)

#define VEC_IMPL_FOR_TYPE(type) VEC_IMPL_FOR_TYPE_BUILTIN(type, _)

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

// This is a generic vector type.
// Built-in types can be called with generic function names like below
//      vec_i32 ivec;
//      r result = vec_push(&ivec, 123);
// or
//      vec_u8_2 u82vec;
//      r result = vec_push(&u82vec, 12);
// Custom types cannot be automatically inferred and needs to be called with type
// postfix, like below
//      typedef u8 custom;
//      test_containers cvec;
//      r result = vec_push_custom(&cvec, 0x1);
// You probably don't want to read this header, but it's really simple
// to use. See unit tests for some examples.

// notes:
//   You should not make perment pointers to the vector member when it's not fixed because
//     the vector might grow and the memory can get invalidated. To do so, set it to fixed.

#pragma once

#include "alloc.h"
#include "compiler.h"
#include "result.h"
#include "silverfir.h"
#include "types.h"

#include <assert.h>
#include <stdbool.h>

#define VEC_DEFAULT_CAPACITY (10)
#define VEC_GROW_FACTOR (1.5)

#define VEC_TYPE_DECL(type, _)  \
    typedef struct vec_##type { \
        size_t _size;           \
        size_t _capacity;       \
        type * _data;           \
        bool fixed;             \
    } vec_##type;               \
    typedef type * vec_iter_##type;

#define VEC_ASSOCIATION_LIST(type, func) vec_##type * : vec_##func##_##type,

// _Generic requires C11
#if (__STDC_VERSION__) >= 201112L
extern void vec_undefined(int, ...);
#define VEC_FUNC1(name, v)                                     \
    _Generic((v),                                              \
             FOR_EACH_TYPE(VEC_ASSOCIATION_LIST, name) default \
             : vec_undefined)(v)
#define VEC_FUNC2(name, v, a)                                  \
    _Generic((v),                                              \
             FOR_EACH_TYPE(VEC_ASSOCIATION_LIST, name) default \
             : vec_undefined)(v, a)
#define VEC_FUNC3(name, v, a, b)                               \
    _Generic((v),                                              \
             FOR_EACH_TYPE(VEC_ASSOCIATION_LIST, name) default \
             : vec_undefined)(v, a, b)
#define VEC_FUNC4(name, v, a, b, c)                            \
    _Generic((v),                                              \
             FOR_EACH_TYPE(VEC_ASSOCIATION_LIST, name) default \
             : vec_undefined)(v, a, b, c)
#endif

#define VEC_IS_VALID_DECL(type, _)                                                                                                     \
    inline bool vec_is_valid_##type(vec_##type * v) {                                                                                  \
        assert(v);                                                                                                                     \
        return ((v)->_data) ? (((v)->_capacity > 0) && ((v)->_capacity >= (v)->_size)) : (((v)->_capacity == 0) && ((v)->_size == 0)); \
    }

// The differene between clear and popall is that clear will reclaim
// the memory but popall will keep the capacity.
// void vec_clear(vec_i32 * v);
#define VEC_CLEAR_DECL(type, _) void vec_clear_##type(vec_##type * v);
#define vec_clear(v) VEC_FUNC1(clear, v)

// r vec_push_i32(vec_i32 * v, i32 i);
// #define VEC_PUSH_DECL(type, _) r vec_push_##type(vec_##type * v, type e);
#define vec_push(v, e) VEC_FUNC2(push, v, e)
#define VEC_PUSH_DECL(type, _)                                                     \
    INLINE r vec_push_##type(vec_##type * v, type e) {                             \
        assert(vec_is_valid_##type(v));                                            \
        check_prep(r);                                                             \
        if (likely((v)->_capacity > (v)->_size)) {                                 \
            (v)->_data[(v)->_size] = e;                                            \
            (v)->_size++;                                                          \
        } else if (!((v)->_data)) {                                                \
            if ((v)->fixed) {                                                      \
                return err(e_general, "vector is fixed");                                     \
            }                                                                      \
            vec_iter_##type _data = array_alloc(type, VEC_DEFAULT_CAPACITY);       \
            if (!_data) {                                                          \
                return err(e_general, "vector: out of memory");                               \
            }                                                                      \
            (v)->_data = _data;                                                    \
            (v)->_capacity = VEC_DEFAULT_CAPACITY;                                 \
            (v)->_data[(v)->_size] = e;                                            \
            (v)->_size = 1;                                                        \
        } else if ((v)->_size == (v)->_capacity) {                                 \
            if ((v)->fixed) {                                                      \
                return err(e_general, "vector is fixed");                                     \
            }                                                                      \
            size_t new_capacity = (size_t)((v)->_capacity * VEC_GROW_FACTOR) + 1;  \
            if (new_capacity <= (v)->_capacity) {                                  \
                return err(e_general, "vector: can't grow");                                  \
            }                                                                      \
            vec_iter_##type _data = array_realloc(type, (v)->_data, new_capacity); \
            if (!_data) {                                                          \
                return err(e_general, "vector: out of memory");                               \
            }                                                                      \
            (v)->_data = _data;                                                    \
            (v)->_capacity = new_capacity;                                         \
            (v)->_data[(v)->_size] = e;                                            \
            (v)->_size++;                                                          \
        }                                                                          \
        return ok_r;                                                               \
    }

// void vec_pop_i32(vec_i32 * v);
// #define VEC_POP_DECL(type, _) void vec_pop_##type(vec_##type * v);
#define vec_pop(v) VEC_FUNC1(pop, v)
#define VEC_POP_DECL(type, _)                    \
    INLINE void vec_pop_##type(vec_##type * v) { \
        assert(vec_is_valid_##type(v));          \
        assert((v)->_size);                      \
        (v)->_size--;                            \
        return;                                  \
    }

// void vec_pop_i32(vec_i32 * v);
#define VEC_POPALL_DECL(type, _) void vec_popall_##type(vec_##type * v);
#define vec_popall(v) VEC_FUNC1(popall, v)

// r vec_shrink_to_fit_i32(vec_i32 * v);
#define VEC_SHRINK_TO_FIT_DECL(type, _) r vec_shrink_to_fit_##type(vec_##type * v);
#define vec_shrink_to_fit(v) VEC_FUNC1(shrink_to_fit, v)

// r vec_reserve_i32(vec_i32 * v, 123);
#define VEC_RESERVE_DECL(type, _) r vec_reserve_##type(vec_##type * v, size_t capacity);
#define vec_reserve(v, c) VEC_FUNC2(reserve, v, c)

// INLINE size_t vec_size_i32(vec_i32 * v);
#define VEC_SIZE_DECL(type, _)                      \
    INLINE size_t vec_size_##type(vec_##type * v) { \
        return v->_size;                            \
    }
#define vec_size(v) VEC_FUNC1(size, v)

#define VEC_CAPACITY_DECL(type, _)                      \
    INLINE size_t vec_capacity_##type(vec_##type * v) { \
        return v->_capacity;                            \
    }
#define vec_capacity(v) VEC_FUNC1(capacity, v)

// resize the vector. behave the same as STL std::vector::resize.
#define VEC_RESIZE_DECL(type, _) r vec_resize_##type(vec_##type * v, size_t new_size);
#define vec_resize(v, c) VEC_FUNC2(resize, v, c)
// reduce the vector to 'size' elements
#define vec_resize_smaller_unchecked(v, size) ((v)->_size = size)

// memmove inside a vector. from and to can be overlapped.
#define VEC_MEMMOVE_DECL(type, _) r vec_memmove_##type(vec_##type * v, size_t dst, size_t src, size_t size);
#define vec_memmove(v, dst, src, size) VEC_FUNC4(memmove, dst, src, size)

#define VEC_MEMMOVE_UNCHECKED_DECL(type, _)                                                         \
    INLINE void vec_memmove_unchecked_##type(vec_##type * v, size_t dst, size_t src, size_t size) { \
        memmove((v)->_data + dst, (v)->_data + src, size * sizeof(type));                           \
    }

// duplicate a vector
#define VEC_DUP_DECL(type, _) r vec_dup_##type(vec_##type * dst, vec_##type * src);
#define vec_dup(dst, src) VEC_FUNC2(dup, dst, src)

// the "at" is unchecked to avoid performance hit. You should
// always check the vector _size before calling it.
// INLINE i32 * vec_at_i32(vec_i32 * v, size_t idx);
#define VEC_AT_DECL(type, _)                                           \
    INLINE vec_iter_##type vec_at_##type(vec_##type * v, size_t idx) { \
        assert(idx < v->_size);                                        \
        return v->_data + idx;                                         \
    }
#define vec_at(v, i) VEC_FUNC2(at, v, i)

#define VEC_BACK_DECL(type, _)                               \
    INLINE vec_iter_##type vec_back_##type(vec_##type * v) { \
        assert(v->_size);                                    \
        return v->_data + v->_size - 1;                      \
    }
#define vec_back(v) VEC_FUNC1(back, v)

#define vec_set_fixed(v, f) ((v)->fixed = f)

// Unchecked!!
#define VEC_FOR_EACH(vec, type, iter) \
    for (vec_iter_##type iter = (vec)->_data, _end_##iter = ((vec)->_data + (vec)->_size); iter < _end_##iter; iter++)

#define VEC_FOR_EACH_REVERSE(vec, type, iter) \
    for (vec_iter_##type iter = ((vec)->_data + (vec)->_size - 1); ((vec)->_size) && (iter >= (vec)->_data); iter--)

// well, using a temp variable is better, but it needs one more type parameter.
#define VEC_REVERSE(vec)                                               \
    for (size_t _sz_ = (vec)->_size / 2, _i_ = 0; _i_ < _sz_; _i_++) { \
        (vec)->_data[_i_] ^= (vec)->_data[(vec)->_size - 1 - _i_];     \
        (vec)->_data[(vec)->_size - 1 - _i_] ^= (vec)->_data[_i_];     \
        (vec)->_data[_i_] ^= (vec)->_data[(vec)->_size - 1 - _i_];     \
    }

#define VEC_DECL_FOR_TYPE_BUILTIN(type, _) \
    VEC_TYPE_DECL(type, _)                 \
    VEC_IS_VALID_DECL(type, _)             \
    VEC_CLEAR_DECL(type, _)                \
    VEC_PUSH_DECL(type, _)                 \
    VEC_POP_DECL(type, _)                  \
    VEC_POPALL_DECL(type, _)               \
    VEC_SHRINK_TO_FIT_DECL(type, _)        \
    VEC_RESERVE_DECL(type, _)              \
    VEC_SIZE_DECL(type, _)                 \
    VEC_CAPACITY_DECL(type, _)             \
    VEC_RESIZE_DECL(type, _)               \
    VEC_MEMMOVE_DECL(type, _)              \
    VEC_MEMMOVE_UNCHECKED_DECL(type, _)    \
    VEC_DUP_DECL(type, _)                  \
    VEC_AT_DECL(type, _)                   \
    VEC_BACK_DECL(type, _)

#define VEC_DECL_FOR_TYPE(type) VEC_DECL_FOR_TYPE_BUILTIN(type, _)
FOR_EACH_TYPE(VEC_DECL_FOR_TYPE_BUILTIN, _)

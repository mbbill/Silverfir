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

#pragma once

#include "alloc.h"
#include "result.h"
#include "silverfir.h"
#include "types.h"
#include "vec.h"

#include <alloc.h>
#include <assert.h>
#include <stdbool.h>
#include <string.h>

// A buffer type. It must be created from a constant, e.g. the pointer is point to a
// static memory like string literal which never require deallocation, or from a long
// lived buffer which lives at least longer than the str itself.
// The str is not necessarily zero-ended.

// Again, this is a BUFFER type, although it's called 'str'. It's perfectly fine to have
// zeros in the str and it's safe to do so because it's length-encoded.

typedef const u8 * str_iter_t;
typedef struct str {
    str_iter_t ptr;
    size_t len;
} str;

RESULT_TYPE_DECL(str)
VEC_DECL_FOR_TYPE(str)

#define STR_NULL ((str){0})

// useful when we need to build a zero-ended unix string from str.
#define STR_ZERO_END               \
    ((str){                        \
        .len = 1,                  \
        .ptr = (str_iter_t)("\0"), \
    })

INLINE bool str_is_valid(str s) {
    return ((s.ptr != NULL && s.len != 0) || (s.ptr == NULL && s.len == 0));
}

INLINE str str_from(str_iter_t p, size_t len) {
    assert(p);
    if (!len) {
        return STR_NULL;
    }
    return (str){.ptr = p, .len = len};
}

// build a str from a string literal. e.g. s("abc")
// You MUST NOT use this ctor on heap allocated buffers, otherwise it's going to be leaked.
// The produced str is always non-zero-terminated
#define s(pstr) ((str){.ptr = (str_iter_t)(&(pstr)[0]), .len = strnlen((const char *)pstr, sizeof(pstr))})

// str from a p(pointer)
// char * a = "1234";
// str sa = s_p(a);
#define s_p(p) str_from((str_iter_t)(&(p)[0]), strlen((const char *)(p)))

// build s str from p(pointer) + l(length)
#define s_pl(p, len) str_from((str_iter_t)(&(p)[0]), len)

INLINE bool str_eq(str s1, str s2) {
    assert(str_is_valid(s1) && str_is_valid(s2));
    return s1.len == s2.len && !memcmp(s1.ptr, s2.ptr, s1.len);
}

INLINE bool str_is_null(str s) {
    assert(str_is_valid(s));
    return !s.len;
}

INLINE size_t str_len(str s) {
    return s.len;
}

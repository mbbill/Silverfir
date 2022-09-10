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

#if defined(__GNUC__) || defined(__clang__)
    #include "gcc_clang.h"
#elif defined(_MSC_VER)
    #include "msvc.h"
#endif

#if !defined(likely)
    #define likely(expr) (expr)
#endif

#if !defined(unlikely)
    #define unlikely(expr) (expr)
#endif

#define UNUSED(...) (void)(__VA_ARGS__)

#if (__STDC_VERSION__) >= 201112L
    #define STATIC_ASSERT(COND, MSG) _Static_assert(COND, #MSG)
#else
    #define STATIC_ASSERT(COND, MSG) typedef char static_assertion_##MSG[(COND) ? 1 : -1]
#endif

#define TO_WRAPPER(type, member, p_inner) ((type *)((char *)(p_inner)-offsetof(type, member)))

#define array_len(array) (sizeof(array) / sizeof(array[0]))

INLINE int is_little_endian() {
    int test = 1;
    return ((char *)(&test))[0] == 1;
}

#define ARRAY_FOR_EACH(array, type, iter) \
    for (type * iter = &array[0]; iter < (&array[0] + array_len(array)); iter++)

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
#define SRC_LOCATION __FILE__ ":" TOSTRING(__LINE__)

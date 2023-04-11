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

#if defined(_MSC_VER) // clang for windows
    #define _CRTDBG_MAP_ALLOC
    #include <crtdbg.h>
    #include <malloc.h>
#endif

#include <assert.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define likely(expr) __builtin_expect(!!(expr), 1)
#define unlikely(expr) __builtin_expect(!!(expr), 0)

#if !defined(NDEBUG)
    #define debug_trap() __builtin_trap()
#else
    #define debug_trap()
#endif

#define s_clz32(x) (x == 0 ? 32 : __builtin_clz(x))
#define s_clz64(x) (x == 0 ? 64 : __builtin_clzll(x))
#define s_ctz32(x) (x == 0 ? 32 : __builtin_ctz(x))
#define s_ctz64(x) (x == 0 ? 64 : __builtin_ctzll(x))
#define s_popcnt32(x) __builtin_popcount(x)
#define s_popcnt64(x) __builtin_popcountll(x)

#define HAS_COMPUTED_GOTO

#define NOINLINE __attribute__((noinline))
#define MUSTTAIL __attribute__((musttail))
#define INLINE __attribute__((always_inline)) static inline

#if defined(__clang__)
#define UNUSED_FUNCTION_WARNING_PUSH _Pragma("clang diagnostic push") _Pragma("clang diagnostic ignored \"-Wunused-function\"")
#define UNUSED_FUNCTION_WARNING_POP _Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
#define UNUSED_FUNCTION_WARNING_PUSH _Pragma("GCC diagnostic push") _Pragma("GCC diagnostic ignored \"-Wunused-function\"")
#define UNUSED_FUNCTION_WARNING_POP _Pragma("GCC diagnostic pop")
#else
#define UNUSED_FUNCTION_WARNING_PUSH
#define UNUSED_FUNCTION_WARNING_POP
#endif

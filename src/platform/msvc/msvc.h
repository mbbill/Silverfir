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

#define _CRTDBG_MAP_ALLOC

#include <assert.h>
#include <crtdbg.h>
#include <malloc.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define likely(expr) (expr)
#define unlikely(expr) (expr)

#if !defined(NDEBUG)
    #define debug_trap() __debugbreak()
#else
    #define debug_trap()
#endif

#define s_clz32(x) __lzcnt(x)
#define s_clz64(x) __lzcnt64(x)
#define s_ctz32(x) _tzcnt_u32(x)
#define s_ctz64(x) _tzcnt_u64(x)
#define s_popcnt32(x) __popcnt(x)
#define s_popcnt64(x) __popcnt64(x)

#define NOINLINE __declspec(noinline)
#define MUSTTAIL
#define INLINE __forceinline static

#define UNUSED_FUNCTION_WARNING_PUSH
#define UNUSED_FUNCTION_WARNING_POP

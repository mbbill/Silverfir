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

#include "silverfir.h"

#include <float.h>
#include <limits.h>
#include <stddef.h>
#include <stdint.h>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef uintptr_t uptr;

typedef i8 * i8p;
typedef i16 * i16p;
typedef i32 * i32p;
typedef i64 * i64p;
typedef u8 * u8p;
typedef u16 * u16p;
typedef u32 * u32p;
typedef u64 * u64p;
typedef f32 * f32p;
typedef f64 * f64p;

#define i8_MAX INT8_MAX
#define i8_MIN INT8_MIN
#define i16_MAX INT16_MAX
#define i16_MIN INT16_MIN
#define i32_MAX INT32_MAX
#define i32_MIN INT32_MIN
#define i64_MAX INT64_MAX
#define i64_MIN INT64_MIN
#define u8_MAX UINT8_MAX
#define u8_MIN 0
#define u16_MAX UINT16_MAX
#define u16_MIN 0
#define u32_MAX UINT32_MAX
#define u32_MIN 0
#define u64_MAX UINT64_MAX
#define u64_MIN 0
#define uptr_MAX UINTPTR_MAX

#define FOR_EACH_TYPE(macro, extra) \
    macro(i8, extra)                \
    macro(i16, extra)               \
    macro(i32, extra)               \
    macro(i64, extra)               \
    macro(u8, extra)                \
    macro(u16, extra)               \
    macro(u32, extra)               \
    macro(u64, extra)               \
    macro(f32, extra)               \
    macro(f64, extra)               \
    macro(i8p, extra)               \
    macro(i16p, extra)              \
    macro(i32p, extra)              \
    macro(i64p, extra)              \
    macro(u8p, extra)               \
    macro(u16p, extra)              \
    macro(u32p, extra)              \
    macro(u64p, extra)              \
    macro(f32p, extra)              \
    macro(f64p, extra)              \
    macro(size_t, extra)            \
    macro(uptr, extra)

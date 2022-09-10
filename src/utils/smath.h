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

#include "compiler.h"
#include "silverfir.h"
#include "types.h"

#include <math.h>

static const u32 s_qnan32 = 0x7fc00000U;
static const u32 s_qnan32_neg = 0xffc00000U;

static const u64 s_qnan64 = 0x7ff8000000000000ULL;
static const u64 s_qnan64_neg = 0xfff8000000000000ULL;

INLINE bool s_is_canonical_nan32(f32 v) {
    return (*((u32 *)(&v)) == s_qnan32) || (*((u32 *)(&v)) == s_qnan32_neg);
}

INLINE bool s_is_canonical_nan64(f64 v) {
    return (*((u64 *)(&v)) == s_qnan64) || (*((u64 *)(&v)) == s_qnan64_neg);
}

INLINE bool s_is_arithmetic_nan32(f32 v) {
    return (*((u32 *)(&v)) == s_qnan32);
}

INLINE bool s_is_arithmetic_nan64(f64 v) {
    return (*((u64 *)(&v)) == s_qnan64);
}

INLINE bool s_isnan32(f32 v) {
    return ((*((u32 *)(&v)) & 0x7f800000U) == 0x7f800000U) && ((*((u32 *)(&v)) & 0x7fffffffU) != 0x7f800000U);
}

INLINE bool s_isnan64(f64 v) {
    return ((*((u64 *)(&v)) & 0x7ff0000000000000U) == 0x7ff0000000000000U) && ((*((u64 *)(&v)) & 0x7fffffffffffffffU) != 0x7ff0000000000000U);
}

INLINE u32 s_rotr32(u32 v, u32 n) {
    return (v >> n % 32) | (v << (32 - n) % 32);
}

INLINE u32 s_rotl32(u32 v, u32 n) {
    return (v << n % 32) | (v >> (32 - n) % 32);
}

INLINE u64 s_rotr64(u64 v, u64 n) {
    return (v >> n % 64) | (v << (64 - n) % 64);
}

INLINE u64 s_rotl64(u64 v, u64 n) {
    return (v << n % 64) | (v >> (64 - n) % 64);
}

INLINE f32 s_fneg32(f32 v) {
    return -v;
}

INLINE f64 s_fneg64(f64 v) {
    return -v;
}

#define s_add(a, b) ((a) + (b))
#define s_sub(a, b) ((a) - (b))
#define s_mul(a, b) ((a) * (b))
#define s_div(a, b) ((a) / (b))
#define s_rem(a, b) ((a) % (b))
#define s_and(a, b) ((a) & (b))
#define s_or(a, b) ((a) | (b))
#define s_xor(a, b) ((a) ^ (b))
#define s_shl(a, b) ((a) << (b))
#define s_shr(a, b) ((a) >> (b))

#define s_eqz(a) ((a) == 0)
#define s_eq(a, b) (a == b)
#define s_ne(a, b) (a != b)
#define s_lt(a, b) (a < b)
#define s_gt(a, b) (a > b)
#define s_le(a, b) (a <= b)
#define s_ge(a, b) (a >= b)

#define s_nop(x) (x)

INLINE f32 s_fabs32(f32 x) {
    u32 u = *((u32 *)&x) & 0x7fffffffU;
    return *((f32 *)&u);
}

INLINE f64 s_fabs64(f64 x) {
    u64 u = *((u64 *)&x) & 0x7fffffffffffffffU;
    return *((f64 *)&u);
}

#define s_ceil ceil
#define s_floor floor
#define s_trunc trunc
#define s_rint rint
#define s_sqrt sqrt

#define bit_eq32(x, y) (!(*((u32 *)(&x)) ^ *((u32 *)(&y))))
#define bit_eq64(x, y) (!(*((u64 *)(&x)) ^ *((u64 *)(&y))))

// clang-format off
INLINE f32 s_fmin32(f32 x, f32 y) {
    return unlikely(x != x) ? s_qnan32 :
           unlikely(y != y) ? s_qnan32 :
           unlikely((x == 0.) && (y == 0.)) ?
           (bit_eq32(x, y) ? x : -0.f) : (x < y) ? x : y;
}

INLINE f32 s_fmax32(f32 x, f32 y) {
    return unlikely(x != x) ? s_qnan32 :
           unlikely(y != y) ? s_qnan32 :
           unlikely((x == 0.) && (y == 0.)) ?
           (bit_eq32(x, y) ? x : 0.f) : (x > y) ? x : y;
}

INLINE f64 s_fmin64(f64 x, f64 y) {
    return unlikely(x != x) ? s_qnan64 :
           unlikely(y != y) ? s_qnan64 :
           unlikely((x == 0.) && (y == 0.)) ?
           (bit_eq64(x, y) ? x : -0.) : (x < y) ? x : y;
}

INLINE f64 s_fmax64(f64 x, f64 y) {
    return unlikely(x != x) ? s_qnan64 :
           unlikely(y != y) ? s_qnan64 :
           unlikely((x == 0.) && (y == 0.)) ?
           (bit_eq64(x, y) ? x : 0.) : (x > y) ? x : y;
}
// clang-format on

INLINE f32 s_copysign32(f32 x, f32 sign) {
    u32 u = (*((u32 *)&sign) & 0x80000000U) | (*((u32 *)&x) & 0x7fffffffU);
    return *((f32 *)&u);
}

INLINE f64 s_copysign64(f64 x, f64 sign) {
    u64 u = (*((u64 *)&sign) & 0x8000000000000000U) | (*((u64 *)&x) & 0x7fffffffffffffffU);
    return *((f64 *)&u);
}

#define s_truncf32i(x) ((i64)(truncf(x)))
#define s_truncf64i(x) ((i64)(trunc(x)))
#define s_truncf32u(x) ((u64)(truncf(x)))
#define s_truncf64u(x) ((u64)(trunc(x)))

#define s_extend8(x) (*((i8 *)(&x)))
#define s_extend16(x) (*((i16 *)(&x)))
#define s_extend32(x) (*((i32 *)(&x)))
#define s_extend32u(x) (*((u32 *)(&x)))

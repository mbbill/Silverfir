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

#include "compiler.h"
#include "smath.h"
#include "test_containers.h"

#include <cmocka.h>
#include <cmocka_private.h>


static void smath_test1(void ** state) {
    assert_int_equal(s_clz64(0xffffffffffffffffu), 0);
    assert_int_equal(s_clz64(0), 64);
    assert_int_equal(s_clz64(0x00008000u), 48);
    assert_int_equal(s_clz64(0xffu), 56);
    assert_int_equal(s_clz64(0x8000000000000000u), 0);
    assert_int_equal(s_clz64(1), 63);
    assert_int_equal(s_clz64(0x7fffffffffffffffu), 1);
}

static void test_s_is_canonical_nan32(void **state) {
    (void) state; // unused

    f32 qnan = *((f32 *)(&s_qnan32));
    f32 qnan_neg = *((f32 *)(&s_qnan32_neg));
    f32 non_qnan = 3.14f;

    assert_true(s_is_canonical_nan32(qnan));
    assert_true(s_is_canonical_nan32(qnan_neg));
    assert_false(s_is_canonical_nan32(non_qnan));
}

static void test_s_is_canonical_nan64(void **state) {
    (void) state; // unused

    f64 qnan = *((f64 *)(&s_qnan64));
    f64 qnan_neg = *((f64 *)(&s_qnan64_neg));
    f64 non_qnan = 3.14;

    assert_true(s_is_canonical_nan64(qnan));
    assert_true(s_is_canonical_nan64(qnan_neg));
    assert_false(s_is_canonical_nan64(non_qnan));
}

static void test_s_is_arithmetic_nan32(void **state) {
    (void) state; // unused

    f32 qnan = *((f32 *)(&s_qnan32));
    f32 qnan_neg = *((f32 *)(&s_qnan32_neg));
    f32 non_qnan = 3.14f;

    assert_true(s_is_arithmetic_nan32(qnan));
    assert_false(s_is_arithmetic_nan32(qnan_neg));
    assert_false(s_is_arithmetic_nan32(non_qnan));
}

static void test_s_is_arithmetic_nan64(void **state) {
    (void) state; // unused

    f64 qnan = *((f64 *)(&s_qnan64));
    f64 qnan_neg = *((f64 *)(&s_qnan64_neg));
    f64 non_qnan = 3.14;

    assert_true(s_is_arithmetic_nan64(qnan));
    assert_false(s_is_arithmetic_nan64(qnan_neg));
    assert_false(s_is_arithmetic_nan64(non_qnan));
}

static void test_s_isnan32(void **state) {
    (void) state; // unused

    f32 qnan = *((f32 *)(&s_qnan32));
    f32 qnan_neg = *((f32 *)(&s_qnan32_neg));
    f32 non_qnan = 3.14f;

    assert_true(s_isnan32(qnan));
    assert_true(s_isnan32(qnan_neg));
    assert_false(s_isnan32(non_qnan));
}

static void test_s_isnan64(void **state) {
    (void) state; // unused

    f64 qnan = *((f64 *)(&s_qnan64));
    f64 qnan_neg = *((f64 *)(&s_qnan64_neg));
    f64 non_qnan = 3.14;

    assert_true(s_isnan64(qnan));
    assert_true(s_isnan64(qnan_neg));
    assert_false(s_isnan64(non_qnan));
}

static void test_s_rotr32(void**state) {
    (void) state; // unused

    u32 value = 0x12345678;
    u32 rot_8 = 0x78123456;
    u32 rot_16 = 0x56781234;
    u32 rot_24 = 0x34567812;
    u32 rot_32 = value;

    assert_int_equal(s_rotr32(value, 8), rot_8);
    assert_int_equal(s_rotr32(value, 16), rot_16);
    assert_int_equal(s_rotr32(value, 24), rot_24);
    assert_int_equal(s_rotr32(value, 32), rot_32);
}

static void test_s_rotl32(void **state) {
    (void) state; // unused

    u32 value = 0x12345678;
    u32 rot_8 = 0x34567812;
    u32 rot_16 = 0x56781234;
    u32 rot_24 = 0x78123456;
    u32 rot_32 = value;

    assert_int_equal(s_rotl32(value, 8), rot_8);
    assert_int_equal(s_rotl32(value, 16), rot_16);
    assert_int_equal(s_rotl32(value, 24), rot_24);
    assert_int_equal(s_rotl32(value, 32), rot_32);
}

static void test_s_rotr64(void **state) {
    (void) state; // unused

    u64 value = 0x123456789ABCDEF0ULL;
    u64 rot_16 = 0xDEF0123456789ABCULL;
    u64 rot_32 = 0x9ABCDEF012345678ULL;
    u64 rot_48 = 0x56789ABCDEF01234ULL;
    u64 rot_64 = value;

    assert_int_equal(s_rotr64(value, 16), rot_16);
    assert_int_equal(s_rotr64(value, 32), rot_32);
    assert_int_equal(s_rotr64(value, 48), rot_48);
    assert_int_equal(s_rotr64(value, 64), rot_64);
}

static void test_s_rotl64(void **state) {
    (void) state; // unused

    u64 value = 0x123456789ABCDEF0ULL;
    u64 rot_16 = 0x56789ABCDEF01234ULL;
    u64 rot_32 = 0x9ABCDEF012345678ULL;
    u64 rot_48 = 0xDEF0123456789ABCULL;
    u64 rot_64 = value;

    assert_int_equal(s_rotl64(value, 16), rot_16);
    assert_int_equal(s_rotl64(value, 32), rot_32);
    assert_int_equal(s_rotl64(value, 48), rot_48);
    assert_int_equal(s_rotl64(value, 64), rot_64);
}

static void test_s_fneg32(void **state) {
    (void) state; // unused

    f32 value = 3.14f;
    f32 neg_value = -3.14f;
    f32 value_zero = 0.0f;
    f32 neg_zero = -0.0f;

    assert_float_equal(s_fneg32(value), neg_value, FLT_EPSILON);
    assert_float_equal(s_fneg32(neg_value), value, FLT_EPSILON);
    assert_float_equal(s_fneg32(value_zero), neg_zero, FLT_EPSILON);
    assert_float_equal(s_fneg32(neg_zero), value_zero, FLT_EPSILON);
}

static void test_s_fneg64(void **state) {
    (void) state; // unused

    f64 value = 3.14;
    f64 neg_value = -3.14;
    f64 value_zero = 0.0;
    f64 neg_zero = -0.0;

    assert_float_equal(s_fneg64(value), neg_value, DBL_EPSILON);
    assert_float_equal(s_fneg64(neg_value), value, DBL_EPSILON);
    assert_float_equal(s_fneg64(value_zero), neg_zero, DBL_EPSILON);
    assert_float_equal(s_fneg64(neg_zero), value_zero, DBL_EPSILON);
}

static void test_s_add(void** state) {
    (void) state;

    assert_int_equal(s_add(0, 0), 0);
    assert_int_equal(s_add(1, 1), 2);
    assert_int_equal(s_add(-1, 1), 0);
    // assert_int_equal(s_add(INT32_MAX, 1), INT32_MIN);
    // assert_int_equal(s_add(INT32_MIN, -1), INT32_MAX);
}

static void test_s_sub(void** state) {
    (void) state;

    assert_int_equal(s_sub(0, 0), 0);
    assert_int_equal(s_sub(1, 1), 0);
    assert_int_equal(s_sub(2, 1), 1);
    // assert_int_equal(s_sub(INT32_MAX, 1), INT32_MAX - 1);
    // assert_int_equal(s_sub(INT32_MIN, -1), INT32_MIN + 1);
}

static void test_s_mul(void** state) {
    (void) state;

    assert_int_equal(s_mul(0, 0), 0);
    assert_int_equal(s_mul(1, 1), 1);
    assert_int_equal(s_mul(-1, 1), -1);
    assert_int_equal(s_mul(2, 3), 6);
    // assert_int_equal(s_mul(INT32_MAX, 1), INT32_MAX);
}

static void test_s_div(void** state) {
    (void) state;

    assert_int_equal(s_div(1, 1), 1);
    assert_int_equal(s_div(-1, 1), -1);
    assert_int_equal(s_div(6, 2), 3);
    // assert_int_equal(s_div(INT32_MAX, 1), INT32_MAX);
    // assert_int_equal(s_div(INT32_MIN, -1), INT32_MAX);
}

static void test_s_rem(void** state) {
    (void) state;

    assert_int_equal(s_rem(0, 1), 0);
    assert_int_equal(s_rem(1, 1), 0);
    assert_int_equal(s_rem(4, 2), 0);
    assert_int_equal(s_rem(5, 2), 1);
}

static void test_s_and(void** state) {
    (void) state;

    assert_int_equal(s_and(0, 0), 0);
    assert_int_equal(s_and(1, 1), 1);
    assert_int_equal(s_and(3, 1), 1);
    assert_int_equal(s_and(3, 2), 2);
}

static void test_s_or(void** state) {
    (void) state;

    assert_int_equal(s_or(0, 0), 0);
    assert_int_equal(s_or(1, 1), 1);
    assert_int_equal(s_or(3, 1), 3);
    assert_int_equal(s_or(3, 2), 3);
}

static void test_s_xor(void** state) {
    (void) state;

    assert_int_equal(s_xor(0, 0), 0);
    assert_int_equal(s_xor(1, 1), 0);
    assert_int_equal(s_xor(3, 1), 2);
    assert_int_equal(s_xor(3, 2), 1);
}

static void test_s_shl(void** state) {
    (void) state;

    assert_int_equal(s_shl(1, 0), 1);
    assert_int_equal(s_shl(1, 1), 2);
    assert_int_equal(s_shl(1, 2), 4);
    assert_int_equal(s_shl(3, 1), 6);
    // assert_int_equal(s_shl(INT32_MAX, 1), INT32_MIN + 2);
    // assert_int_equal(s_shl(INT32_MIN, 1), 0);
}

static void test_s_shr(void** state) {
    (void) state;

    assert_int_equal(s_shr(1, 0), 1);
    assert_int_equal(s_shr(1, 1), 0);
    assert_int_equal(s_shr(4, 1), 2);
    assert_int_equal(s_shr(6, 1), 3);
    // assert_int_equal(s_shr(INT32_MAX, 1), INT32_MAX / 2);
    // assert_int_equal(s_shr(INT32_MIN, 1), INT32_MIN / 2);
}

static void test_s_fabs32_inf(void **state) {
    float input = INFINITY;
    float result = s_fabs32(input);
    assert_true(isinf(result));
}

static void test_s_fabs32_neg_inf(void **state) {
    float input = -INFINITY;
    float result = s_fabs32(input);
    assert_true(isinf(result));
}

static void test_s_fabs32_nan(void **state) {
    float input = NAN;
    float result = s_fabs32(input);
    assert_true(isnan(result));
}

static void test_s_fabs32_zero(void **state) {
    float input = 0.0f;
    float result = s_fabs32(input);
    assert_true(result == input);
}

static void test_s_fabs32_neg(void **state) {
    float input = -10.0f;
    float expected = 10.0f;
    float result = s_fabs32(input);
    assert_true(fabsf(result - expected) < FLT_EPSILON);
}

static void test_s_fabs32_pos(void **state) {
    float input = 10.0f;
    float expected = 10.0f;
    float result = s_fabs32(input);
    assert_true(result == expected);
}

static void test_s_fabs64_inf(void **state) {
    double input = INFINITY;
    double result = s_fabs64(input);
    assert_true(isinf(result));
}

static void test_s_fabs64_neg_inf(void **state) {
    double input = -INFINITY;
    double result = s_fabs64(input);
    assert_true(isinf(result));
}

static void test_s_fabs64_nan(void **state) {
    double input = NAN;
    double result = s_fabs64(input);
    assert_true(isnan(result));
}

static void test_s_fabs64_zero(void **state) {
    double input = 0.0;
    double result = s_fabs64(input);
    assert_true(result == input);
}

static void test_s_fabs64_neg(void **state) {
    double input = -10.0;
    double expected = 10.0;
    double result = s_fabs64(input);
    assert_true(fabs(result - expected) < DBL_EPSILON);
}

static void test_s_fabs64_pos(void **state) {
    double input = 10.0;
    double expected = 10.0;
    double result = s_fabs64(input);
    assert_true(result == expected);
}

static void test_s_ceil(void **state) {
    assert_float_equal(s_ceil(0.0), 0.0, 0.001);
    assert_float_equal(s_ceil(3.5), 4.0, 0.001);
    assert_float_equal(s_ceil(-3.5), -3.0, 0.001);
    assert_float_equal(s_ceil(1.0), 1.0, 0.001);
    assert_float_equal(s_ceil(-1.0), -1.0, 0.001);
}

static void test_s_floor(void **state) {
    assert_float_equal(s_floor(0.0), 0.0, 0.001);
    assert_float_equal(s_floor(3.5), 3.0, 0.001);
    assert_float_equal(s_floor(-3.5), -4.0, 0.001);
    assert_float_equal(s_floor(1.0), 1.0, 0.001);
    assert_float_equal(s_floor(-1.0), -1.0, 0.001);
}

static void test_s_trunc(void **state) {
    assert_float_equal(s_trunc(0.0), 0.0, 0.001);
    assert_float_equal(s_trunc(3.5), 3.0, 0.001);
    assert_float_equal(s_trunc(-3.5), -3.0, 0.001);
    assert_float_equal(s_trunc(1.0), 1.0, 0.001);
    assert_float_equal(s_trunc(-1.0), -1.0, 0.001);
}

static void test_s_rint(void **state) {
    assert_float_equal(s_rint(0.0), 0.0, 0.001);
    assert_float_equal(s_rint(3.5), 4.0, 0.001);
    assert_float_equal(s_rint(-3.5), -4.0, 0.001);
    assert_float_equal(s_rint(1.0), 1.0, 0.001);
    assert_float_equal(s_rint(-1.0), -1.0, 0.001);
}

static void test_s_sqrt(void **state) {
    assert_float_equal(s_sqrt(0.0), 0.0, 0.001);
    assert_float_equal(s_sqrt(1.0), 1.0, 0.001);
    assert_float_equal(s_sqrt(4.0), 2.0, 0.001);
    assert_float_equal(s_sqrt(9.0), 3.0, 0.001);
    assert_float_equal(s_sqrt(16.0), 4.0, 0.001);
    assert_false(isnormal(s_sqrt(-1.0))); // Check if the result is NaN or an infinity
}

static void test_s_fmin32(void **state) {
    (void) state; // unused

    assert_float_equal(s_fmin32(1.f, 2.f), 1.f, FLT_EPSILON);
    assert_float_equal(s_fmin32(-1.f, 2.f), -1.f, FLT_EPSILON);
    assert_float_equal(s_fmin32(0.f, -0.f), -0.f, FLT_EPSILON);
    assert_float_equal(s_fmin32(-0.f, 0.f), -0.f, FLT_EPSILON);
    assert_float_equal(s_fmin32(NAN, 1.f), s_qnan32, FLT_EPSILON);
    assert_float_equal(s_fmin32(1.f, NAN), s_qnan32, FLT_EPSILON);
    assert_float_equal(s_fmin32(INFINITY, 1.f), 1.f, FLT_EPSILON);
    assert_float_equal(s_fmin32(-INFINITY, 1.f), -INFINITY, FLT_EPSILON);
}

static void test_s_fmax32(void **state) {
    (void) state; // unused

    assert_float_equal(s_fmax32(1.f, 2.f), 2.f, FLT_EPSILON);
    assert_float_equal(s_fmax32(-1.f, 2.f), 2.f, FLT_EPSILON);
    assert_float_equal(s_fmax32(0.f, -0.f), 0.f, FLT_EPSILON);
    assert_float_equal(s_fmax32(-0.f, 0.f), 0.f, FLT_EPSILON);
    assert_float_equal(s_fmax32(NAN, 1.f), s_qnan32, FLT_EPSILON);
    assert_float_equal(s_fmax32(1.f, NAN), s_qnan32, FLT_EPSILON);
    assert_float_equal(s_fmax32(INFINITY, 1.f), INFINITY, FLT_EPSILON);
    assert_float_equal(s_fmax32(-INFINITY, 1.f), 1.f, FLT_EPSILON);
}

static void test_s_fmin64(void **state) {
    (void) state; // unused

    assert_true(fabs(s_fmin64(1., 2.) - 1.) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(-1., 2.) - (-1.)) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(0., -0.) - (-0.)) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(-0., 0.) - (-0.)) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(NAN, 1.) - s_qnan64) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(1., NAN) - s_qnan64) <= DBL_EPSILON);
    assert_true(fabs(s_fmin64(INFINITY, 1.) - 1.) <= DBL_EPSILON);
    assert_true(isinf(s_fmin64(-INFINITY,1.0)) && s_fmin64(-INFINITY,1.0) < 0);
}

static void test_s_fmax64(void ** state) {
    (void)state; // unused
    assert_true(fabs(s_fmax64(1., 2.) - 2.) <= DBL_EPSILON);
    assert_true(fabs(s_fmax64(-1., 2.) - 2.) <= DBL_EPSILON);
    assert_true(fabs(s_fmax64(0., -0.) - 0.) <= DBL_EPSILON);
    assert_true(fabs(s_fmax64(-0., 0.) - 0.) <= DBL_EPSILON);
    assert_true(fabs(s_fmax64(NAN, 1.) - s_qnan64) <= DBL_EPSILON);
    assert_true(fabs(s_fmax64(1., NAN) - s_qnan64) <= DBL_EPSILON);
    assert_true(isinf(s_fmax64(INFINITY,1.0)) && s_fmax64(INFINITY,1.0) > 0);
    assert_true(fabs(s_fmax64(-INFINITY, 1.) - 1.) <= DBL_EPSILON);
}

static void test_s_copysign32(void **state) {
    (void)state; // unused

    // Test with positive sign, positive number
    assert_true(s_copysign32(10.0f, 1.0f) == 10.0f);

    // Test with positive sign, negative number
    assert_true(s_copysign32(-10.0f, 1.0f) == 10.0f);

    // Test with negative sign, positive number
    assert_true(s_copysign32(10.0f, -1.0f) == -10.0f);

    // Test with negative sign, negative number
    assert_true(s_copysign32(-10.0f, -1.0f) == -10.0f);

    // Test with positive zero
    assert_true(s_copysign32(0.0f, 1.0f) == 0.0f);

    // Test with negative zero
    assert_true(s_copysign32(-0.0f, 1.0f) == 0.0f);

    // Test with NaN
    assert_true(isnan(s_copysign32(NAN, 1.0f)));

    // Test with positive infinity
    assert_true(s_copysign32(INFINITY, 1.0f) == INFINITY);

    // Test with negative infinity
    assert_true(s_copysign32(-INFINITY, 1.0f) == INFINITY);
}

static void test_s_copysign64(void **state) {
    (void)state; // unused

    // Test with positive sign, positive number
    assert_true(s_copysign64(10.0, 1.0) == 10.0);

    // Test with positive sign, negative number
    assert_true(s_copysign64(-10.0, 1.0) == 10.0);

    // Test with negative sign, positive number
    assert_true(s_copysign64(10.0, -1.0) == -10.0);

    // Test with negative sign, negative number
    assert_true(s_copysign64(-10.0, -1.0) == -10.0);

    // Test with positive zero
    assert_true(s_copysign64(0.0, 1.0) == 0.0);

    // Test with negative zero
    assert_true(s_copysign64(-0.0, 1.0) == -0.0);

    // Test with NaN
    assert_true(isnan(s_copysign64(NAN, 1.0)));

    // Test with positive infinity
    assert_true(s_copysign64(INFINITY, 1.0) == INFINITY);

    // Test with negative infinity
    assert_true(s_copysign64(-INFINITY, 1.0) == INFINITY);
}

static void test_s_truncf32i(void **state) {
    assert_int_equal(s_truncf32i(10.5f), 10);
    assert_int_equal(s_truncf32i(10.4f), 10);
    assert_int_equal(s_truncf32i(0.5f), 0);
    assert_int_equal(s_truncf32i(0.4f), 0);
    assert_int_equal(s_truncf32i(-10.5f), -10);
    assert_int_equal(s_truncf32i(-10.4f), -10);
    assert_int_equal(s_truncf32i(-0.5f), 0);
    assert_int_equal(s_truncf32i(-0.4f), 0);
}

static void test_s_truncf64i(void **state) {
    assert_int_equal(s_truncf64i(10.5), 10);
    assert_int_equal(s_truncf64i(10.4), 10);
    assert_int_equal(s_truncf64i(0.5), 0);
    assert_int_equal(s_truncf64i(0.4), 0);
    assert_int_equal(s_truncf64i(-10.5), -10);
    assert_int_equal(s_truncf64i(-10.4), -10);
    assert_int_equal(s_truncf64i(-0.5), 0);
    assert_int_equal(s_truncf64i(-0.4), 0);
}

static void test_s_truncf32u(void **state) {
    assert_int_equal(s_truncf32u(10.5f), 10);
    assert_int_equal(s_truncf32u(10.4f), 10);
    assert_int_equal(s_truncf32u(0.5f), 0);
    assert_int_equal(s_truncf32u(0.4f), 0);
    // The behavior of casting a negative floating-point number to an
    // unsigned integer is not well-defined according to the C standard.
    // It means that the result might be different between compilers or
    // even different versions of the same compiler.
    // assert_int_equal(s_truncf32u(-10.5f), (u64)(0xfffffffffffffff6));
    // assert_int_equal(s_truncf32u(-10.4f), (u64)(0xfffffffffffffff6));
    // assert_int_equal(s_truncf32u(-0.5f), 0);
    // assert_int_equal(s_truncf32u(-0.4f), 0);
}

static void test_s_truncf64u(void **state) {
    assert_int_equal(s_truncf64u(10.5), 10);
    assert_int_equal(s_truncf64u(10.4), 10);
    assert_int_equal(s_truncf64u(0.5), 0);
    assert_int_equal(s_truncf64u(0.4), 0);
    // assert_int_equal(s_truncf64u(-10.5), 18446744073709551606u);
    // assert_int_equal(s_truncf64u(-10.4), 18446744073709551606u);
    // assert_int_equal(s_truncf64u(-0.5), 0);
    // assert_int_equal(s_truncf64u(-0.4), 0);
}

void test_s_clz32(void **state) {
    assert_int_equal(s_clz32(0), 32);
    assert_int_equal(s_clz32(0x80000000), 0);
    assert_int_equal(s_clz32(0x40000000), 1);
    assert_int_equal(s_clz32(0x00000100), 23);
    assert_int_equal(s_clz32(0x00000001), 31);
}

void test_s_clz64(void **state) {
    assert_int_equal(s_clz64(0), 64);
    assert_int_equal(s_clz64(0x8000000000000000), 0);
    assert_int_equal(s_clz64(0x4000000000000000), 1);
    assert_int_equal(s_clz64(0x0000000000000100), 55);
    assert_int_equal(s_clz64(0x0000000000000001), 63);
}

void test_s_ctz32(void **state) {
    assert_int_equal(s_ctz32(0), 32);
    assert_int_equal(s_ctz32(0x80000000), 31);
    assert_int_equal(s_ctz32(0x40000000), 30);
    assert_int_equal(s_ctz32(0x00000100), 8);
    assert_int_equal(s_ctz32(0x00000001), 0);
}

void test_s_ctz64(void **state) {
    assert_int_equal(s_ctz64(0), 64);
    assert_int_equal(s_ctz64(0x8000000000000000), 63);
    assert_int_equal(s_ctz64(0x4000000000000000), 62);
    assert_int_equal(s_ctz64(0x0000000000000100), 8);
    assert_int_equal(s_ctz64(0x0000000000000001), 0);
}

void test_s_popcnt32(void **state) {
    assert_int_equal(s_popcnt32(0), 0);
    assert_int_equal(s_popcnt32(0xffffffff), 32);
    assert_int_equal(s_popcnt32(0xabcdef00), 17);
    assert_int_equal(s_popcnt32(0x00010001), 2);
}

void test_s_popcnt64(void **state) {
    assert_int_equal(s_popcnt64(0), 0);
    assert_int_equal(s_popcnt64(0xffffffffffffffff), 64);
    assert_int_equal(s_popcnt64(0xabcdef0000000000), 17);
    assert_int_equal(s_popcnt64(0x0001000100000001), 3);
}

struct CMUnitTest smath_tests[] = {
    cmocka_unit_test(smath_test1),
    cmocka_unit_test(test_s_is_canonical_nan32),
    cmocka_unit_test(test_s_is_canonical_nan64),
    cmocka_unit_test(test_s_is_arithmetic_nan32),
    cmocka_unit_test(test_s_is_arithmetic_nan64),
    cmocka_unit_test(test_s_isnan32),
    cmocka_unit_test(test_s_isnan64),
    cmocka_unit_test(test_s_rotr32),
    cmocka_unit_test(test_s_rotl32),
    cmocka_unit_test(test_s_rotr64),
    cmocka_unit_test(test_s_rotl64),
    cmocka_unit_test(test_s_fneg32),
    cmocka_unit_test(test_s_fneg64),
    cmocka_unit_test(test_s_add),
    cmocka_unit_test(test_s_sub),
    cmocka_unit_test(test_s_mul),
    cmocka_unit_test(test_s_div),
    cmocka_unit_test(test_s_rem),
    cmocka_unit_test(test_s_and),
    cmocka_unit_test(test_s_or),
    cmocka_unit_test(test_s_xor),
    cmocka_unit_test(test_s_shl),
    cmocka_unit_test(test_s_shr),
    cmocka_unit_test(test_s_fabs32_inf),
    cmocka_unit_test(test_s_fabs32_neg_inf),
    cmocka_unit_test(test_s_fabs32_nan),
    cmocka_unit_test(test_s_fabs32_zero),
    cmocka_unit_test(test_s_fabs32_neg),
    cmocka_unit_test(test_s_fabs32_pos),
    cmocka_unit_test(test_s_fabs64_inf),
    cmocka_unit_test(test_s_fabs64_neg_inf),
    cmocka_unit_test(test_s_fabs64_nan),
    cmocka_unit_test(test_s_fabs64_zero),
    cmocka_unit_test(test_s_fabs64_neg),
    cmocka_unit_test(test_s_fabs64_pos),
    cmocka_unit_test(test_s_ceil),
    cmocka_unit_test(test_s_floor),
    cmocka_unit_test(test_s_trunc),
    cmocka_unit_test(test_s_rint),
    cmocka_unit_test(test_s_sqrt),
    cmocka_unit_test(test_s_fmin32),
    cmocka_unit_test(test_s_fmax32),
    cmocka_unit_test(test_s_fmin64),
    cmocka_unit_test(test_s_fmax64),
    cmocka_unit_test(test_s_copysign32),
    cmocka_unit_test(test_s_copysign64),
    cmocka_unit_test(test_s_truncf32i),
    cmocka_unit_test(test_s_truncf64i),
    cmocka_unit_test(test_s_truncf32u),
    cmocka_unit_test(test_s_truncf64u),
    cmocka_unit_test(test_s_clz32),
    cmocka_unit_test(test_s_clz64),
    cmocka_unit_test(test_s_ctz32),
    cmocka_unit_test(test_s_ctz64),
    cmocka_unit_test(test_s_popcnt32),
    cmocka_unit_test(test_s_popcnt64),
};

const size_t smath_tests_count = array_len(smath_tests);


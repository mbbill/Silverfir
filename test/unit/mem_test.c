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
#include "mem_util.h"
#include "types.h"

#include <cmocka.h>
#include <cmocka_private.h>

#define to_u8p(x) ((u8 *)(x))

static void mem_test_read(void ** state) {
    {
        i16 i_16 = i16_MIN;
        i32 i_32 = mem_read_i16(to_u8p(&i_16));
        assert_int_equal(i_32, i16_MIN);
        i64 i_64 = mem_read_i16(to_u8p(&i_16));
        assert_int_equal(i_64, i16_MIN);
    }
    {
        i16 u_16 = i16_MAX;
        i32 i_32 = mem_read_i16(to_u8p(&u_16));
        assert_int_equal(i_32, i16_MAX);
        i64 i_64 = mem_read_i16(to_u8p(&u_16));
        assert_int_equal(i_64, i16_MAX);
    }
    {
        u32 u_32 = u32_MAX;
        i64 i_64 = mem_read_u32(to_u8p(&u_32));
        assert_int_equal(i_64, u32_MAX);
    }
    {
        f32 f_32 = 1.23456f;
        f32 f_32_2 = mem_read_f32(to_u8p(&f_32));
        assert_float_equal(f_32_2, 1.23456f, FLT_EPSILON);
    }
    {
        // test unaligned accesses.
        u64 buf[32] = {0};
        u8 * p = (u8 *)(buf);
        p++; // 1 byte aligned.
        p[0] = 0xaa;
        p[1] = 0xbb;
        p[2] = 0xcc;
        p[3] = 0xdd;
        p[4] = 0xee;
        p[5] = 0xff;
        i16 i_16 = mem_read_i16(p);
        assert_int_equal(i_16, (i16)(u16)(0xbbaa));
        i32 i_32 = mem_read_i32(p);
        assert_int_equal(i_32, (i32)(0xddccbbaa));
        p++; // 2 bytes aligned
        i64 i_64 = mem_read_i64(p);
        assert_int_equal(i_64, (i64)(0xffeeddccbb));
    }
}

static void mem_test_write(void ** state) {
    u64 buf[32] = {0};
    u8 * p = (u8 *)(buf);
    {
        mem_write_u8(p, 123);
        assert_int_equal(mem_read_u8(p), 123);

        mem_write_u8(p, (u8)((i8)(-123)));
        assert_int_equal(mem_read_i8(p), -123);

        mem_write_u16(p, 65534);
        assert_int_equal(mem_read_u16(p), 65534);

        mem_write_u16(p, (u16)((i16)(-32765)));
        assert_int_equal(mem_read_i16(p), -32765);

        mem_write_u32(p, 65534000);
        assert_int_equal(mem_read_u32(p), 65534000);

        mem_write_u32(p, (u32)((i32)(-32769)));
        assert_int_equal(mem_read_i32(p), -32769);

        mem_write_u64(p, u64_MAX);
        assert_int_equal(mem_read_u64(p), u64_MAX);

        mem_write_u64(p, (u64)((i64)(i64_MIN)));
        assert_int_equal(mem_read_u64(p), i64_MIN);
    }
    // unaligned
    memset(p, 0, sizeof(buf));
    {
        p++; // offset 1 byte to make it unaligned.

        mem_write_u8(p, 123);
        assert_int_equal(mem_read_u8(p), 123);

        mem_write_u8(p, (u8)((i8)(-123)));
        assert_int_equal(mem_read_i8(p), -123);

        mem_write_u16(p, 65534);
        assert_int_equal(mem_read_u16(p), 65534);

        mem_write_u16(p, (u16)((i16)(-32765)));
        assert_int_equal(mem_read_i16(p), -32765);

        mem_write_u32(p, 65534000);
        assert_int_equal(mem_read_u32(p), 65534000);

        mem_write_u32(p, (u32)((i32)(-32769)));
        assert_int_equal(mem_read_i32(p), -32769);

        mem_write_u64(p, u64_MAX);
        assert_int_equal(mem_read_u64(p), u64_MAX);

        mem_write_u64(p, (u64)((i64)(i64_MIN)));
        assert_int_equal(mem_read_u64(p), i64_MIN);
    }
}

struct CMUnitTest mem_tests[] = {
    cmocka_unit_test(mem_test_read),
    cmocka_unit_test(mem_test_write),
};

const size_t mem_tests_count = array_len(mem_tests);

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

#include "leb128_cases.h"
#include "stream.h"
#include "logger.h"

#include <cmocka.h>
#include <cmocka_private.h>

#define LOGI(fmt, ...) LOG_INFO(log_channel_test, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_test, fmt, ##__VA_ARGS__)

#if defined(NDEBUG)
    #define ENABLE_LEB128_PERFTEST
#endif

#if defined(ENABLE_LEB128_PERFTEST)
    #include <stdio.h>
    #include <stdlib.h>
    #include <time.h>

#define PERF_TEST(name, stmt)                                                                                                                 \
    {                                                                                                                                         \
        struct timespec start, current;                                                                                                       \
        timespec_get(&start, TIME_UTC);                                                                                                       \
        u64 total_iterations = 1;                                                                                                             \
        while (true) {                                                                                                                        \
            while (total_iterations++ & 0xffff) {                                                                                             \
                stmt;                                                                                                                         \
            }                                                                                                                                 \
            timespec_get(&current, TIME_UTC);                                                                                                 \
            double duration_s = (((double)current.tv_sec * 1e9 + current.tv_nsec) - ((double)start.tv_sec * 1e9 + start.tv_nsec)) / 1e9;      \
            if (duration_s > 1.) {                                                                                                            \
                LOGW("%s rate: %f M times/s", name, (double)(total_iterations * leb128_vi32_cases_count) / duration_s / 1000 / 1000); \
                break;                                                                                                                        \
            }                                                                                                                                 \
        }                                                                                                                                     \
    }

// we only test the bottleneck scenario, the vi32 and vu32. Since llvm like to generate
// full-length LEB32, we only test the 5-byte cases, which starts from index 513
static void stream_test_checked_leb_performance(void ** state) {
    PERF_TEST("stream_read_vi32", {
        for (u32 i = 513; i < leb128_vi32_cases_count; i++) {
            stream st = stream_from(leb128_vi32_cases[i].encoded);
            volatile r_i32 u = stream_read_vi32(&st);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vu32", {
        for (u32 i = 513; i < leb128_vu32_cases_count; i++) {
            stream st = stream_from(leb128_vu32_cases[i].encoded);
            volatile r_u32 u = stream_read_vu32(&st);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vi64", {
        for (u32 i = 0; i < leb128_vi64_cases_count; i++) {
            stream st = stream_from(leb128_vi64_cases[i].encoded);
            volatile r_i64 u = stream_read_vi64(&st);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vu64", {
        for (u32 i = 0; i < leb128_vu64_cases_count; i++) {
            stream st = stream_from(leb128_vu64_cases[i].encoded);
            volatile r_u64 u = stream_read_vu64(&st);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vi32_unchecked", {
        for (u32 i = 513; i < leb128_vi32_cases_count; i++) {
            const u8 * p = leb128_vi32_cases[i].encoded.ptr;
            volatile i32 u;
            stream_read_vi32_unchecked(u, p);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vu32_unchecked", {
        for (u32 i = 513; i < leb128_vu32_cases_count; i++) {
            const u8 * p = leb128_vu32_cases[i].encoded.ptr;
            volatile u32 u;
            stream_read_vu32_unchecked(u, p);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vi64_unchecked", {
        for (u32 i = 0; i < leb128_vi64_cases_count; i++) {
            const u8 * p = leb128_vi64_cases[i].encoded.ptr;
            volatile i64 u;
            stream_read_vi64_unchecked(u, p);
            UNUSED(u);
        }
    });
    PERF_TEST("stream_read_vu64_unchecked", {
        for (u32 i = 0; i < leb128_vu64_cases_count; i++) {
            const u8 * p = leb128_vu64_cases[i].encoded.ptr;
            volatile u64 u;
            stream_read_vu64_unchecked(u, p);
            UNUSED(u);
        }
    });
}
#endif

static void stream_test_checked_leb_decoding(void ** state) {
    {
        for (u32 i = 0; i < leb128_vi32_cases_count; i++) {
            stream st = stream_from(leb128_vi32_cases[i].encoded);
            r_i32 u = stream_read_vi32(&st);
            assert_true(is_ok(u));
            assert_int_equal(u.value, leb128_vi32_cases[i].expect);
            assert_int_equal(stream_remaining(&st), 0);
        }
    }
    {
        for (u32 i = 0; i < leb128_vi64_cases_count; i++) {
            stream st = stream_from(leb128_vi64_cases[i].encoded);
            r_i64 u = stream_read_vi64(&st);
            assert_true(is_ok(u));
            assert_int_equal(u.value, leb128_vi64_cases[i].expect);
            assert_int_equal(stream_remaining(&st), 0);
        }
    }
    {
        for (u32 i = 0; i < leb128_vu32_cases_count; i++) {
            stream st = stream_from(leb128_vu32_cases[i].encoded);
            r_u32 u = stream_read_vu32(&st);
            assert_true(is_ok(u));
            assert_int_equal(u.value, leb128_vu32_cases[i].expect);
            assert_int_equal(stream_remaining(&st), 0);
        }
    }
    {
        for (u32 i = 0; i < leb128_vu64_cases_count; i++) {
            stream st = stream_from(leb128_vu64_cases[i].encoded);
            r_u64 u = stream_read_vu64(&st);
            assert_true(is_ok(u));
            assert_int_equal(u.value, leb128_vu64_cases[i].expect);
            assert_int_equal(stream_remaining(&st), 0);
        }
    }

    {
        for (u32 i = 0; i < leb128_vi32_cases_count; i++) {
            const u8 * p = leb128_vi32_cases[i].encoded.ptr;
            i32 value;
            stream_read_vi32_unchecked(value, p);
            assert_int_equal(value, leb128_vi32_cases[i].expect);
        }
    }
    {
        for (u32 i = 0; i < leb128_vi64_cases_count; i++) {
            const u8 * p = leb128_vi64_cases[i].encoded.ptr;
            i64 u;
            stream_read_vi64_unchecked(u, p);
            assert_int_equal(u, leb128_vi64_cases[i].expect);
        }
    }
    {
        for (u32 i = 0; i < leb128_vu32_cases_count; i++) {
            const u8 * p = leb128_vu32_cases[i].encoded.ptr;
            u32 value;
            stream_read_vu32_unchecked(value, p);
            assert_int_equal(value, leb128_vu32_cases[i].expect);
        }
    }
    {
        for (u32 i = 0; i < leb128_vu64_cases_count; i++) {
            const u8 * p = leb128_vu64_cases[i].encoded.ptr;
            u64 u;
            stream_read_vu64_unchecked(u, p);
            assert_int_equal(u, leb128_vu64_cases[i].expect);
        }
    }
}

struct CMUnitTest stream_tests[] = {
    cmocka_unit_test(stream_test_checked_leb_decoding),
#if defined(ENABLE_LEB128_PERFTEST)
    cmocka_unit_test(stream_test_checked_leb_performance),
#endif
};

const size_t stream_tests_count = array_len(stream_tests);

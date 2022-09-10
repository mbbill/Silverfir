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

#if defined(TEST_COVERAGE)
typedef struct test_here_marker {
    int flag;
    const char * file;
    const char * function;
    int line;
} test_here_marker;
    #if defined(_MSC_VER)
        #pragma section("test$a")
        #pragma section("test$b")
        #pragma section("test$c")
        // Make sure there's no /ZI in the compiler flags because that will disable __LINE__ macro
        #define TEST_HERE()                                                       \
            do {                                                                  \
                __declspec(allocate("test$b")) static test_here_marker marker = { \
                    .flag = 0xbaba5a50,                                           \
                    .file = __FILE__,                                             \
                    .function = __FUNCTION__,                                     \
                    .line = __LINE__,                                             \
                };                                                                \
                marker.flag = 0xbaba5a51;                                         \
            } while (0)
    #elif defined(__GNUC__)
        #define TEST_HERE()                                                                                    \
            do {                                                                                               \
                __attribute__((section("test$a"))) __attribute__((unused)) static int dummy_marker1 = 0;       \
                __attribute__((section("test$b"))) __attribute__((unused)) static _test_here_marker marker = { \
                    .flag = (0xbaba5a50),                                                                      \
                    .file = __FILE__,                                                                          \
                    .function = __FUNCTION__,                                                                  \
                    .line = __LINE__,                                                                          \
                };                                                                                             \
                __attribute__((section("test$c"))) __attribute__((unused)) static int dummy_marker2 = 0;       \
                marker.flag = 0xbaba5a51;                                                                      \
            } while (0)
    #else // _MSC_VER
        #define TEST_HERE()
    #endif
    #define TEST_WHEN(condition) \
        if (condition) {         \
            TEST_HERE();         \
        }
#else // WG_TEST_COVERAGE
    #define TEST_HERE()
    #define TEST_WHEN(x)
#endif // WG_TEST_COVERAGE
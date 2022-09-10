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

#include "alloc.h"

#include <assert.h>
#include <cmocka.h>
#include <cmocka_private.h>
#include <stdlib.h>
#include <string.h>

#define FOR_EACH_UNIT_TEST_GROUP(macro) \
    macro(list)                         \
    macro(vec)                          \
    macro(parser)                       \
    macro(result)                       \
    macro(sjson)                        \
    macro(smath)                        \
    macro(stream)                       \
    macro(option)                       \
    macro(mem)
// disabled atm.
//    macro(runtime)


#define EXTERN_TEST_GROUP_DECL(name)               \
    extern const struct CMUnitTest name##_tests[]; \
    extern const size_t name##_tests_count;
FOR_EACH_UNIT_TEST_GROUP(EXTERN_TEST_GROUP_DECL)

int main() {
#if defined(_MSC_VER)
    int flag = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF | _CRTDBG_CHECK_ALWAYS_DF; // | _CRTDBG_DELAY_FREE_MEM_DF; this is a bit too slow.
    flag = (flag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
    _CrtSetDbgFlag(flag);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    // _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    // _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

#define GROUP_TESTS_COUNT(name) +name##_tests_count

    size_t total_tests_count = 0 FOR_EACH_UNIT_TEST_GROUP(GROUP_TESTS_COUNT);

    struct CMUnitTest * tests = array_alloc(struct CMUnitTest, total_tests_count);
    if (!tests) {
        cmprintf(PRINTF_TEST_ERROR, 0, "test", "Failed to allocate memory for unit tests");
        return -1;
    }

    size_t i = 0;
#define COPY_TEST_GROUP(name)                                                        \
    memcpy(tests + i, name##_tests, name##_tests_count * sizeof(struct CMUnitTest)); \
    i += name##_tests_count;
    FOR_EACH_UNIT_TEST_GROUP(COPY_TEST_GROUP);
    assert(i == total_tests_count);

    // TODO: shuffle and repeat.
    int result = _cmocka_run_group_tests("All tests", tests, total_tests_count, NULL, NULL);

    array_free(tests);
    return result;
}

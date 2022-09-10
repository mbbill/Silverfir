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

#include "coverage.h"

#include <stdio.h>

#if defined(TEST_COVERAGE)

#if defined(_MSC_VER)
__declspec(allocate("test$a")) int MarkerStart = 0;
__declspec(allocate("test$c")) int MarkerEnd = 0;
#elif defined(__GNUC__)
__attribute__((section("test$a"))) int MarkerStart = 0;
__attribute__((section("test$b"))) int dummy_marker_0 = 0;
__attribute__((section("test$c"))) int MarkerEnd = 0;
#else
#endif

void test_coverage_summary() {
    if (&MarkerEnd <= &MarkerStart) {
        printf("Incorrect section marker section layout\n");
        return;
    }
    printf("\n[==========] Test coverage statistics:\n");
    int total_slots = 0;
    int hit_slots = 0;
    for (int * p = &MarkerStart; p < (&MarkerEnd - 1); p++) {
        test_here_marker * marker = (test_here_marker *)p;
        if (((marker->flag & 0xbaba5a50) == 0xbaba5a50)) {
            total_slots++;
            if (marker->flag & 1) {
                hit_slots++;
            } else {
                printf("[  MISSED  ] %s:%s():%d \n", marker->file, marker->function, marker->line);
            }
        }
    }
    if (total_slots == 0) {
        printf("Test coverage marker not found\n");
    } else {
        printf("[==========] Total %d, hit %d, coverage %d%%\n", total_slots, hit_slots, hit_slots * 100 / total_slots);
    }
}

#else
void test_coverage_summary() {}
#endif // TEST_COVERAGE


// int main() {
//     test_coverage_summary();
//     return 0;
// }

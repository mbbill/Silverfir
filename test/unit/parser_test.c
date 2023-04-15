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

#include "hello_wasm.h"
#include "module.h"
#include "parser.h"
#include "stream.h"
#include "types.h"
#include "alloc.h"

#include <cmocka.h>
#include <cmocka_private.h>
#include <stdio.h>

static void parser_test_magic_mismatch(void ** state) {
    vstr bin = vs_pl("\0as1234", 7);
    module m = {0};
    r ret = module_init(&m, bin, vs("test"));
    assert_true(!is_ok(ret));

    module_drop(&m);
}

static void parser_test_full_module(void ** state) {
    vstr bin = vs_pl(hello_wasm, hello_wasm_size);
    module m = {0};
    r ret = module_init(&m, bin, vs("test"));
    assert_true(is_ok(ret));

    module_drop(&m);
}

// static void parser_test_load_file(void ** state) {
//     FILE * fp = fopen("rustwasm.wasm", "rb");
//     assert_non_null(fp);
//     fseek(fp, 0, SEEK_END);
//     size_t size = ftell(fp);
//     rewind(fp);
//     u8 * wasm_bin = array_alloc(u8, size);
//     assert_non_null(wasm_bin);
//     size_t read_size = fread(wasm_bin, sizeof(u8), size, fp);
//     assert_int_equal(read_size, size);
//     fclose(fp);

//     r_vstr bin = vstr_dup(s_pl(wasm_bin, read_size));
//     assert_true(is_ok(bin));
//     array_free(wasm_bin);
//     module m = {0};
//     r ret = module_init(&m, bin.value, vs("test"));
//     assert_true(is_ok(ret));

//     module_drop(&m);
// }

struct CMUnitTest parser_tests[] = {
    // cmocka_unit_test(parser_test_load_file),
    cmocka_unit_test(parser_test_magic_mismatch),
    cmocka_unit_test(parser_test_full_module),
};

const size_t parser_tests_count = array_len(parser_tests);

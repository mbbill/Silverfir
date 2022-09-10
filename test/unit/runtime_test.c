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
#include "interpreter.h"
#include "runtime.h"

#include <cmocka.h>
#include <cmocka_private.h>
#include <stdio.h>

static void runtime_test1(void ** state) {
    runtime rt = {0};
    r ret;

    vstr bin = vs_pl(hello_wasm, hello_wasm_size);
    ret = runtime_module_add(&rt, bin, vs("hello"));
    assert_true(is_ok(ret));

    module * m = runtime_module_find(&rt, s("hello"));
    assert_non_null(m);

    r_vm_ptr pvm = runtime_vm_new(&rt);
    assert_true(is_ok(pvm));

    vm * vm1 = pvm.value;
    ret = vm_instantiate_module(vm1, m);
    assert_true(is_ok(ret));

    func_addr f_addr = vm_find_func(vm1, s("hello"), s("_start"));
    assert_non_null(f_addr);

    // ret = interp_call_in_thread(t, f_addr, ARG_NULL());
    // assert_true(is_ok(ret));

    ret = runtime_drop(&rt);
    assert_true(is_ok(ret));
}

static void runtime_test_load_file(void ** state) {
    r ret;
    FILE * fp = fopen("D:\\Dev\\testwasm\\hello\\hello.wasm", "rb");
    assert_non_null(fp);
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    rewind(fp);
    u8 * wasm_bin = array_alloc(u8, size);
    assert_non_null(wasm_bin);
    size_t read_size = fread(wasm_bin, sizeof(u8), size, fp);
    assert_int_equal(read_size, size);
    fclose(fp);

    runtime rt = {0};
    r_vstr bin = vstr_dup(s_pl(wasm_bin, read_size));
    array_free(wasm_bin);
    assert_true(is_ok(bin));
    ret = runtime_module_add(&rt, bin.value, vs("hello"));
    assert_true(is_ok(ret));

    module * m = runtime_module_find(&rt, s("hello"));
    assert_non_null(m);

    r_vm_ptr pvm = runtime_vm_new(&rt);
    assert_true(is_ok(pvm));

    vm * vm1 = pvm.value;
    ret = vm_instantiate_module(vm1, m);
    assert_true(is_ok(ret));

    func_addr f_addr = vm_find_func(vm1, s("hello"), s("_start"));
    assert_non_null(f_addr);

    thread * t = vm_get_thread(vm1);

    ret = interp_call_in_thread(t, f_addr, ARG_NULL());
    assert_true(is_ok(ret));

    ret = runtime_drop(&rt);
    assert_true(is_ok(ret));
}

////////////////////////////////////////////////

struct CMUnitTest runtime_tests[] = {
    cmocka_unit_test(runtime_test1),
    cmocka_unit_test(runtime_test_load_file),
};

const size_t runtime_tests_count = array_len(runtime_tests);

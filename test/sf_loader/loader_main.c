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
#include "host_modules.h"
#include "interpreter.h"
#include "logger.h"
#include "result.h"
#include "runtime.h"

#ifdef SILVERFIR_ENABLE_WASI
#include "wasi_impl_uv.h"
#endif

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

#define LOGI(fmt, ...) LOG_INFO(log_channel_test, fmt, __VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_test, fmt, __VA_ARGS__)

r run_module(u8 * wasm_mem, size_t size) {
    assert(wasm_mem);
    check_prep(r);

    runtime rt = {0};
    module * m = NULL;

    log_channel_set_enabled(log_info, log_channel_ir_builder, true);

    unwrap(vm_ptr, vm, runtime_vm_new(&rt));

    unwrap(module, spectest_mod, host_mod_new(s("spectest")));
    check(runtime_module_add_mod(&rt, spectest_mod));
    m = runtime_module_find(&rt, s("spectest"));
    assert(m);
    check(vm_instantiate_module(vm, m));

#ifdef SILVERFIR_ENABLE_WASI
    unwrap(module, wasi_mod, host_mod_new(s("wasi_snapshot_preview1")));
    check(runtime_module_add_mod(&rt, wasi_mod));
    m = runtime_module_find(&rt, s("wasi_snapshot_preview1"));
    assert(m);
    check(vm_instantiate_module(vm, m));
#endif

    unwrap(vstr, bin, vstr_dup(s_pl(wasm_mem, size)));
    check(runtime_module_add(&rt, bin, vs("test")));
    m = runtime_module_find(&rt, s("test"));
    if (!m) {
        return err(e_general, "runtime_module_find failed");
    }
    check(vm_instantiate_module(vm, m), runtime_drop(&rt));

    thread * t = vm_get_thread(vm);
    assert(t);
    func_addr f_addr = vm_find_func(vm, s("test"), s_p("_start"));
    if (f_addr == NULL) {
        return err(e_general, "_start function not found");
    }
    check(interp_call_in_thread(t, f_addr, ARG_NULL()), runtime_drop(&rt));
    runtime_drop(&rt);
    return ok_r;
}

int simple_runner(const char * wasm_file_name) {
    assert(wasm_file_name);
    FILE * fp = fopen(wasm_file_name, "rb");
    if (!fp) {
        LOGW("fopen failed");
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (!size) {
        LOGW("File is empty");
        return 1;
    }
    rewind(fp);
    u8 * wasm_mem = array_alloc(u8, size);
    if (!wasm_mem) {
        fclose(fp);
        LOGW("OOM");
        return 1;
    }
    size_t read_size = fread(wasm_mem, sizeof(u8), size, fp);
    if (read_size != size) {
        fclose(fp);
        free(wasm_mem);
        LOGW("Read failed");
        return 1;
    }

    r result = run_module(wasm_mem, read_size);
    if (!is_ok(result)) {
        LOGW("Err: %s", result.msg);
    }
    fclose(fp);
    free(wasm_mem);
    return (!is_ok(result));
}

int main(int argc, char * argv[]) {
#if defined(_MSC_VER) && !defined(NDEBUG)
    int flag = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF; //_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF; this is a bit too slow.
    flag = (flag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
    _CrtSetDbgFlag(flag);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    // _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    // _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif

    if (argc <= 1) {
        LOGW("Usage: sf_loader <wasm file>");
#ifdef SILVERFIR_ENABLE_WASI
        LOGW("All remaining arguments will be passed to wasi");
#endif
        return 1;
    }

#ifdef SILVERFIR_ENABLE_WASI
    wasi_set_args(argc - 1, argv + 1);
#endif

    return simple_runner(argv[1]);
}

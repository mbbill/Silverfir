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

#include "host_modules.h"

#include "module.h"
#include "silverfir.h"
#include "vm.h"
#include "vstr.h"

#ifdef SILVERFIR_ENABLE_SPECTEST
    #include "spectest_gen.h"
#endif

#ifdef SILVERFIR_ENABLE_WASI
    #include "wasi_gen.h"
    #include "wasi_impl_uv.h"
#endif

const host_func_info * find_host_func_info(str name, const host_func_info * info, size_t info_len) {
    for (size_t i = 0; i < info_len; i++) {
        const host_func_info * in = &info[i];
        if (str_eq(name, s_p(in->name))) {
            return in;
        }
    }
    return NULL;
}

r inject_host_functions(module * mod, const host_func_info * info, size_t info_len) {
    check_prep(r);

    VEC_FOR_EACH(&mod->exports, export, exp) {
        if (exp->external_kind == EXTERNAL_KIND_Function) {
            func * f = vec_at_func(&mod->funcs, exp->external_idx);
            if (is_imported(f->linkage)) {
                return err(e_general, "Host functions should not be marked as imported");
            }
            const host_func_info * in = find_host_func_info(exp->name, info, info_len);
            if (!in) {
                return err(e_general, "Missing hosted function implementation");
            }
            f->tr = in->tr;
            f->host_func = in->host_func;
        }
    }
    return ok_r;
}

r_module host_mod_new(str name) {
    check_prep(r_module);

#ifdef SILVERFIR_ENABLE_SPECTEST
    if (str_eq(name, s("spectest"))) {
        module mod = {0};
        vstr mod_bin = vs_pl(spectest_wasm, spectest_wasm_len);
        check(module_init(&mod, mod_bin, vs("spectest")), {
            module_drop(&mod);
        });
        check(inject_host_functions(&mod, spectest_host_info, spectest_host_info_len), {
            module_drop(&mod);
        });
        return ok(mod);
    }
#endif
#ifdef SILVERFIR_ENABLE_WASI
    if (str_eq(name, s("wasi_snapshot_preview1"))) {
        module mod = {0};
        wasi_ctx * wctx = wasi_ctx_get();
        if (!wctx) {
            return err(e_general, "Failed to create wasi context");
        }
        mod.resource_payload = wctx;
        mod.resource_drop_cb = wasi_ctx_drop;
        vstr mod_bin = vs_pl(wasi_wasm, wasi_wasm_len);
        check(module_init(&mod, mod_bin, vs("wasi_snapshot_preview1")), {
            module_drop(&mod);
        });
        check(inject_host_functions(&mod, wasi_host_info, wasi_host_info_len), {
            module_drop(&mod);
        });
        check(module_register_name(&mod, vs("wasi_unstable")), {
            module_drop(&mod);
        });
        return ok(mod);
    }
#endif
    return err(e_general, "Module not found");
}

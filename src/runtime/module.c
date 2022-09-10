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

#include "module.h"

#include "list_impl.h"
#include "parser.h"
#include "silverfir.h"
#include "str.h"
#include "vec_impl.h"

VEC_IMPL_FOR_TYPE(type_id)
VEC_IMPL_FOR_TYPE(jump_table)

LIST_IMPL_FOR_TYPE(module)

#define FOR_EACH_MODULE_MEMBER_VECTOR(macro) \
    macro(func_type, func_types)             \
    macro(func, funcs)                       \
    macro(table, tables)                     \
    macro(memory, memories)                  \
    macro(global, globals)                   \
    macro(export, exports)                   \
    macro(element, elements)                 \
    macro(data, data)

#define MODULE_MEMBER_VEC_TYPE_DECL(type, member) \
    VEC_IMPL_FOR_TYPE(type)

FOR_EACH_MODULE_MEMBER_VECTOR(MODULE_MEMBER_VEC_TYPE_DECL)

r module_init(module * mod, vstr bin, vstr name) {
    assert(mod);
    check_prep(r);
    mod->wasm_bin = bin;
    check(vec_push_vstr(&mod->names, name));
    check(vec_shrink_to_fit_vstr(&mod->names));

    check(parse_module(mod));
    return ok_r;
}

r module_drop(module * mod) {
    assert(mod);
    check_prep(r);

    if (mod->ref_count) {
        return err(e_general, "Can't drop the module because it's in use (ref_count > 0)");
    }

    // clean up the inner vectors first.
    VEC_FOR_EACH(&mod->funcs, func, iter) {
        vec_clear_type_id(&iter->local_types);
        vec_clear_jump_table(&iter->jt);
    }
    VEC_FOR_EACH(&mod->elements, element, iter) {
        vec_clear_u32(&iter->v_funcidx);
        vec_clear_str(&iter->v_expr);
    }

    // clean up the direct members.
#define MODULE_MEMBER_VEC_CLEAR(type, member) \
    vec_clear_##type(&mod->member);
    FOR_EACH_MODULE_MEMBER_VECTOR(MODULE_MEMBER_VEC_CLEAR);

    // clear the name and wasm binary storage if they're owned.
    VEC_FOR_EACH(&mod->names, vstr, name) {
        vstr_drop(name);
    }
    vec_clear_vstr(&mod->names);
    vstr_drop(&mod->wasm_bin);
    if (mod->resource_payload) {
        assert(mod->resource_drop_cb);
        mod->resource_drop_cb(mod->resource_payload);
    }

    return ok_r;
}

bool func_type_eq(func_type a, func_type b) {
    if ((a.param_count == b.param_count) && (a.result_count == b.result_count)) {
        if (str_eq(a.params, b.params) && str_eq(a.results, b.results)) {
            return true;
        }
    }
    return false;
}

r module_register_name(module * mod, vstr name) {
    assert(mod);
    check_prep(r);
    assert(vec_size_vstr(&mod->names));
    check(vec_push_vstr(&mod->names, name));
    return ok_r;
}

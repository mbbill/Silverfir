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

#include "parser.h"

#include "opcode.h"
#include "silverfir.h"
#include "types.h"
#include "utf8.h"
#include "wasm_format.h"

#include <assert.h>

#define PARSER_FUNC_DECL(name, id) r parse_section_##name(module * mod, stream st);
FOR_EACH_WASM_SECTION(PARSER_FUNC_DECL)

r_vec_type_id parse_types(u32 count, str types) {
    check_prep(r_vec_type_id);

    vec_type_id v = {0};
    if (!count) {
        return ok(v);
    }
    vec_reserve_type_id(&v, count);
    stream st = stream_from(types);
    for (u32 i = 0; i < count; i++) {
        unwrap(i8, t, stream_read_vi7(&st), {
            vec_clear_type_id(&v);
        });
        if (!is_value_type(t)) {
            vec_clear_type_id(&v);
            return err(e_invalid, "Invalid value type");
        }
        check(vec_push_type_id(&v, t), {
            vec_clear_type_id(&v);
        });
    }
    return ok(v);
}

static r_limits parse_limits(stream * st, u32 max_cap) {
    check_prep(r_limits);
    assert(max_cap);

    unwrap(u8, flag, stream_read_u8(st));
    if ((flag != 0x0) && (flag != 0x1)) {
        return err(e_malformed, "Invalid limits");
    }
    unwrap(u32, min, stream_read_vu32(st));
    u32 max = max_cap;
    if (flag == 0x1) {
        unwrap(u32, m, stream_read_vu32(st));
        if (m <= max_cap) {
            max = m;
        } else {
            return err(e_invalid, "Invalid limits max");
        }
    }
    if (min > max) {
        return err(e_invalid, "Invalid limits min");
    }
    limits lim = {
        .min = min,
        .max = max,
    };
    return ok(lim);
}

static r_str parse_const_expr(stream * st) {
    check_prep(r_str);

    stream st_copy = *st;
    while (true) {
        unwrap(u8, opcode, stream_read_u8(&st_copy));
        switch (opcode) {
            case op_i32_const: {
                unwrap_drop(i32, stream_read_vi32(&st_copy));
                continue;
            }
            case op_i64_const: {
                unwrap_drop(i64, stream_read_vi64(&st_copy));
                continue;
            }
            case op_f32_const: {
                unwrap_drop(f32, stream_read_f32(&st_copy));
                continue;
            }
            case op_f64_const: {
                unwrap_drop(f64, stream_read_f64(&st_copy));
                continue;
            }
            case op_ref_null: {
                unwrap_drop(i8, stream_read_vi7(&st_copy));
                continue;
            }
            case op_ref_func: {
                unwrap_drop(u32, stream_read_vu32(&st_copy));
                continue;
            }
            case op_global_get: {
                unwrap_drop(u32, stream_read_vu32(&st_copy));
                continue;
            }
            case op_end: {
                goto end;
            }
            default: {
                // the interp_reduce_const_expr will check if it's malformed
                // or invalid later. So we just keep moving until we see the op_end
            }
        }
    }

end:;
    // TODO: probably need an API to do this.
    str s = {
        .ptr = st->p,
        .len = st_copy.p - st->p,
    };
    *st = st_copy;
    return ok(s);
}

// TODO: handle UTF-8
static r_str parse_name(stream * st) {
    assert(st);
    check_prep(r_str);

    unwrap(u32, name_len, stream_read_vu32(st));
    if (!name_len) {
        return ok(STR_NULL);
    }
    unwrap(stream, name, stream_slice(st, name_len));
    if (!is_valid_utf8(name.s.ptr, name.s.len)) {
        return err(e_malformed, "Invalid utf8 name");
    }
    return ok(name.s);
}

r parse_section_custom(module * mod, stream st) {
    return ok_r;
}

r parse_section_type(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_reserve_func_type(&mod->func_types, count));
    vec_set_fixed(&mod->func_types, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(u8, form, stream_read_vu7(&st));

        if (form != 0x60) {
            return err(e_invalid, "Unsupported function form (should be 0x60)");
        }

        func_type ft = {0};

        unwrap(u32, param_count, stream_read_vu32(&st));

        if (param_count > RESULT_TYPE_COUNT_MAX) {
            return err(e_invalid, "Function argument count exceeds limit");
        }
        ft.param_count = param_count;
        if (ft.param_count) {
            ft.params = str_from(st.p, ft.param_count);
        }

        for (u32 a = 0; a < param_count; a++) {
            unwrap(i8, param_type, stream_read_vi7(&st));
            if (!is_value_type(param_type)) {
                return err(e_invalid, "Invalid value type");
            }
        }

        unwrap(u32, return_count, stream_read_vu32(&st));
        if (return_count > RESULT_TYPE_COUNT_MAX) {
            return err(e_invalid, "Result count exceeds limit");
        }
        ft.result_count = return_count;
        if (ft.result_count) {
            ft.results = str_from(st.p, ft.result_count);
        }

        for (u32 a = 0; a < return_count; a++) {
            unwrap(i8, return_type, stream_read_vi7(&st));
            if (!is_value_type(return_type)) {
                return err(e_invalid, "Invalid value type");
            }
        }
        check(vec_push_func_type(&mod->func_types, ft));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed function type section");
    }
    return ok_r;
}

r parse_section_import(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));

    for (u32 i = 0; i < count; i++) {
        // empty names are actually allowed.
        unwrap(str, module_name, parse_name(&st));
        unwrap(str, field_name, parse_name(&st));

        unwrap(u8, external_kind, stream_read_u8(&st));
        switch (external_kind) {
            case EXTERNAL_KIND_Function: {
                unwrap(u32, typeidx, stream_read_vu32(&st));
                if (typeidx >= vec_size_func_type(&mod->func_types)) {
                    return err(e_invalid, "Invalid function type index");
                }
                func_type fn_type = *vec_at_func_type(&mod->func_types, typeidx);
                func func = {
                    .fn_type = fn_type,
                    .linkage = linkage_imported,
                    .path = (import_path){.module = module_name, .field = field_name},
                };
                check(vec_push_func(&mod->funcs, func));
                mod->imported_func_count++;
                break;
            }
            case EXTERNAL_KIND_Table: {
                unwrap(i8, valtype, stream_read_vi7(&st));
                if (!is_ref(valtype)) {
                    return err(e_invalid, "Invalid ref in the imported table");
                }
                unwrap(limits, lim, parse_limits(&st, u32_MAX));
                table table = {
                    .valtype = valtype,
                    .lim = lim,
                    .linkage = linkage_imported,
                    .path = (import_path){.module = module_name, .field = field_name},
                };
                check(vec_push_table(&mod->tables, table));
                mod->imported_table_count++;
                break;
            }
            case EXTERNAL_KIND_Memory: {
                unwrap(limits, lim, parse_limits(&st, WASM_MEM_MAX_PAGES));
                memory mem = {
                    .lim = lim,
                    .linkage = linkage_imported,
                    .path = (import_path){.module = module_name, .field = field_name},
                };
                check(vec_push_memory(&mod->memories, mem));
                mod->imported_mem_count++;
                break;
            }
            case EXTERNAL_KIND_Global: {
                unwrap(i8, valtype, stream_read_vi7(&st));
                if (!is_value_type(valtype)) {
                    return err(e_invalid, "Invalid value type in the imported global");
                }
                unwrap(u8, mut, stream_read_u8(&st));
                if ((mut != 0) && (mut != 1)) {
                    return err(e_malformed, "Invalid mut value");
                }
                global g = {
                    .valtype = valtype,
                    .mut = (!!mut),
                    .linkage = linkage_imported,
                    .path = (import_path){.module = module_name, .field = field_name},
                };
                check(vec_push_global(&mod->globals, g));
                mod->imported_global_count++;
                break;
            }
            default: {
                return err(e_malformed, "Malformed import kind");
            }
        }
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed import section");
    }
    return ok_r;
}

r parse_section_function(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));

    check(vec_reserve_func(&mod->funcs, count + vec_size_func(&mod->funcs)));
    vec_set_fixed(&mod->funcs, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(u32, typeidx, stream_read_vu32(&st));
        if (typeidx >= vec_size_func_type(&mod->func_types)) {
            return err(e_invalid, "Invalid function type index");
        }
        func_type fn_type = *vec_at_func_type(&mod->func_types, typeidx);
        func func = {
            .fn_type = fn_type,
            .linkage = linkage_none,
            .local_count = fn_type.param_count,
        };
        check(vec_push_func(&mod->funcs, func));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed function section");
    }
    return ok_r;
}

r parse_section_table(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_reserve_table(&mod->tables, count + vec_size_table(&mod->tables)));
    vec_set_fixed(&mod->tables, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(i8, valtype, stream_read_vi7(&st));
        if (!is_ref(valtype)) {
            return err(e_invalid, "Invalid ref in the table section");
        }
        unwrap(limits, lim, parse_limits(&st, u32_MAX));
        table tab = {
            .valtype = valtype,
            .lim = lim,
            .linkage = linkage_none,
        };
        check(vec_push_table(&mod->tables, tab));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed table section");
    }
    return ok_r;
}

r parse_section_memory(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_reserve_memory(&mod->memories, count + vec_size_memory(&mod->memories)));
    vec_set_fixed(&mod->memories, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(limits, lim, parse_limits(&st, WASM_MEM_MAX_PAGES));
        memory mem = {
            .lim = lim,
            .linkage = linkage_none,
        };
        check(vec_push_memory(&mod->memories, mem));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed memory section");
    }
    return ok_r;
}

r parse_section_global(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_reserve_global(&mod->globals, count + vec_size_global(&mod->globals)));
    vec_set_fixed(&mod->globals, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(i8, valtype, stream_read_vi7(&st));
        if (!is_value_type(valtype)) {
            return err(e_invalid, "Invalid value type in the global section");
        }
        unwrap(u8, mut, stream_read_u8(&st));
        if ((mut != 0) && (mut != 1)) {
            return err(e_malformed, "Invalid encoding of the 'mut' flag of global values");
        }
        global g = {
            .valtype = valtype,
            .mut = (!!mut),
            .linkage = linkage_none,
        };
        unwrap(str, expr, parse_const_expr(&st));
        g.expr = expr;
        check(vec_push_global(&mod->globals, g));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed global section");
    }
    return ok_r;
}

r parse_section_export(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_reserve_export(&mod->exports, count));
    vec_set_fixed(&mod->exports, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(str, name, parse_name(&st));
        unwrap(u8, external_kind, stream_read_u8(&st));
        if (!is_valid_external_kind(external_kind)) {
            return err(e_invalid, "Invalid external kind");
        }
        // atm all idx are u32
        unwrap(u32, idx, stream_read_vu32(&st));
        switch (external_kind) {
            case EXTERNAL_KIND_Function: {
                if (idx >= vec_size_func(&mod->funcs)) {
                    return err(e_invalid, "Invalid function index");
                }
                vec_at_func(&mod->funcs, idx)->linkage |= linkage_exported;
                break;
            }
            case EXTERNAL_KIND_Table: {
                if (idx >= vec_size_table(&mod->tables)) {
                    return err(e_invalid, "Invalid table index");
                }
                vec_at_table(&mod->tables, idx)->linkage |= linkage_exported;
                break;
            }
            case EXTERNAL_KIND_Memory: {
                if (idx >= vec_size_memory(&mod->memories)) {
                    return err(e_invalid, "Invalid memory index");
                }
                vec_at_memory(&mod->memories, idx)->linkage |= linkage_exported;
                break;
            }
            case EXTERNAL_KIND_Global: {
                if (idx >= vec_size_global(&mod->globals)) {
                    return err(e_invalid, "Invalid global index");
                }
                vec_at_global(&mod->globals, idx)->linkage |= linkage_exported;
                break;
            }
        }
        // TODO: check if the exported names are unique.
        export exp = {
            .name = name,
            .external_kind = external_kind,
            .external_idx = idx,
        };
        check(vec_push_export(&mod->exports, exp));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed export section");
    }
    return ok_r;
}

r parse_section_start(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, start_funcidx, stream_read_vu32(&st));
    if (start_funcidx >= vec_size_func(&mod->funcs)) {
        return err(e_invalid, "Invalid function index of the start function");
    }
    func * start_fn = vec_at_func(&mod->funcs, start_funcidx);
    assert(start_fn);
    func_type start_fn_type = start_fn->fn_type;
    if (start_fn_type.param_count || start_fn_type.result_count) {
        return err(e_invalid, "The start function should have no parameter and return value");
    }
    mod->start_funcidx = some(option_u32, start_funcidx);
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed start section");
    }
    return ok_r;
}

r parse_section_element(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    check(vec_resize_element(&mod->elements, count));
    vec_set_fixed(&mod->elements, true);

    VEC_FOR_EACH(&mod->elements, element, elem) {
        unwrap(u8, kind, stream_read_u8(&st));
        // https://github.com/WebAssembly/bulk-memory-operations/blob/master/proposals/bulk-memory-operations/Overview.md
        // 0:active                expr           vec(funcidx)
        // 1:passive                    elemkind  vec(funcidx)
        // 2:active       tableidx expr elemkind  vec(funcidx)
        // 3:declarative                elemkind  vec(funcidx)
        // 4:active                expr           vec(expr)
        // 5:passive                    ref       vec(expr)
        // 6:active       tableidx expr ref       vec(expr)
        // 7:declarative                ref       vec(expr)
        //
        // b & 1 == 0 -> active
        // b & 3 == 3 -> declarative
        // b & 3 == 1 -> passive
        //
        // b & 3 == 2 -> tableidx
        // b & 1 == 0 -> expr
        // b >=1 && b <=3 elemkind
        // b >= 5 && b <= 7 ref
        // b <= 3 -> vec(funcidx), else vec(expr)
        if (kind > elem_kind_max) {
            return err(e_malformed, "Invalid element flag");
        }
        if (!(kind & elem_kind_passive)) {
            elem->mode = elem_mode_active;
        } else if ((kind & elem_kind_declarative) == elem_kind_declarative) {
            elem->mode = elem_mode_declarative;
        } else {
            elem->mode = elem_mode_passive;
        }
        // active and have table idx
        if ((kind & (elem_kind_passive | elem_kind_table_idx)) == elem_kind_table_idx) {
            unwrap(u32, tableidx, stream_read_vu32(&st));
            elem->tableidx = tableidx;
        } else {
            elem->tableidx = 0;
        }
        // active
        if (!(kind & elem_kind_passive)) {
            unwrap(str, expr, parse_const_expr(&st));
            elem->offset_expr = expr;
        }
        // elemkind
        elem->valtype = TYPE_ID_funcref; // default
        if (kind & ((elem_kind_passive | elem_kind_table_idx))) {
            if (kind & elem_kind_expr) {
                unwrap(i8, type, stream_read_vi7(&st));
                if (!is_ref(type)) {
                    return err(e_malformed, "Invalid ref");
                }
                elem->valtype = type;
            } else {
                unwrap(u8, external_kind, stream_read_u8(&st));
                if (external_kind != 0) {
                    // Additional element kinds may be added in future versions of WebAssembly.
                    return err(e_invalid, "Invalid element external kind");
                }
            }
        }
        // vec(funcidx)
        unwrap(u32, idx_count, stream_read_vu32(&st));
        elem->data_len = idx_count;
        if (kind & elem_kind_expr) {
            // vec(expr)
            check(vec_reserve_str(&elem->v_expr, idx_count));
            vec_set_fixed(&elem->v_expr, true);
            for (u32 j = 0; j < idx_count; j++) {
                unwrap(str, expr, parse_const_expr(&st));
                check(vec_push_str(&elem->v_expr, expr));
            }
        } else {
            check(vec_reserve_u32(&elem->v_funcidx, idx_count));
            vec_set_fixed(&elem->v_funcidx, true);
            for (u32 j = 0; j < idx_count; j++) {
                unwrap(u32, funcidx, stream_read_vu32(&st));
                if (funcidx >= vec_size_func(&mod->funcs)) {
                    return err(e_invalid, "Invalid function index");
                }
                vec_push_u32(&elem->v_funcidx, funcidx);
            }
        }
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed element section");
    }
    return ok_r;
}

r parse_section_code(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));
    if (count + mod->imported_func_count != vec_size_func(&mod->funcs)) {
        return err(e_malformed, "Function number mismatch between code and function sections");
    }

    for (u32 i = 0; i < count; i++) {
        // we've checked the vector size, so it's safe.
        func * fn = vec_at_func(&mod->funcs, mod->imported_func_count + i);
        // the size includes locals
        unwrap(u32, code_size, stream_read_vu32(&st));
        unwrap(stream, code, stream_slice(&st, code_size));
        // we create a slice of the main stream so that we can leave the main "st"
        // and parse the copy.
        unwrap(u32, local_group_count, stream_read_vu32(&code));

        // in order to pass the spec test we need to calculate the total local numbers to
        // make sure it doesn't exceed 2^32-1.
        stream code_copy = code;
        u64 total_locals = 0;
        for (u32 j = 0; j < local_group_count; j++) {
            unwrap(u32, local_count, stream_read_vu32(&code_copy));
            unwrap_drop(i8, stream_read_vi7(&code_copy));
            total_locals += local_count;
            // parser should allow no more than u32_MAX number of locals because that's
            // valid binary format. However, the runtime may still limit the local to a
            // much smaller number due to the resource limit. And in that case a different
            // error (assert_exhaustion) will be raised instead.
            if (total_locals > u32_MAX) {
                return err(e_malformed, "Too many locals");
            }
        }

        // now we do the job
        for (u32 j = 0; j < local_group_count; j++) {
            unwrap(u32, local_count, stream_read_vu32(&code));
            unwrap(i8, valtype, stream_read_vi7(&code));
            if (!is_value_type(valtype)) {
                return err(e_invalid, "Invalid value type");
            }
            for (u32 k = 0; k < local_count; k++) {
                check(vec_push_type_id(&fn->local_types, valtype));
                fn->local_count++;
            }
        }
        // now, the sub stream should contain the code only.
        if (!stream_remaining(&code)) {
            return err(e_invalid, "Function body is empty");
        }
        // TODO: move this to an str API
        fn->code = (str){
            .ptr = code.p,
            .len = code.s.ptr + code.s.len - code.p,
        };
        check(vec_shrink_to_fit_type_id(&fn->local_types));
        vec_set_fixed(&fn->local_types, true);
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed code section");
    }
    return ok_r;
}

r parse_section_data(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, count, stream_read_vu32(&st));

    if (is_some(mod->data_count) && (value(mod->data_count) != count)) {
        return err(e_malformed, "Data and data count section doesn't match");
    } else {
        mod->data_count = some(option_u32, count);
    }

    check(vec_reserve_data(&mod->data, count));
    vec_set_fixed(&mod->data, true);

    for (u32 i = 0; i < count; i++) {
        unwrap(u8, data_kind, stream_read_u8(&st));
        if (data_kind > 2) {
            return err(e_malformed, "Invalid data kind");
        }
        bool is_active = !(data_kind & 1);
        u32 memidx = 0;
        str offset_expr = STR_NULL;
        if (data_kind & 2) {
            unwrap(u32, idx, stream_read_vu32(&st));
            if (idx) {
                // In the current version of WebAssembly, at most one
                // memory may be defined or imported in a single module, so
                // all valid active data segments have a memory value of 0.
                return err(e_invalid, "Invalid memory index");
            }
            memidx = idx;
        }
        if (!(data_kind & 1)) {
            unwrap(str, expr, parse_const_expr(&st));
            offset_expr = expr;
        }
        unwrap(u32, byte_len, stream_read_vu32(&st));
        str bytes;
        if (byte_len) {
            unwrap(stream, b, stream_slice(&st, byte_len));
            bytes = b.s;
        } else {
            bytes = STR_NULL;
        }
        data d = {
            .is_active = is_active,
            .memidx = memidx,
            .offset_expr = offset_expr,
            .bytes = bytes,
        };
        check(vec_push_data(&mod->data, d));
    }
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed data section");
    }
    return ok_r;
}

r parse_section_data_count(module * mod, stream st) {
    check_prep(r);

    unwrap(u32, data_count, stream_read_vu32(&st));
    mod->data_count = some(option_u32, data_count);
    if (stream_remaining(&st)) {
        return err(e_malformed, "Malformed data_count section");
    }
    return ok_r;
}

r parse_module(module * mod) {
    check_prep(r);
    assert(mod);

    if (vstr_is_null(mod->wasm_bin)) {
        return err(e_malformed, "Missing wasm binary");
    }

    stream st = stream_from(mod->wasm_bin.s);

    unwrap(u32, magic, stream_read_u32(&st));
    if (magic != 0x6D736100) {
        return err(e_malformed, "Invalid magic number");
    }

    unwrap(u32, version, stream_read_u32(&st));
    if (version != 1) {
        return err(e_malformed, "Unsupported WebAssembly binary format version");
    }
    mod->format_version = version;

    u8 current_order_idx = 0; // to check the section order.
    while (true) {
        if (!stream_remaining(&st)) {
            // Every section is optional, although unlikely to happen.
            return ok_r;
        }

        unwrap(u8, id, stream_read_vu7(&st));
        unwrap(u32, payload_len, stream_read_vu32(&st));

        if (id != SECTION_custom) {
#define SECTION_ID(name, id) id,
            static const u8 order[] = {
                FOR_EACH_WASM_SECTION(SECTION_ID)};
            while (current_order_idx < array_len(order) && order[current_order_idx] != id) {
                current_order_idx++;
            }
            if (current_order_idx == array_len(order)) {
                return err(e_malformed, "Incorrect section order");
            }
        }
        if (!payload_len) {
            continue;
        }
        unwrap(stream, section_content, stream_slice(&st, payload_len));
        // clang-format off
        switch (id) {
            #define CALL_SECTION_PARSER(name, id) case id: { \
                check(parse_section_##name(mod, section_content)); break; }
            FOR_EACH_WASM_SECTION(CALL_SECTION_PARSER);
            default:
                return err(e_malformed, "Unsupported section");
        }
        // clang-format on
    }

    return ok_r;
}

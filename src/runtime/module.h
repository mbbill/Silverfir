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

#include "list.h"
#include "option.h"
#include "result.h"
#include "str.h"
#include "trampoline.h"
#include "types.h"
#include "vec.h"
#include "vstr.h"
#include "wasm_format.h"

#define linkage_none 0
#define linkage_imported (1u)
#define linkage_exported (1u << 1)

#define is_imported(x) (x & linkage_imported)
#define is_exported(x) (x & linkage_exported)

typedef struct func_type {
    u32 param_count;
    u32 result_count;
    str params;
    str results;
} func_type;
VEC_DECL_FOR_TYPE(func_type)
RESULT_TYPE_DECL(func_type)

bool func_type_eq(func_type a, func_type b);

typedef struct import_path {
    str module;
    str field;
} import_path;

#define JUMP_TABLE_IDX_INVALID u16_MAX

// the target_offset points to the instruction address *after* the target, and *before* the immediates.
// This we if we need to jump into another location, we save some time reading the immediates.
// However, the stack_offset record the stack state *after* all the push/pops are done since we can't
// skip any of them.
typedef struct jump_table {
    u32 pc; // offset from func->code
    i32 target_offset; // positive -> forward, negative -> backward (loop)
    u32 stack_offset; // always backward
    u16 arity;
    u16 next_idx;
} jump_table;
VEC_DECL_FOR_TYPE(jump_table)

typedef struct func {
    func_type fn_type;
    u32 linkage;
    u32 local_count; // including the parameters.
    vec_type_id local_types; // Doesn't include params.
    str code;
    import_path path;
    vec_jump_table jt;
    // the maximum stack usage of the function. Locals NOT included. Filled by the validator.
    // it also contains the local size of the outgoing calls so that the callee doesn't have
    // to copy the args to locals and simply reuse the entire local var space.
    u32 stack_size_max;
    // for host functions
    trampoline tr;
    void * host_func;
} func;
VEC_DECL_FOR_TYPE(func)

typedef struct limits {
    u32 min;
    u32 max;
} limits;
RESULT_TYPE_DECL(limits)

typedef struct memory {
    limits lim;
    u32 linkage;
    import_path path;
} memory;
VEC_DECL_FOR_TYPE(memory)

typedef struct table {
    type_id valtype;
    limits lim;
    u32 linkage;
    import_path path;
} table;
VEC_DECL_FOR_TYPE(table)

typedef struct global {
    type_id valtype;
    bool mut;
    str expr;
    u32 linkage;
    import_path path;
} global;
VEC_DECL_FOR_TYPE(global)

typedef struct export {
    str name;
    external_kind external_kind;
    u32 external_idx;
}
export;
VEC_DECL_FOR_TYPE(export)

typedef struct element {
    elem_mode mode;
    u32 tableidx;
    str offset_expr;
    type_id valtype;
    size_t data_len; // the length of the vectors
    vec_u32 v_funcidx;
    vec_str v_expr;
} element;
VEC_DECL_FOR_TYPE(element)

typedef struct data {
    bool is_active;
    u32 memidx; // should be always 0 atm
    str offset_expr;
    str bytes;
} data;
VEC_DECL_FOR_TYPE(data)

// called on module dtor.
typedef void (*resource_drop_callback)(void *);

typedef struct module {
    vec_vstr names;
    vstr wasm_bin;
    u32 ref_count;
    u32 format_version;
    vec_func_type func_types;
    u32 imported_func_count;
    vec_func funcs;
    u32 imported_table_count;
    vec_table tables;
    u32 imported_mem_count;
    vec_memory memories;
    u32 imported_global_count;
    vec_global globals;
    vec_export exports;
    option_u32 start_funcidx;
    vec_element elements;
    option_u32 data_count;
    vec_data data;
    void * resource_payload;
    resource_drop_callback resource_drop_cb;
} module;
LIST_DECL_FOR_TYPE(module)
RESULT_TYPE_DECL(module)

// initialize a module with a wasm binary, then parse it
r module_init(module * mod, vstr bin, vstr name);

// Give another name to the module. A module can have more than one registered names.
r module_register_name(module * mod, vstr name);

// release the internal resources
r module_drop(module * mod);


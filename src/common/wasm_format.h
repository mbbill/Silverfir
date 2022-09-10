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

#include "types.h"
#include "vec.h"

#include <stdbool.h>

#define WASM_PAGE_SIZE (65536)
#define WASM_MEM_MAX_PAGES (65536)

// sections, in order
#define FOR_EACH_WASM_SECTION(macro) \
    macro(custom, 0)                 \
    macro(type, 1)                   \
    macro(import, 2)                 \
    macro(function, 3)               \
    macro(table, 4)                  \
    macro(memory, 5)                 \
    macro(global, 6)                 \
    macro(export, 7)                 \
    macro(start, 8)                  \
    macro(element, 9)                \
    macro(data_count, 12)            \
    macro(code, 10)                  \
    macro(data, 11)

#define SECTION_ENUM(name, id) SECTION_##name = id,
typedef enum section_id {
    FOR_EACH_WASM_SECTION(SECTION_ENUM)
} section_id;

typedef uptr ref;
typedef u32 func_;
typedef u32 struct_;
typedef u32 array_;
typedef u32 void_;

VEC_DECL_FOR_TYPE(ref)

// nullref is a null value of the ref type. It's an runtime representation (pointer) of a
// function referring to null. Notice the difference between the nullref and nullfidx
// in which the later represents the compile-time value of a null function. And in wasm all
// function index are u32, so the nullfidx will simply be u32_MAX.
// The only place you can get nullref or nay reference it through ref.null or ref.func instr
#define nullref uptr_MAX
#define nullfidx u32_MAX

// wasm types
// Note that I just picked a unused slot for the unknown.
#define FOR_EACH_VALUE_TYPE(macro)           \
    macro(i32, ((i8)(-0x01))) /*0x7f*/       \
    macro(i64, ((i8)(-0x02))) /*0x7e*/       \
    macro(f32, ((i8)(-0x03))) /*0x7d*/       \
    macro(f64, ((i8)(-0x04))) /*0x7c*/       \
    macro(v128, ((i8)(-0x05))) /*0x7b*/      \
    macro(funcref, ((i8)(-0x10))) /*0x70*/   \
    macro(externref, ((i8)(-0x11))) /*0x6f*/ \
    macro(ref, ((i8)(-0x15))) /*0x6b*/       \
    macro(func_, ((i8)(-0x20))) /*0x60*/     \
    macro(struct_, ((i8)(-0x21))) /*0x5f*/   \
    macro(array_, ((i8)(-0x22))) /*0x5e*/    \
    macro(void_, ((i8)(-0x40))) /*0x40*/     \
    macro(unknown, ((i8)(-0x39))) /*0x41*/

#define TYPE_ID_ENUM(name, id) TYPE_ID_##name = id,
typedef enum type_id {
    FOR_EACH_VALUE_TYPE(TYPE_ID_ENUM)
} type_id;
VEC_DECL_FOR_TYPE(type_id)
RESULT_TYPE_DECL(type_id)
RESULT_TYPE_DECL(vec_type_id)

typedef union value_u {
    i32 u_i32;
    i64 u_i64;
    f32 u_f32;
    f64 u_f64;
    ref u_ref;
    // u32 and u64 are not wasm basic types. We use it for easier reinterpretation casts.
    u32 u_u32;
    u64 u_u64;
} value_u;
VEC_DECL_FOR_TYPE(value_u)
RESULT_TYPE_DECL(value_u)

INLINE bool is_num(type_id id) {
    switch (id) {
        case TYPE_ID_i32:
        case TYPE_ID_i64:
        case TYPE_ID_f32:
        case TYPE_ID_f64:
        case TYPE_ID_unknown:
            return true;
        default:
            return false;
    }
}

INLINE bool is_vec(type_id id) {
    return (id == TYPE_ID_v128) || (id == TYPE_ID_unknown);
}

INLINE bool is_ref(type_id id) {
    return (id == TYPE_ID_funcref) || (id == TYPE_ID_externref) || (id == TYPE_ID_ref) || (id == TYPE_ID_unknown);
}

INLINE bool is_value_type(type_id id) {
    return is_num(id) || is_vec(id) || is_ref(id);
}

// A bundle of type and value. Useful for passing arguments and return values
// while checking types at the same time.
typedef struct typed_value {
    type_id type;
    value_u val;
} typed_value;
VEC_DECL_FOR_TYPE(typed_value)
RESULT_TYPE_DECL(typed_value)
RESULT_TYPE_DECL(vec_typed_value)

// convenient for passing null args.
#define ARG_NULL()      \
    (vec_typed_value) { \
        0               \
    }

// import export types
#define FOR_EACH_EXTERNAL_KIND(macro) \
    macro(Function, 0x00)             \
    macro(Table, 0x01)                \
    macro(Memory, 0x02)               \
    macro(Global, 0x03)

#define EXTERNAL_KIND_ENUM(name, id) EXTERNAL_KIND_##name = id,
typedef enum external_kind {
    FOR_EACH_EXTERNAL_KIND(EXTERNAL_KIND_ENUM)
} external_kind;

typedef enum elem_mode {
    elem_mode_active = 1,
    elem_mode_passive = 2,
    elem_mode_declarative = 3,
} elem_mode;

#define elem_kind_passive 1
#define elem_kind_table_idx 2
#define elem_kind_declarative 3
#define elem_kind_expr 4
#define elem_kind_max 7

INLINE bool is_valid_external_kind(external_kind id) {
#define EXTERNAL_KIND_CASE(name, id) \
    case id:                         \
        return true;
    switch (id) {
        FOR_EACH_EXTERNAL_KIND(EXTERNAL_KIND_CASE)
        default:
            return false;
    }
}

// maximum values. Some of them are limitted by the data types
// see the struct function_type
#define RESULT_TYPE_COUNT_MAX UINT16_MAX

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

#include "module.h"
#include "result.h"
#include "types.h"
#include "opcode.h"
#include "stream.h"

typedef r (*cb_decode_begin)(void * payload);
typedef r (*cb_none)(void * payload, wasm_opcode opcode, stream imm);
typedef r (*cb_block)(void * payload, wasm_opcode opcode, stream imm, func_type type); // block, loop, if
typedef r (*cb_br_or_if)(void * payload, wasm_opcode opcode, stream imm, u8 lth);
typedef r (*cb_u32)(void * payload, stream imm, u32 u1);
typedef r (*cb_u32_u32)(void * payload, stream imm, u32 u1, u32 u2);
typedef r (*cb_typeid)(void * payload, wasm_opcode opcode, stream imm, type_id type);
typedef r (*cb_load_store)(void * payload, wasm_opcode opcode, stream imm, u8 align, u32 offset);
typedef r (*cb_i32)(void * payload, stream imm, i32 val);
typedef r (*cb_i64)(void * payload, stream imm, i64 val);
typedef r (*cb_f32)(void * payload, stream imm, f32 val);
typedef r (*cb_f64)(void * payload, stream imm, f64 val);
typedef r (*cb_fc_none)(void * payload, wasm_opcode_fc opcode, stream imm);
typedef r (*cb_fd_none)(void * payload, wasm_opcode_fd opcode, stream imm);
typedef r (*cb_decode_end)(void * payload);

typedef struct op_decoder_callbacks {
    cb_none on_opcode;
    cb_decode_begin on_decode_begin;
    cb_none on_unreachable;
    cb_none on_nop;
    cb_block on_block;
    cb_none on_else;
    cb_none on_end;
    cb_br_or_if on_br_or_if;
    cb_none on_br_table;
    cb_none on_return;
    cb_u32 on_call;
    cb_u32_u32 on_call_indirect;
    cb_none on_drop;
    cb_none on_select;
    cb_typeid on_select_t;
    cb_u32 on_local_get;
    cb_u32 on_local_set;
    cb_u32 on_local_tee;
    cb_u32 on_global_get;
    cb_u32 on_global_set;
    cb_u32 on_table_get;
    cb_u32 on_table_set;
    cb_load_store on_memory_load_store;
    cb_none on_memory_size;
    cb_none on_memory_grow;
    cb_i32 on_i32_const;
    cb_i64 on_i64_const;
    cb_f32 on_f32_const;
    cb_f64 on_f64_const;
    cb_none on_iunop;
    cb_none on_funop;
    cb_none on_ibinop;
    cb_none on_fbinop;
    cb_none on_itestop;
    cb_none on_irelop;
    cb_none on_frelop;
    cb_none on_wrapop;
    cb_none on_truncop;
    cb_none on_convertop;
    cb_none on_rankop; // demote & promote
    cb_none on_reinterpretop;
    cb_none on_extendop;
    cb_typeid on_ref_null;
    cb_none on_ref_is_null;
    cb_u32 on_ref_func;
    cb_fc_none on_opcode_fc;
    cb_fc_none on_trunc_sat;
    cb_u32 on_memory_init;
    cb_fc_none on_memory_copy;
    cb_fc_none on_memory_fill;
    cb_u32 on_data_drop;
    cb_u32_u32 on_table_init;
    cb_u32 on_elem_drop;
    cb_u32_u32 on_table_copy;
    cb_u32 on_table_grow;
    cb_u32 on_table_size;
    cb_u32 on_table_fill;
    cb_fd_none on_opcode_fd;
    cb_decode_end on_decode_end;
} op_decoder_callbacks;

r decode_function(module * mod, func * f, const op_decoder_callbacks * callbacks, void * payload);



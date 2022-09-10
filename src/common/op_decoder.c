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

#include "op_decoder.h"

#include "stream.h"

// clang-format off
static const func_type s_block_types[] = {
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x6f"}}, // externref
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x70"}}, // funcref
    {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0}, {0},                // 0x71 - 0x7a
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x7b"}}, // v128
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x7c"}}, // f64
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x7d"}}, // f32
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x7e"}}, // i64
    {.result_count = 1, .results = {.len = 1, .ptr = (u8p) "\x7f"}}, // i32
};
// clang-format on
#define to_func_type(id) (&s_block_types[id - TYPE_ID_externref])

static r_func_type get_blocktype(stream * st, module * mod) {
    // Value types can occur in contexts where type indices are also allowed,
    // such as in the case of block types. Thus, the binary format for types
    // corresponds to the signed LEB128 encoding of small negative  values,
    // so that they can coexist with (positive) type indices in the future.
    check_prep(r_func_type);
    unwrap(i8, blocktype, stream_read_vi7(st));
    func_type ft = {0};
    if (unlikely(blocktype != TYPE_ID_void_)) {
        if (is_value_type(blocktype)) {
            ft = *to_func_type(blocktype);
        } else if (blocktype >= 0) {
            if ((u32)(blocktype) >= vec_size_func_type(&mod->func_types)) {
                return err(e_malformed, "Invalid blocktype index");
            }
            ft = *vec_at_func_type(&mod->func_types, blocktype);
        } else {
            return err(e_malformed, "Invalid block value type");
        }
    }
    return ok(ft);
}

#define callback(name, ...)                      \
    do {                                         \
        if (callbacks->name) {                   \
            check(callbacks->name(__VA_ARGS__)); \
        }                                        \
    } while (0)

r decode_function(module * mod, func * f, const op_decoder_callbacks * callbacks, void * payload) {
    assert(mod);
    assert(f);
    assert(callbacks);
    check_prep(r);

    stream code = stream_from(f->code);
    stream * pc = &code;

    callback(on_decode_begin, payload);

    while (true) {
        unwrap(u8, opcode, stream_read_u8(pc));
        callback(on_opcode, payload, opcode, code);
        switch (opcode) {
            case op_unreachable: {
                callback(on_unreachable, payload, opcode, code);
                continue;
            }
            case op_nop: {
                callback(on_nop, payload, opcode, code);
                continue;
            }
            case op_block:
            case op_loop:
            case op_if: {
                stream imm = code;
                unwrap(func_type, type, get_blocktype(pc, mod));
                callback(on_block, payload, opcode, imm, type);
                continue;
            }
            case op_else: {
                callback(on_else, payload, opcode, code);
                continue;
            }
            case op_end: {
                callback(on_end, payload, opcode, code);
                if (!stream_remaining(pc)) {
                    goto finish;
                } else {
                    continue;
                }
            }
            case op_br:
            case op_br_if: {
                stream imm = code;
                unwrap(u8, lth, stream_read_u8(pc));
                callback(on_br_or_if, payload, opcode, imm, lth);
                continue;
            }
            case op_br_table: {
                callback(on_br_table, payload, opcode, code);
                unwrap(u32, table_len, stream_read_vu32(pc));
                check(stream_seek(pc, table_len + 1));
                continue;
            }
            case op_return: {
                callback(on_return, payload, opcode, code);
                continue;
            }
            case op_call: {
                stream imm = code;
                unwrap(u32, func_idx, stream_read_vu32(pc));
                callback(on_call, payload, imm, func_idx);
                continue;
            }
            case op_call_indirect: {
                stream imm = code;
                unwrap(u32, type_idx, stream_read_vu32(pc));
                unwrap(u32, table_idx, stream_read_vu32(pc));
                callback(on_call_indirect, payload, imm, type_idx, table_idx);
                continue;
            }
            case op_drop: {
                callback(on_drop, payload, opcode, code);
                continue;
            }
            case op_select: {
                callback(on_select, payload, opcode, code);
                continue;
            }
            case op_select_t: {
                stream imm = code;
                unwrap(i8, n, stream_read_vi7(pc));
                if (n != 1) {
                    return err(e_invalid, "invalid select t type count");
                }
                unwrap(i8, t, stream_read_vi7(pc));
                if (!is_value_type(t)) {
                    return err(e_invalid, "invalid select t value type");
                }
                callback(on_select_t, payload, opcode, imm, t);
                continue;
            }
            case op_local_get: {
                stream imm = code;
                unwrap(u32, local_idx, stream_read_vu32(pc));
                callback(on_local_get, payload, imm, local_idx);
                continue;
            }
            case op_local_set: {
                stream imm = code;
                unwrap(u32, local_idx, stream_read_vu32(pc));
                callback(on_local_set, payload, imm, local_idx);
                continue;
            }
            case op_local_tee: {
                stream imm = code;
                unwrap(u32, local_idx, stream_read_vu32(pc));
                callback(on_local_tee, payload, imm, local_idx);
                continue;
            }
            case op_global_get: {
                stream imm = code;
                unwrap(u32, global_idx, stream_read_vu32(pc));
                callback(on_global_get, payload, imm, global_idx);
                continue;
            }
            case op_global_set: {
                stream imm = code;
                unwrap(u32, global_idx, stream_read_vu32(pc));
                callback(on_global_set, payload, imm, global_idx);
                continue;
            }
            case op_table_get: {
                stream imm = code;
                unwrap(u32, table_idx, stream_read_vu32(pc));
                callback(on_table_get, payload, imm, table_idx);
                continue;
            }
            case op_table_set: {
                stream imm = code;
                unwrap(u32, table_idx, stream_read_vu32(pc));
                callback(on_table_set, payload, imm, table_idx);
                continue;
            }
            case op_i32_load:
            case op_i32_load8_s:
            case op_i32_load8_u:
            case op_i32_load16_s:
            case op_i32_load16_u:
            case op_i64_load:
            case op_i64_load8_s:
            case op_i64_load8_u:
            case op_i64_load16_s:
            case op_i64_load16_u:
            case op_i64_load32_s:
            case op_i64_load32_u:
            case op_f32_load:
            case op_f64_load:
            case op_i32_store:
            case op_i32_store8:
            case op_i32_store16:
            case op_i64_store:
            case op_i64_store8:
            case op_i64_store16:
            case op_i64_store32:
            case op_f32_store:
            case op_f64_store: {
                stream imm = code;
                unwrap(u8, align, stream_read_vu7(pc));
                unwrap(u32, offset, stream_read_vu32(pc));
                callback(on_memory_load_store, payload, opcode, imm, align, offset);
                continue;
            }

            case op_memory_size: {
                stream imm = code;
                unwrap(u8, reserved_byte, stream_read_vu7(pc));
                if (reserved_byte) {
                    return err(e_malformed, "currently memory.size reserved byte should be zero");
                }
                callback(on_memory_size, payload, opcode, imm);
                continue;
            }
            case op_memory_grow: {
                stream imm = code;
                unwrap(u8, reserved_byte, stream_read_vu7(pc));
                if (reserved_byte) {
                    return err(e_malformed, "currently memory.grow reserved byte should be zero");
                }
                callback(on_memory_grow, payload, opcode, imm);
                continue;
            }

            case op_i32_const: {
                stream imm = code;
                unwrap(i32, val, stream_read_vi32(pc));
                callback(on_i32_const, payload, imm, val);
                continue;
            }
            case op_i64_const: {
                stream imm = code;
                unwrap(i64, val, stream_read_vi64(pc));
                callback(on_i64_const, payload, imm, val);
                continue;
            }
            case op_f32_const: {
                stream imm = code;
                unwrap(f32, val, stream_read_f32(pc));
                callback(on_f32_const, payload, imm, val);
                continue;
            }
            case op_f64_const: {
                stream imm = code;
                unwrap(f64, val, stream_read_f64(pc));
                callback(on_f64_const, payload, imm, val);
                continue;
            }

            case op_i32_eqz:
            case op_i64_eqz: {
                callback(on_itestop, payload, opcode, code);
                continue;
            }

            case op_i32_clz:
            case op_i64_clz:
            case op_i32_ctz:
            case op_i64_ctz:
            case op_i32_popcnt:
            case op_i64_popcnt: {
                callback(on_iunop, payload, opcode, code);
                continue;
            }

            case op_i32_eq:
            case op_i32_ne:
            case op_i32_lt_s:
            case op_i32_lt_u:
            case op_i32_gt_s:
            case op_i32_gt_u:
            case op_i32_le_s:
            case op_i32_le_u:
            case op_i32_ge_s:
            case op_i32_ge_u:
            case op_i64_eq:
            case op_i64_ne:
            case op_i64_lt_s:
            case op_i64_lt_u:
            case op_i64_gt_s:
            case op_i64_gt_u:
            case op_i64_le_s:
            case op_i64_le_u:
            case op_i64_ge_s:
            case op_i64_ge_u: {
                callback(on_irelop, payload, opcode, code);
                continue;
            }

            case op_i32_add:
            case op_i32_sub:
            case op_i32_mul:
            case op_i32_div_s:
            case op_i32_div_u:
            case op_i32_rem_s:
            case op_i32_rem_u:
            case op_i32_and:
            case op_i32_or:
            case op_i32_xor:
            case op_i32_shl:
            case op_i32_shr_s:
            case op_i32_shr_u:
            case op_i32_rotl:
            case op_i32_rotr:
            case op_i64_add:
            case op_i64_sub:
            case op_i64_mul:
            case op_i64_div_s:
            case op_i64_div_u:
            case op_i64_rem_s:
            case op_i64_rem_u:
            case op_i64_and:
            case op_i64_or:
            case op_i64_xor:
            case op_i64_shl:
            case op_i64_shr_s:
            case op_i64_shr_u:
            case op_i64_rotl:
            case op_i64_rotr: {
                callback(on_ibinop, payload, opcode, code);
                continue;
            }

            case op_f32_abs:
            case op_f32_neg:
            case op_f32_ceil:
            case op_f32_floor:
            case op_f32_trunc:
            case op_f32_nearest:
            case op_f32_sqrt:
            case op_f64_abs:
            case op_f64_neg:
            case op_f64_ceil:
            case op_f64_floor:
            case op_f64_trunc:
            case op_f64_nearest:
            case op_f64_sqrt: {
                callback(on_funop, payload, opcode, code);
                continue;
            }

            case op_f32_eq:
            case op_f32_ne:
            case op_f32_lt:
            case op_f32_gt:
            case op_f32_le:
            case op_f32_ge:
            case op_f64_eq:
            case op_f64_ne:
            case op_f64_lt:
            case op_f64_gt:
            case op_f64_le:
            case op_f64_ge: {
                callback(on_frelop, payload, opcode, code);
                continue;
            }

            case op_f32_add:
            case op_f32_sub:
            case op_f32_mul:
            case op_f32_div:
            case op_f32_min:
            case op_f32_max:
            case op_f32_copysign:
            case op_f64_add:
            case op_f64_sub:
            case op_f64_mul:
            case op_f64_div:
            case op_f64_min:
            case op_f64_max:
            case op_f64_copysign: {
                callback(on_fbinop, payload, opcode, code);
                continue;
            }

            case op_i32_wrap_i64: {
                callback(on_wrapop, payload, opcode, code);
                continue;
            }

            case op_i32_trunc_f32_s:
            case op_i32_trunc_f32_u:
            case op_i32_trunc_f64_s:
            case op_i32_trunc_f64_u:
            case op_i64_trunc_f32_s:
            case op_i64_trunc_f32_u:
            case op_i64_trunc_f64_s:
            case op_i64_trunc_f64_u: {
                callback(on_truncop, payload, opcode, code);
                continue;
            }

            case op_f32_convert_i32_s:
            case op_f32_convert_i32_u:
            case op_f32_convert_i64_s:
            case op_f32_convert_i64_u:
            case op_f64_convert_i32_s:
            case op_f64_convert_i32_u:
            case op_f64_convert_i64_s:
            case op_f64_convert_i64_u: {
                callback(on_convertop, payload, opcode, code);
                continue;
            }

            case op_f32_demote_f64:
            case op_f64_promote_f32: {
                callback(on_rankop, payload, opcode, code);
                continue;
            }

            case op_i32_reinterpret_f32:
            case op_i64_reinterpret_f64:
            case op_f32_reinterpret_i32:
            case op_f64_reinterpret_i64: {
                callback(on_reinterpretop, payload, opcode, code);
                continue;
            }

            case op_i64_extend_i32_s:
            case op_i64_extend_i32_u:
            case op_i32_extend8_s:
            case op_i32_extend16_s:
            case op_i64_extend8_s:
            case op_i64_extend16_s:
            case op_i64_extend32_s: {
                callback(on_extendop, payload, opcode, code);
                continue;
            }

            case op_ref_null: {
                stream imm = code;
                unwrap(i8, ref_type, stream_read_vi7(pc));
                if (!is_ref(ref_type)) {
                    return err(e_invalid, "Invalid reference type");
                }
                callback(on_ref_null, payload, opcode, imm, ref_type);
                continue;
            }
            case op_ref_is_null: {
                callback(on_ref_is_null, payload, opcode, code);
                continue;
            }

            case op_ref_func: {
                stream imm = code;
                unwrap(u32, fref, stream_read_vu32(pc));
                callback(on_ref_func, payload, imm, fref);
                continue;
            }

            case op_prefix_fc: {
                unwrap(u32, opcode_fc, stream_read_vu32(pc));
                callback(on_opcode_fc, payload, opcode_fc, code);
                switch (opcode_fc) {
                    case op_i32_trunc_sat_f32_s:
                    case op_i32_trunc_sat_f32_u:
                    case op_i32_trunc_sat_f64_s:
                    case op_i32_trunc_sat_f64_u:
                    case op_i64_trunc_sat_f32_s:
                    case op_i64_trunc_sat_f32_u:
                    case op_i64_trunc_sat_f64_s:
                    case op_i64_trunc_sat_f64_u: {
                        callback(on_trunc_sat, payload, opcode_fc, code);
                        continue;
                    }
                    case op_memory_init: {
                        stream imm = code;
                        unwrap(u32, data_idx, stream_read_vu32(pc));
                        unwrap(u8, reserved_byte, stream_read_vu7(pc));
                        if (reserved_byte) {
                            return err(e_malformed, "currently memory.x reserved byte should be zero");
                        }
                        callback(on_memory_init, payload, imm, data_idx);
                        continue;
                    }
                    case op_memory_copy: {
                        stream imm = code;
                        unwrap(u8, mem_idx_src, stream_read_vu7(pc));
                        unwrap(u8, mem_idx_dst, stream_read_vu7(pc));
                        if (mem_idx_dst || mem_idx_src) {
                            return err(e_malformed, "currently memory.x reserved byte should be zero");
                        }
                        callback(on_memory_copy, payload, opcode_fc, imm);
                        continue;
                    }
                    case op_memory_fill: {
                        stream imm = code;
                        unwrap(u8, reserved_byte, stream_read_vu7(pc));
                        if (reserved_byte) {
                            return err(e_malformed, "currently memory.x reserved byte should be zero");
                        }
                        callback(on_memory_fill, payload, opcode_fc, imm);
                        continue;
                    }
                    case op_data_drop: {
                        stream imm = code;
                        unwrap(u32, data_idx, stream_read_vu32(pc));
                        callback(on_data_drop, payload, imm, data_idx);
                        continue;
                    }
                    case op_table_init: {
                        stream imm = code;
                        unwrap(u32, elem_idx, stream_read_vu32(pc));
                        unwrap(u32, table_idx, stream_read_vu32(pc));
                        callback(on_table_init, payload, imm, elem_idx, table_idx);
                        continue;
                    }
                    case op_elem_drop: {
                        stream imm = code;
                        unwrap(u32, elem_idx, stream_read_vu32(pc));
                        callback(on_elem_drop, payload, imm, elem_idx);
                        continue;
                    }
                    case op_table_copy: {
                        stream imm = code;
                        unwrap(u32, table_idx_dst, stream_read_vu32(pc));
                        unwrap(u32, table_idx_src, stream_read_vu32(pc));
                        callback(on_table_copy, payload, imm, table_idx_dst, table_idx_src);
                        continue;
                    }
                    case op_table_grow: {
                        stream imm = code;
                        unwrap(u32, table_idx, stream_read_vu32(pc));
                        callback(on_table_grow, payload, imm, table_idx);
                        continue;
                    }
                    case op_table_size: {
                        stream imm = code;
                        unwrap(u32, table_idx, stream_read_vu32(pc));
                        callback(on_table_size, payload, imm, table_idx);
                        continue;
                    }
                    case op_table_fill: {
                        stream imm = code;
                        unwrap(u32, table_idx, stream_read_vu32(pc));
                        callback(on_table_fill, payload, imm, table_idx);
                        continue;
                    }
                    default: {
                        return err(e_invalid, "Unsupported opcode");
                    }
                }
            }
            case op_prefix_fd: {
                unwrap(u32, opcode_fd, stream_read_vu32(pc));
                callback(on_opcode_fd, payload, opcode_fd, code);
                switch (opcode_fd) {
                    default: {
                        return err(e_invalid, "Unsupported opcode");
                    }
                }
                continue;
            }
            default: {
                return err(e_malformed, "Invalid opcode");
            }
        }
    }
finish:;
    callback(on_decode_end, payload);
    return ok_r;
}

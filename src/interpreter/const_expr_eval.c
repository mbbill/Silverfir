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

#include "interpreter.h"
#include "opcode.h"
#include "stream.h"
#include "vec.h"

// Reducing constant expressions into one output value.
// The arity needs to be one.
// Unlike the main reducer, this function will also validate the sequence.
r_typed_value interp_reduce_const_expr(module_inst * mod_inst, stream code, bool passive_elem) {
    assert(mod_inst);
    check_prep(r_typed_value);

    // it's actually unnecessary to use a stack because there's no stack pop instructions
    // so as long as the stack exceeds 1, it's and error. But I'll still keep it in case of
    // future wasm changes.
    vec_typed_value value_stack = {0};

    while (stream_remaining(&code)) {
        unwrap(u8, opcode, stream_read_u8(&code));
        if ((passive_elem) && (opcode != op_ref_null) && (opcode != op_ref_func) && (opcode != op_end)) {
            vec_clear_typed_value(&value_stack);
            return err(e_malformed, "illegal opcode");
        }
        switch (opcode) {
            case op_i32_const: {
                unwrap(i32, v, stream_read_vi32(&code));
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = TYPE_ID_i32,
                                                             .val.u_i32 = v,
                                                         }));
                continue;
            }
            case op_i64_const: {
                unwrap(i64, v, stream_read_vi64(&code));
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = TYPE_ID_i64,
                                                             .val.u_i64 = v,
                                                         }));
                continue;
            }
            case op_f32_const: {
                unwrap(f32, v, stream_read_f32(&code));
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = TYPE_ID_f32,
                                                             .val.u_f32 = v,
                                                         }));
                continue;
            }
            case op_f64_const: {
                unwrap(f64, v, stream_read_f64(&code));
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = TYPE_ID_f64,
                                                             .val.u_f64 = v,
                                                         }));
                continue;
            }
            case op_ref_null: {
                unwrap(i8, type, stream_read_vi7(&code));
                if (!is_ref(type)) {
                    return err(e_invalid, "Invalid reference type after ref.null");
                }
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = type,
                                                             .val.u_ref = nullref,
                                                         }));
                continue;
            }
            case op_ref_func: {
                unwrap(u32, f_idx, stream_read_vu32(&code));
                ref fref = to_ref(*vec_at_func_addr(&mod_inst->f_addrs, f_idx));
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = TYPE_ID_funcref,
                                                             .val.u_ref = fref,
                                                         }));
                continue;
            }
            case op_global_get: {
                unwrap(u32, idx, stream_read_vu32(&code));
                if (idx >= vec_size_global_inst(&mod_inst->globals)) {
                    return err(e_invalid, "Invalid global index");
                }
                glob_addr g_addr = *vec_at_glob_addr(&mod_inst->g_addrs, idx);
                assert(g_addr);
                global * g = g_addr->glob;
                assert(g);
                check(vec_push_typed_value(&value_stack, (typed_value){
                                                             .type = g->valtype,
                                                             .val = g_addr->gvalue,
                                                         }));
                continue;
            }
            case op_end: {
                goto end_of_loop;
            }
            default: {
                vec_clear_typed_value(&value_stack);
                return err(e_invalid, "illegal opcode");
            }
        }
    }

end_of_loop:
    if (vec_size_typed_value(&value_stack) != 1) {
        vec_clear_typed_value(&value_stack);
        return err(e_invalid, "illegal opcode");
    }
    typed_value val = *vec_back_typed_value(&value_stack);
    vec_clear_typed_value(&value_stack);
    return ok(val);
}

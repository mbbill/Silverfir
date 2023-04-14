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

// An interpreter implementation with the most basic switch-case loop or
// direct threading (computed goto) if available.
#include "interpreter.h"
#include "mem_util.h"
#include "opcode.h"
#include "silverfir.h"
#include "smath.h"
#include "stream.h"
#include "logger.h"
#include "vec.h"

#if SILVERFIR_INTERP_INPLACE_DT

#define LOGI(fmt, ...) LOG_INFO(log_channel_in_place_dt, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_in_place_dt, fmt, ##__VA_ARGS__)

r in_place_dt_call(thread * t, func_addr f_addr, value_u * args) {
    assert(f_addr);
    assert(args);
    check_prep(r);

    if (unlikely(t->frame_depth > SILVERFIR_STACK_FRAME_LIMIT)) {
        return err(e_exhaustion, "Stack frame reached limit");
    }

    if (unlikely(t->stack_size > SILVERFIR_STACK_SIZE_LIMIT)) {
        return err(e_exhaustion, "Stack reached size limit");
    }

#define pop() (*--sp)
#define pop_drop() (--sp)
#define push(val) ((*sp++) = val)

    t->frame_depth++;
    func * fn = f_addr->fn;
    module_inst * mod_inst = f_addr->mod_inst;

    // zero-out the reset of the locals. This is *required* by the spec.
    register value_u * local = args;
    memset(local + fn->fn_type.param_count, 0, (fn->local_count - fn->fn_type.param_count) * sizeof(value_u));
    // local and stack are NOT continuous! locals belongs to the caller's stack frame
    // whereas the stack is allocated in this frame
    value_u * stack_base = array_alloca(value_u, fn->stack_size_max);
    if (!stack_base) {
        return err(e_general, "Stack overflow!");
    }
    t->stack_size += fn->stack_size_max;
    register value_u * sp = stack_base;

    str code = fn->code;
    register const u8 * pc = code.ptr;

    mem_addr mem_inst0 = NULL;
    // we CANNOT cache the memory size or u8 pointer here because the memory can grow in
    // the callee frames, and when the execution returns here, the cached size and pointer
    // will be invalid.
    // size_t mem0_size = 0;
    vec_u8 * pmem0 = NULL;
    if (vec_size_mem_addr(&mod_inst->m_addrs)) {
        mem_inst0 = *vec_at_mem_addr(&mod_inst->m_addrs, 0);
        pmem0 = &mem_inst0->mdata;
    }

    // the next_jt_idx also needs to be backed up like pc.
    jump_table * jt = fn->jt._data;
    size_t jt_size = vec_size_jump_table(&fn->jt);
    UNUSED(jt_size);
    u16 next_jt_idx = 0;

    register u8 opcode;
    while (true) {
        opcode = stream_read_u8_unchecked(pc);
#ifdef LOG_INFO_ENABLED
        {
            const char * op_name = get_op_name(opcode);
            u32 op_offset = pc - mod_inst->mod->wasm_bin.s.ptr;
            LOGI("%08x locals:%-3u stack:%-3u |%s", op_offset, fn->local_count, (u32)(sp-stack_base), op_name);
        }
#endif // LOG_INFO_ENABLED
        // clang-format off
#if !defined(HAS_COMPUTED_GOTO)
        #define OP(name, stmt) case op_##name: { stmt } continue;
        switch (opcode) {
#else
        #define OP(name, stmt) l_##name:; { stmt } continue;
        #define OPCODE_JUMP_LABEL(name, _1, _2, _3) [op_##name] = &&l_##name,
        static const void * const opcode_map[] = { FOR_EACH_WASM_OPCODE(OPCODE_JUMP_LABEL) };
        goto *opcode_map[opcode];
#endif
            // clang-format on
            OP(unreachable, {
                // stack-polymorphic
                return err(e_general, "unreachable: unreachable");
            });
            OP(nop, {});
            OP(block, {
                stream_seek_unchecked(pc, 1);
            });
            OP(loop, {
                stream_seek_unchecked(pc, 1);
            });
            OP(if, {
                i32 c = pop().u_i32;
                if (c) {
                    stream_seek_unchecked(pc, 1); //block type
                    next_jt_idx++;
                } else {
                    jump_table * tbl = jt + next_jt_idx;
                    assert(next_jt_idx < jt_size || next_jt_idx == JUMP_TABLE_IDX_INVALID);
                    assert(code.ptr + tbl->pc == pc);
                    next_jt_idx = tbl->next_idx;
                    pc += tbl->target_offset;
                }
            });
            OP(else, {
                jump_table * tbl = jt + next_jt_idx;
                assert(next_jt_idx < jt_size || next_jt_idx == JUMP_TABLE_IDX_INVALID);
                assert(code.ptr + tbl->pc == pc);
                next_jt_idx = tbl->next_idx;
                pc += tbl->target_offset;
            });
            OP(end, {
                // it's the end of the function.
                if (unlikely(pc == code.ptr + code.len)) {
                    goto end;
                }
            });
            OP(br, {
                handle_br:;
                    jump_table * tbl = jt + next_jt_idx;
                    assert(next_jt_idx < jt_size || next_jt_idx == JUMP_TABLE_IDX_INVALID);
                    assert(code.ptr + tbl->pc == pc);
                    next_jt_idx = tbl->next_idx;
                    pc += tbl->target_offset;
                    if (unlikely(tbl->stack_offset)) {
                        u32 stack_offset = tbl->stack_offset;
                        u16 arity = tbl->arity;
                        assert(stack_offset < 32767);
                        assert(sp - stack_base >= stack_offset + arity);
                        value_u * dst = sp - stack_offset - arity;
                        value_u * src = sp - arity;
                        memmove(dst, src, sizeof(value_u) * arity);
                        sp -= stack_offset;
                    }
            });
            OP(br_if, {
                i32 c = pop().u_i32;
                if (c) {
                    goto handle_br;
                } else {
                    stream_seek_unchecked(pc, 1); //lth
                    next_jt_idx++;
                }
            });
            OP(br_table, {
                u32 i = pop().u_u32;
                u32 table_len;
                stream_read_vu32_unchecked(table_len, pc);
                stream_seek_unchecked(pc, table_len + 1);
                if (i > table_len) {
                    i = table_len;
                }
                next_jt_idx += (u16)i;
                goto handle_br;
            });
            OP(return, {
                goto end;
            });
            OP(call, {
                u32 fn_idx_local;
                stream_read_vu32_unchecked(fn_idx_local, pc);
                assert(fn_idx_local < vec_size_func(&mod_inst->mod->funcs));
                func_addr callee_addr = *vec_at_func_addr(&mod_inst->f_addrs, fn_idx_local);
                func * callee_fn = callee_addr->fn;
                func_type callee_type = callee_fn->fn_type;
                assert(sp - stack_base >= callee_type.param_count);
                sp -= callee_type.param_count;
                if (unlikely(callee_fn->tr)) {
                    // native call, we're passing in the caller's context.
                    check(callee_fn->tr((tr_ctx){
                                            .f_addr = callee_addr,
                                            .args = sp,
                                            .mem0 = mem_inst0,
                                        },
                                        callee_fn->host_func));
                } else {
                    check(in_place_dt_call(t, callee_addr, sp));
                }
                sp += callee_type.result_count;
            });
            OP(call_indirect, {
                // source
                u32 type_idx;
                stream_read_vu32_unchecked(type_idx, pc);
                func_type src_type = *vec_at_func_type(&mod_inst->mod->func_types, type_idx);
                // target
                u32 table_idx;
                stream_read_vu32_unchecked(table_idx, pc);
                assert(table_idx < vec_size_tab_addr(&mod_inst->t_addrs));
                tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                size_t i = pop().u_i32;
                if (i >= vec_size_ref(&t_addr->tdata)) {
                    return err(e_general, "call_indirect: invalid table element index");
                }
                ref fref = *vec_at_ref(&t_addr->tdata, i);
                if (fref == nullref) {
                    return err(e_general, "call_indirect: element is ref.null");
                }
                func_addr callee_addr = to_func_addr(fref);
                func * callee_fn = callee_addr->fn;
                func_type callee_type = callee_fn->fn_type;
                if (!func_type_eq(callee_type, src_type)) {
                    return err(e_general, "call_indirect: function type mismatch");
                }
                assert(sp - stack_base >= callee_type.param_count);
                sp -= callee_type.param_count;
                if (unlikely(callee_fn->tr)) {
                    // native call, we're passing in the caller's context.
                    check(callee_fn->tr((tr_ctx){
                                            .f_addr = callee_addr,
                                            .args = sp,
                                            .mem0 = mem_inst0,
                                        },
                                        callee_fn->host_func));
                } else {
                    check(in_place_dt_call(t, callee_addr, sp));
                }
                sp += callee_type.result_count;
            });
            OP(drop, {
                pop_drop();
            });
            OP(select, {
                handle_op_select:;
                    i32 cond = pop().u_i32;
                    value_u false_val = pop();
                    value_u true_val = pop();
                    if (cond) {
                        push(true_val);
                    } else {
                        push(false_val);
                    }
            });
            OP(select_t, {
                stream_seek_unchecked(pc, 2);
                goto handle_op_select;
            });
            OP(local_get, {
                u32 local_idx;
                stream_read_vu32_unchecked(local_idx, pc);
                push(local[local_idx]);
            });
            OP(local_set, {
                u32 local_idx;
                value_u v = pop();
                stream_read_vu32_unchecked(local_idx, pc);
                local[local_idx] = v;
            });
            OP(local_tee, {
                u32 local_idx;
                value_u v = *(sp - 1);
                stream_read_vu32_unchecked(local_idx, pc);
                local[local_idx] = v;
            });
            OP(global_get, {
                u32 global_idx;
                stream_read_vu32_unchecked(global_idx, pc);
                assert(global_idx < vec_size_global_inst(&mod_inst->globals));
                glob_addr g_addr = *vec_at_glob_addr(&mod_inst->g_addrs, global_idx);
                push(g_addr->gvalue);
            });
            OP(global_set, {
                u32 global_idx;
                stream_read_vu32_unchecked(global_idx, pc);
                assert(global_idx < vec_size_global_inst(&mod_inst->globals));
                glob_addr g_addr = *vec_at_glob_addr(&mod_inst->g_addrs, global_idx);
                g_addr->gvalue = pop();
            });
            OP(table_get, {
                u32 table_idx;
                stream_read_vu32_unchecked(table_idx, pc);
                assert(table_idx < vec_size_tab_addr(&mod_inst->t_addrs));
                tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                u32 elem_idx = pop().u_i32;
                if (elem_idx >= vec_size_ref(&t_addr->tdata)) {
                    return err(e_general, "table_get: invalid table element index");
                }
                value_u ref = {.u_ref = *vec_at_ref(&t_addr->tdata, elem_idx)};
                push(ref);
            });
            OP(table_set, {
                u32 table_idx;
                stream_read_vu32_unchecked(table_idx, pc);
                assert(table_idx < vec_size_tab_addr(&mod_inst->t_addrs));
                tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                ref ref = pop().u_ref;
                u32 elem_idx = pop().u_u32;
                if (elem_idx >= vec_size_ref(&t_addr->tdata)) {
                    return err(e_general, "table_set: invalid table element index");
                }
                *vec_at_ref(&t_addr->tdata, elem_idx) = ref;
            });
// although the spec says the popped up value is a signed i32, it should be treated
// as unsigned. Furthormore, the size should be widened to avoid overflow.
#define MEM_LOAD(dst_type, src_type)                                                    \
    {                                                                                   \
        stream_seek_unchecked(pc, 1);                                                   \
        u32 offset;                                                                     \
        stream_read_vu32_unchecked(offset, pc);                                         \
        u64 mem_idx = (u64)(u32)(pop().u_i32) + offset;                                 \
        if (unlikely(mem_idx + sizeof(src_type) > vec_size_u8(pmem0))) {                \
            return err(e_general, "t.load: out-of-bound memory access");                           \
        }                                                                               \
        value_u val = {.u_##dst_type = mem_read_##src_type(vec_at_u8(pmem0, mem_idx))}; \
        push(val);                                                                      \
    }

            OP(i32_load, {MEM_LOAD(i32, i32)});
            OP(i64_load, {MEM_LOAD(i64, i64)});
            OP(f32_load, {MEM_LOAD(f32, f32)});
            OP(f64_load, {MEM_LOAD(f64, f64)});
            OP(i32_load8_s, {MEM_LOAD(i32, i8)});
            OP(i32_load8_u, {MEM_LOAD(i32, u8)});
            OP(i32_load16_s, {MEM_LOAD(i32, i16)});
            OP(i32_load16_u, {MEM_LOAD(i32, u16)});
            OP(i64_load8_s, {MEM_LOAD(i64, i8)});
            OP(i64_load8_u, {MEM_LOAD(i64, u8)});
            OP(i64_load16_s, {MEM_LOAD(i64, i16)});
            OP(i64_load16_u, {MEM_LOAD(i64, u16)});
            OP(i64_load32_s, {MEM_LOAD(i64, i32)});
            OP(i64_load32_u, {MEM_LOAD(i64, u32)});

#define MEM_STORE(dst_type, src_type)                                                    \
    {                                                                                    \
        stream_seek_unchecked(pc, 1);                                                    \
        u32 offset;                                                                      \
        stream_read_vu32_unchecked(offset, pc);                                          \
        value_u val = pop();                                                             \
        u64 mem_idx = (u64)(u32)(pop().u_i32) + offset;                                  \
        if (unlikely(mem_idx + sizeof(dst_type) > vec_size_u8(pmem0))) {                 \
            return err(e_general, "t.store: out-of-bound memory access");                           \
        }                                                                                \
        mem_write_##dst_type(vec_at_u8(pmem0, mem_idx), ((dst_type)(val.u_##src_type))); \
    }

            OP(i32_store, {MEM_STORE(u32, i32)});
            OP(i64_store, {MEM_STORE(u64, i64)});
            OP(f32_store, {MEM_STORE(f32, f32)});
            OP(f64_store, {MEM_STORE(f64, f64)});
            OP(i32_store8, {MEM_STORE(u8, i32)});
            OP(i32_store16, {MEM_STORE(u16, i32)});
            OP(i64_store8, {MEM_STORE(u8, i64)});
            OP(i64_store16, {MEM_STORE(u16, i64)});
            OP(i64_store32, {MEM_STORE(u32, i64)});

            OP(memory_size, {
                stream_seek_unchecked(pc, 1);
                push((value_u){.u_i32 = (i32)(vec_size_u8(pmem0) / WASM_PAGE_SIZE)});
            });
            OP(memory_grow, {
                stream_seek_unchecked(pc, 1);
                size_t sz = vec_size_u8(pmem0) / WASM_PAGE_SIZE;
                size_t n_pages = (size_t)(pop().u_u32);
                if (sz + n_pages <= mem_inst0->mem->lim.max) {
                    r ret_resize = vec_resize_u8(pmem0, (sz + n_pages) * WASM_PAGE_SIZE);
                    if (is_ok(ret_resize)) {
                        push((value_u){.u_i32 = (i32)(sz)});
                        continue;
                    }
                }
                // failed
                push((value_u){.u_i32 = (i32)(-1)});
            });
            OP(i32_const, {
                value_u v;
                stream_read_vi32_unchecked(v.u_i32, pc);
                push(v);
            });
            OP(i64_const, {
                value_u v;
                stream_read_vi64_unchecked(v.u_i64, pc);
                push(v);
            });
            OP(f32_const, {
                value_u v;
                stream_read_f32_unchecked(v.u_f32, pc);
                push(v);
            });
            OP(f64_const, {
                value_u v;
                stream_read_f64_unchecked(v.u_f64, pc);
                push(v);
            });

#define FDIV_TRAP(type)                              \
    if (fpclassify((sp - 1)->u_##type) == FP_ZERO) { \
        return err(e_general, "div: divided by zero");          \
    }

#define UNOP(type, op)                                       \
    {                                                        \
        (sp - 1)->u_##type = (type)(op((sp - 1)->u_##type)); \
    }

#define BINOP(tgt_type, op, op_type)                                    \
    {                                                                   \
        value_u v2 = pop();                                             \
        tgt_type v = (tgt_type)(op(pop().u_##op_type, v2.u_##op_type)); \
        push((value_u){.u_##tgt_type = v});                             \
    }

// RELOP always produce i32 results.
#define RELOP(op_type, op) BINOP(i32, op, op_type)

#define CONVERT_OP(tgt_type, op, src_type)                               \
    {                                                                    \
        (sp - 1)->u_##tgt_type = (tgt_type)(op((sp - 1)->u_##src_type)); \
    }

#define REINTERPRET_OP(tgt_type, src_type)                                   \
    {                                                                        \
        assert(sizeof(tgt_type) == sizeof(src_type));                        \
        (sp - 1)->u_##tgt_type = *((tgt_type *)(&((sp - 1)->u_##src_type))); \
    }

            OP(i32_eqz, { UNOP(i32, s_eqz); });
            OP(i32_eq, { RELOP(i32, s_eq); });
            OP(i32_ne, { RELOP(i32, s_ne); });
            OP(i32_lt_s, { RELOP(i32, s_lt); });
            OP(i32_lt_u, { RELOP(u32, s_lt); });
            OP(i32_gt_s, { RELOP(i32, s_gt); });
            OP(i32_gt_u, { RELOP(u32, s_gt); });
            OP(i32_le_s, { RELOP(i32, s_le); });
            OP(i32_le_u, { RELOP(u32, s_le); });
            OP(i32_ge_s, { RELOP(i32, s_ge); });
            OP(i32_ge_u, { RELOP(u32, s_ge); });
            OP(i64_eqz, { UNOP(i64, s_eqz); });
            OP(i64_eq, { RELOP(i64, s_eq); });
            OP(i64_ne, { RELOP(i64, s_ne); });
            OP(i64_lt_s, { RELOP(i64, s_lt); });
            OP(i64_lt_u, { RELOP(u64, s_lt); });
            OP(i64_gt_s, { RELOP(i64, s_gt); });
            OP(i64_gt_u, { RELOP(u64, s_gt); });
            OP(i64_le_s, { RELOP(i64, s_le); });
            OP(i64_le_u, { RELOP(u64, s_le); });
            OP(i64_ge_s, { RELOP(i64, s_ge); });
            OP(i64_ge_u, { RELOP(u64, s_ge); });
            OP(f32_eq, { RELOP(f32, s_eq); });
            OP(f32_ne, { RELOP(f32, s_ne); });
            OP(f32_lt, { RELOP(f32, s_lt); });
            OP(f32_gt, { RELOP(f32, s_gt); });
            OP(f32_le, { RELOP(f32, s_le); });
            OP(f32_ge, { RELOP(f32, s_ge); });
            OP(f64_eq, { RELOP(f64, s_eq); });
            OP(f64_ne, { RELOP(f64, s_ne); });
            OP(f64_lt, { RELOP(f64, s_lt); });
            OP(f64_gt, { RELOP(f64, s_gt); });
            OP(f64_le, { RELOP(f64, s_le); });
            OP(f64_ge, { RELOP(f64, s_ge); });
            OP(i32_clz, { UNOP(i32, s_clz32); });
            OP(i32_ctz, { UNOP(i32, s_ctz32); });
            OP(i32_popcnt, { UNOP(i32, s_popcnt32); });
            OP(i32_add, { BINOP(i32, s_add, i32); });
            OP(i32_sub, { BINOP(i32, s_sub, i32); });
            OP(i32_mul, { BINOP(i32, s_mul, i32); });
            OP(i32_div_s, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely(((v1.u_i32 == i32_MIN) && (v2.u_i32 == -1)) || (v2.u_i32 == 0))) {
                    return err(e_general, "div trap");
                }
                push((value_u){.u_i32 = s_div(v1.u_i32, v2.u_i32)});
            });
            OP(i32_div_u, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (v2.u_u32 == 0) {
                    return err(e_general, "div trap");
                }
                push((value_u){.u_u32 = s_div(v1.u_u32, v2.u_u32)});
            });
            OP(i32_rem_s, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely((v1.u_i32 == i32_MIN) && (v2.u_i32 == -1))) {
                    push((value_u){.u_i32 = 0});
                    continue;
                }
                if (unlikely(v2.u_i32 == 0)) {
                    return err(e_general, "trap");
                }
                push((value_u){.u_i32 = s_rem(v1.u_i32, v2.u_i32)});
            });
            OP(i32_rem_u, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely(v2.u_u32 == 0)) {
                    return err(e_general, "rem trap");
                }
                push((value_u){.u_u32 = s_rem(v1.u_u32, v2.u_u32)});
            });
            OP(i32_and, { BINOP(i32, s_and, i32); });
            OP(i32_or, { BINOP(i32, s_or, i32); });
            OP(i32_xor, { BINOP(i32, s_xor, i32); });
            OP(i32_shl, { BINOP(i32, s_shl, i32); });
            OP(i32_shr_s, { BINOP(i32, s_shr, i32); });
            OP(i32_shr_u, { BINOP(i32, s_shr, u32); });
            OP(i32_rotl, { BINOP(i32, s_rotl32, u32); });
            OP(i32_rotr, { BINOP(i32, s_rotr32, u32); });
            OP(i64_clz, { UNOP(i64, s_clz64); });
            OP(i64_ctz, { UNOP(i64, s_ctz64); });
            OP(i64_popcnt, { UNOP(i64, s_popcnt64); });
            OP(i64_add, { BINOP(i64, s_add, i64); });
            OP(i64_sub, { BINOP(i64, s_sub, i64); });
            OP(i64_mul, { BINOP(i64, s_mul, i64); });
            OP(i64_div_s, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely(((v1.u_i64 == i64_MIN) && (v2.u_i64 == -1)) || (v2.u_i64 == 0))) {
                    return err(e_general, "div trap");
                }
                push((value_u){.u_i64 = s_div(v1.u_i64, v2.u_i64)});
            });
            OP(i64_div_u, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely(v2.u_u64 == 0)) {
                    return err(e_general, "div trap");
                }
                push((value_u){.u_u64 = s_div(v1.u_u64, v2.u_u64)});
            });
            OP(i64_rem_s, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely((v1.u_i64 == i64_MIN) && (v2.u_i64 == -1))) {
                    push((value_u){.u_i64 = 0});
                    continue;
                }
                if (unlikely(v2.u_i64 == 0)) {
                    return err(e_general, "trap");
                }
                push((value_u){.u_i64 = s_rem(v1.u_i64, v2.u_i64)});
            });
            OP(i64_rem_u, {
                value_u v2 = pop();
                value_u v1 = pop();
                if (unlikely(v2.u_u64 == 0)) {
                    return err(e_general, "rem trap");
                }
                push((value_u){.u_u64 = s_rem(v1.u_u64, v2.u_u64)});
            });
            OP(i64_and, { BINOP(i64, s_and, i64); });
            OP(i64_or, { BINOP(i64, s_or, i64); });
            OP(i64_xor, { BINOP(i64, s_xor, i64); });
            OP(i64_shl, { BINOP(i64, s_shl, i64); });
            OP(i64_shr_s, { BINOP(i64, s_shr, i64); });
            OP(i64_shr_u, { BINOP(i64, s_shr, u64); });
            OP(i64_rotl, { BINOP(i64, s_rotl64, u64); });
            OP(i64_rotr, { BINOP(i64, s_rotr64, u64); });

            // TODO: we might need a math lib that matches wasm standard..
            OP(f32_abs, { UNOP(f32, s_fabs32); });
            OP(f32_neg, { UNOP(f32, s_fneg32); });
            OP(f32_ceil, { UNOP(f32, s_ceil); });
            OP(f32_floor, { UNOP(f32, s_floor); });
            OP(f32_trunc, { UNOP(f32, s_trunc); });
            OP(f32_nearest, { UNOP(f32, s_rint); });
            OP(f32_sqrt, { UNOP(f32, s_sqrt); });
            OP(f32_add, { BINOP(f32, s_add, f32); });
            OP(f32_sub, { BINOP(f32, s_sub, f32); });
            OP(f32_mul, { BINOP(f32, s_mul, f32); });
            OP(f32_div, { BINOP(f32, s_div, f32); });
            OP(f32_min, { BINOP(f32, s_fmin32, f32); });
            OP(f32_max, { BINOP(f32, s_fmax32, f32); });
            OP(f32_copysign, { BINOP(f32, s_copysign32, f32); });
            OP(f64_abs, { UNOP(f64, s_fabs64); });
            OP(f64_neg, { UNOP(f64, s_fneg64); });
            OP(f64_ceil, { UNOP(f64, s_ceil); });
            OP(f64_floor, { UNOP(f64, s_floor); });
            OP(f64_trunc, { UNOP(f64, s_trunc); });
            OP(f64_nearest, { UNOP(f64, s_rint); });
            OP(f64_sqrt, { UNOP(f64, s_sqrt); });
            OP(f64_add, { BINOP(f64, s_add, f64); });
            OP(f64_sub, { BINOP(f64, s_sub, f64); });
            OP(f64_mul, { BINOP(f64, s_mul, f64); });
            OP(f64_div, { BINOP(f64, s_div, f64); });
            OP(f64_min, { BINOP(f64, s_fmin64, f64); });
            OP(f64_max, { BINOP(f64, s_fmax64, f64); });
            OP(f64_copysign, { BINOP(f64, s_copysign64, f64); });
            OP(i32_wrap_i64, { CONVERT_OP(i32, s_nop, i64); });
            OP(i32_trunc_f32_s, {
                if (unlikely(s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 < -2147483648.f || (sp - 1)->u_f32 >= 2147483648.f)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i32, s_truncf32i, f32);
            });
            OP(i32_trunc_f32_u, {
                if (unlikely(s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f || (sp - 1)->u_f32 >= 4294967296.f)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i32, s_truncf32u, f32);
            });
            OP(i32_trunc_f64_s, {
                if (unlikely(s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -2147483649. || (sp - 1)->u_f64 >= 2147483648.)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i32, s_truncf64i, f64);
            });
            OP(i32_trunc_f64_u, {
                if (unlikely(s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1. || (sp - 1)->u_f64 >= 4294967296.)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i32, s_truncf64u, f64);
            });
            OP(i64_extend_i32_s, { CONVERT_OP(i64, s_as_i32, i32); });
            OP(i64_extend_i32_u, { CONVERT_OP(i64, s_as_i32u, i32); });
            OP(i64_trunc_f32_s, {
                if (unlikely(s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 < -9223372036854775808.f || (sp - 1)->u_f32 >= 9223372036854775808.f)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i64, s_truncf32i, f32);
            });
            OP(i64_trunc_f32_u, {
                if (unlikely(s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f || (sp - 1)->u_f32 >= 18446744073709551616.f)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i64, s_truncf32u, f32);
            });
            OP(i64_trunc_f64_s, {
                if (unlikely(s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 < -9223372036854775808. || (sp - 1)->u_f64 >= 9223372036854775808.)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i64, s_truncf64i, f64);
            });
            OP(i64_trunc_f64_u, {
                if (unlikely(s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1. || (sp - 1)->u_f64 >= 18446744073709551616.)) {
                    return err(e_general, "trap");
                }
                CONVERT_OP(i64, s_truncf64u, f64);
            });
            OP(f32_convert_i32_s, { CONVERT_OP(f32, s_nop, i32); });
            OP(f32_convert_i32_u, { CONVERT_OP(f32, s_nop, u32); });
            OP(f32_convert_i64_s, { CONVERT_OP(f32, s_nop, i64); });
            OP(f32_convert_i64_u, { CONVERT_OP(f32, s_nop, u64); });
            OP(f32_demote_f64, { CONVERT_OP(f32, s_nop, f64); });
            OP(f64_convert_i32_s, { CONVERT_OP(f64, s_nop, i32); });
            OP(f64_convert_i32_u, { CONVERT_OP(f64, s_nop, u32); });
            OP(f64_convert_i64_s, { CONVERT_OP(f64, s_nop, i64); });
            OP(f64_convert_i64_u, { CONVERT_OP(f64, s_nop, u64); });
            OP(f64_promote_f32, { CONVERT_OP(f64, s_nop, f32); });
            OP(i32_reinterpret_f32, { REINTERPRET_OP(i32, f32); });
            OP(i64_reinterpret_f64, { REINTERPRET_OP(i64, f64); });
            OP(f32_reinterpret_i32, { REINTERPRET_OP(f32, i32); });
            OP(f64_reinterpret_i64, { REINTERPRET_OP(f64, i64); });
            OP(i32_extend8_s, { CONVERT_OP(i32, s_as_i8, i32); });
            OP(i32_extend16_s, { CONVERT_OP(i32, s_as_i16, i32); });
            OP(i64_extend8_s, { CONVERT_OP(i64, s_as_i8, i32); });
            OP(i64_extend16_s, { CONVERT_OP(i64, s_as_i16, i32); });
            OP(i64_extend32_s, { CONVERT_OP(i64, s_as_i32, i64); });

            OP(ref_null, {
                stream_seek_unchecked(pc, 1);
                push((value_u){.u_ref = nullref});
            });
            OP(ref_is_null, {
                (sp - 1)->u_i32 = ((sp - 1)->u_ref == nullref);
            });
            OP(ref_func, {
                u32 f_idx;
                stream_read_vu32_unchecked(f_idx, pc);
                ref fref = to_ref(*vec_at_func_addr(&mod_inst->f_addrs, f_idx));
                push((value_u){.u_ref = fref});
            });

            // fc and fd prefixes.
            OP(prefix_fc, {
                u32 opcode_fc;
                stream_read_vu32_unchecked(opcode_fc, pc);
                switch (opcode_fc) {
                    case op_i32_trunc_sat_f32_s: {
                        if (s_isnan32((sp - 1)->u_f32)) {
                            (sp - 1)->u_i32 = 0;
                        } else if ((sp - 1)->u_f32 < -2147483648.f) {
                            (sp - 1)->u_i32 = i32_MIN;
                        } else if ((sp - 1)->u_f32 >= 2147483648.f) {
                            (sp - 1)->u_i32 = i32_MAX;
                        } else {
                            CONVERT_OP(i32, s_truncf32i, f32);
                        }
                        continue;
                    }
                    case op_i32_trunc_sat_f32_u: {
                        if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f) {
                            (sp - 1)->u_u32 = 0;
                        } else if ((sp - 1)->u_f32 >= 4294967296.f) {
                            (sp - 1)->u_u32 = u32_MAX;
                        } else {
                            CONVERT_OP(i32, s_truncf32u, f32);
                        }
                        continue;
                    }
                    case op_i32_trunc_sat_f64_s: {
                        if (s_isnan64((sp - 1)->u_f64)) {
                            (sp - 1)->u_i32 = 0;
                        } else if ((sp - 1)->u_f64 <= -2147483649.) {
                            (sp - 1)->u_i32 = i32_MIN;
                        } else if ((sp - 1)->u_f64 >= 2147483648.) {
                            (sp - 1)->u_i32 = i32_MAX;
                        } else {
                            CONVERT_OP(i32, s_truncf64i, f64);
                        }
                        continue;
                    }
                    case op_i32_trunc_sat_f64_u: {
                        if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1.) {
                            (sp - 1)->u_u32 = 0;
                        } else if ((sp - 1)->u_f64 >= 4294967296.) {
                            (sp - 1)->u_u32 = u32_MAX;
                        } else {
                            CONVERT_OP(i32, s_truncf64u, f64);
                        }
                        continue;
                    }
                    case op_i64_trunc_sat_f32_s: {
                        if (s_isnan32((sp - 1)->u_f32)) {
                            (sp - 1)->u_i64 = 0;
                        } else if ((sp - 1)->u_f32 < -9223372036854775808.f) {
                            (sp - 1)->u_i64 = i64_MIN;
                        } else if ((sp - 1)->u_f32 >= 9223372036854775808.f) {
                            (sp - 1)->u_i64 = i64_MAX;
                        } else {
                            CONVERT_OP(i64, s_truncf32i, f32);
                        }
                        continue;
                    }
                    case op_i64_trunc_sat_f32_u: {
                        if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f) {
                            (sp - 1)->u_u64 = 0;
                        } else if ((sp - 1)->u_f32 >= 18446744073709551616.f) {
                            (sp - 1)->u_u64 = u64_MAX;
                        } else {
                            CONVERT_OP(i64, s_truncf32u, f32);
                        }
                        continue;
                    }
                    case op_i64_trunc_sat_f64_s: {
                        if (s_isnan64((sp - 1)->u_f64)) {
                            (sp - 1)->u_i64 = 0;
                        } else if ((sp - 1)->u_f64 < -9223372036854775808.) {
                            (sp - 1)->u_i64 = i64_MIN;
                        } else if ((sp - 1)->u_f64 >= 9223372036854775808.) {
                            (sp - 1)->u_i64 = i64_MAX;
                        } else {
                            CONVERT_OP(i64, s_truncf64i, f64);
                        }
                        continue;
                    }
                    case op_i64_trunc_sat_f64_u: {
                        if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1.) {
                            (sp - 1)->u_u64 = 0;
                        } else if ((sp - 1)->u_f64 >= 18446744073709551616.) {
                            (sp - 1)->u_u64 = u64_MAX;
                        } else {
                            CONVERT_OP(i64, s_truncf64u, f64);
                        }
                        continue;
                    }
                    case op_memory_init: {
                        u32 data_idx;
                        stream_read_vu32_unchecked(data_idx, pc);
                        stream_seek_unchecked(pc, 1);
                        module * mod = mod_inst->mod;
                        assert(mod);
                        assert(data_idx < vec_size_data(&mod->data));
                        data * d = vec_at_data(&mod->data, data_idx);
                        u64 size = pop().u_u32;
                        u64 src = pop().u_u32;
                        u64 dst = pop().u_u32;
                        if (unlikely(((src + size) > str_len(d->bytes)) || ((dst + size) > vec_size_u8(pmem0)))) {
                            return err(e_general, "Invalid data access");
                        }
                        if (size) {
                            if (unlikely(*vec_at_u8(&mod_inst->dropped_data, data_idx))) {
                                return err(e_general, "Data has been dropped");
                            }
                            memcpy(vec_at_u8(pmem0, dst), d->bytes.ptr + src, size);
                        };
                        continue;
                    }
                    case op_data_drop: {
                        u32 data_idx;
                        stream_read_vu32_unchecked(data_idx, pc);
                        assert(vec_size_u8(&mod_inst->dropped_data) == vec_size_data(&mod_inst->mod->data));
                        *vec_at_u8(&mod_inst->dropped_data, data_idx) = true;
                        continue;
                    }
                    case op_memory_copy: {
                        stream_seek_unchecked(pc, 2);
                        u64 size = pop().u_u32;
                        u64 src = pop().u_u32;
                        u64 dst = pop().u_u32;
                        if (unlikely(((src + size) > vec_size_u8(pmem0)) || ((dst + size) > vec_size_u8(pmem0)))) {
                            return err(e_general, "Invalid memory access");
                        }
                        if (size) {
                            memmove(vec_at_u8(pmem0, dst), vec_at_u8(pmem0, src), size * sizeof(u8));
                        }
                        continue;
                    }
                    case op_memory_fill: {
                        stream_seek_unchecked(pc, 1);
                        u64 size = pop().u_u32;
                        u64 val = pop().u_i32;
                        u64 dst = pop().u_u32;
                        if (unlikely((dst + size) > vec_size_u8(pmem0))) {
                            return err(e_general, "Invalid memory access");
                        }
                        if (size) {
                            memset(vec_at_u8(pmem0, dst), (int)val, size * sizeof(u8));
                        }
                        continue;
                    }
                    case op_table_init: {
                        u32 elem_idx;
                        stream_read_vu32_unchecked(elem_idx, pc);
                        u32 table_idx;
                        stream_read_vu32_unchecked(table_idx, pc);
                        tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                        module * mod = mod_inst->mod;
                        assert(mod);
                        element * elem = vec_at_element(&mod->elements, elem_idx);
                        u64 size = pop().u_u32;
                        u64 src = pop().u_u32;
                        u64 dst = pop().u_u32;
                        if (unlikely(((src + size) > elem->data_len) || ((dst + size) > vec_size_ref(&t_addr->tdata)))) {
                            return err(e_general, "Invalid table access");
                        }
                        if (size) {
                            if (unlikely(*vec_at_u8(&mod_inst->dropped_elements, elem_idx))) {
                                return err(e_general, "Element has been dropped");
                            }
                            // TODO, the code is duplicated from vm.c
                            if (vec_size_u32(&elem->v_funcidx)) {
                                for (u32 j = 0; j < size; j++) {
                                    u32 f_idx = *vec_at_u32(&elem->v_funcidx, src + j);
                                    ref fref;
                                    if (f_idx != nullfidx) {
                                        // here we convert the index into the actual func_addr, if not null.
                                        fref = to_ref(*vec_at_func_addr(&mod_inst->f_addrs, f_idx));
                                    } else {
                                        fref = nullref;
                                    }
                                    *vec_at_ref(&t_addr->tdata, dst + j) = fref;
                                }
                                // calculate and convert the function indexes in the v_expr to references
                            } else if (vec_size_str(&elem->v_expr)) {
                                for (u32 j = 0; j < size; j++) {
                                    str expr = *vec_at_str(&elem->v_expr, src + j);
                                    unwrap(typed_value, val, interp_reduce_const_expr(mod_inst, stream_from(expr), true));
                                    if (unlikely(!is_ref(val.type))) {
                                        return err(e_invalid, "Incorrect element constexpr return type");
                                    }
                                    *vec_at_ref(&t_addr->tdata, dst + j) = val.val.u_ref;
                                }
                            } else if (elem->data_len) {
                                // both v_funcidx and v_expr are empty. should not happen.
                                assert(0);
                            }
                        }
                        continue;
                    }
                    case op_elem_drop: {
                        u32 elem_idx;
                        stream_read_vu32_unchecked(elem_idx, pc);
                        assert(vec_size_u8(&mod_inst->dropped_elements) == vec_size_element(&mod_inst->mod->elements));
                        *vec_at_u8(&mod_inst->dropped_elements, elem_idx) = true;
                        continue;
                    }
                    case op_table_copy: {
                        u32 table_idx_dst;
                        stream_read_vu32_unchecked(table_idx_dst, pc);
                        u32 table_idx_src;
                        stream_read_vu32_unchecked(table_idx_src, pc);
                        tab_addr t_addr_dst = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx_dst);
                        tab_addr t_addr_src = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx_src);
                        u64 size = pop().u_u32;
                        u64 src = pop().u_u32;
                        u64 dst = pop().u_u32;
                        if (unlikely(((src + size) > vec_size_ref(&t_addr_src->tdata)) || ((dst + size) > vec_size_ref(&t_addr_dst->tdata)))) {
                            return err(e_general, "Invalid table access");
                        }
                        if (size) {
                            memmove(vec_at_ref(&t_addr_dst->tdata, dst), vec_at_ref(&t_addr_src->tdata, src), size * sizeof(ref));
                        }
                        continue;
                    }
                    case op_table_grow: {
                        u32 table_idx;
                        stream_read_vu32_unchecked(table_idx, pc);
                        u64 size = pop().u_u32;
                        ref type = pop().u_ref;
                        tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                        size_t curr_size = vec_size_ref(&t_addr->tdata);
                        if ((curr_size + size > t_addr->tab->lim.max) || !is_ok(vec_resize_ref(&t_addr->tdata, curr_size + size))) {
                            push((value_u){.u_i32 = -1});
                            continue;
                        }
                        check(vec_resize_ref(&t_addr->tdata, curr_size + size));
                        for (size_t i = 0; i < size; i++) {
                            *vec_at_ref(&t_addr->tdata, curr_size + i) = type;
                        }
                        push((value_u){.u_i32 = (i32)curr_size});
                        continue;
                    }
                    case op_table_size: {
                        u32 table_idx;
                        stream_read_vu32_unchecked(table_idx, pc);
                        tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                        size_t curr_size = vec_size_ref(&t_addr->tdata);
                        push((value_u){.u_i32 = (i32)curr_size});
                        continue;
                    }
                    case op_table_fill: {
                        u32 table_idx;
                        stream_read_vu32_unchecked(table_idx, pc);
                        tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
                        u64 size = pop().u_u32;
                        ref type = pop().u_ref;
                        u64 offset = pop().u_i32;
                        if (unlikely(offset + size > vec_size_ref(&t_addr->tdata))) {
                            return err(e_invalid, "Invalid table access");
                        }
                        for (size_t i = 0; i < size; i++) {
                            *vec_at_ref(&t_addr->tdata, offset + i) = type;
                        }
                        continue;
                    }
                    default: {
                        return err(e_malformed, "Invalid opcode");
                    }
                }
                continue;
            });
            OP(prefix_fd, {
                u32 opcode_fd;
                stream_read_vu32_unchecked(opcode_fd, pc);
                switch (opcode_fd) {
                    case op_v128_load: {
                        continue;
                    }
                    default: {
                        return err(e_malformed, "Unsupported opcode");
                    }
                }
                continue;
            });
#if !defined(HAS_COMPUTED_GOTO)
            default: {
                return err(e_malformed, "Invalid opcode");
            }
        }
#endif
    }

end:;
    u32 arity = fn->fn_type.result_count;
    assert(sp - stack_base >= arity);
    memmove(local, sp - arity, sizeof(value_u) * arity);
    t->frame_depth--;
    t->stack_size -= fn->stack_size_max;
    return ok_r;
}

#endif // SILVERFIR_INTERP_INPLACE_DT

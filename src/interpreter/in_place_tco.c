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

// It's actually a hybrid interpreter with both DT and TCO. It uses the
// direct threading table and doesn't compile, so it's still an in-place
// interpreter, yet it also leverages TCO for further performance improvements.

#include "alloc.h"
#include "compiler.h"
#include "interpreter.h"
#include "mem_util.h"
#include "opcode.h"
#include "silverfir.h"
#include "smath.h"
#include "stream.h"
#include "logger.h"
#include "vec.h"

// This is very experimental as the musttail attribute is not yet stabilized and missing on many platforms.
// The musttail attr also has negative impact on some simple functions and making the compiler emit bad code.

#if SILVERFIR_INTERP_INPLACE_TCO

typedef struct call_ctx {
    thread * t;
    func_addr f_addr;
    func * fn;
    str code;
    module_inst * mod_inst;
    module * mod;
    value_u * stack_base;
    mem_addr mem_inst0;
    u8 * mem0;
    size_t mem0_size;
    // better put them in the reg
    jump_table * jt;
    u16 next_jt_idx;
} call_ctx;

#define TCO_CALL_CONVENTION

#define OP_HANDLER_ARGS const u8 *pc, void *handler_base, value_u *sp, value_u *local, call_ctx *ctx
#define OP(name) NOINLINE TCO_CALL_CONVENTION err_msg_t h_##name(OP_HANDLER_ARGS)
typedef err_msg_t(TCO_CALL_CONVENTION * op_handler)(OP_HANDLER_ARGS);

#define READ_NEXT_OP() op_handler handler = ((op_handler *)handler_base)[stream_read_u8_unchecked(pc)]
#define READ_NEXT_OP_NODECL() handler = ((op_handler *)handler_base)[stream_read_u8_unchecked(pc)]
#if defined(SILVERFIR_ENABLE_TRACER)
#define NEXT_OP()                                                                            \
    trace_next_instr(ctx->f_addr, pc - 1, local, ctx->fn->local_count, sp, ctx->stack_base); \
    return (handler(pc, handler_base, sp, local, ctx))
#define NEXT_OP_TAIL()                                                                       \
    trace_next_instr(ctx->f_addr, pc - 1, local, ctx->fn->local_count, sp, ctx->stack_base); \
    MUSTTAIL return (handler(pc, handler_base, sp, local, ctx))
#else
#define NEXT_OP() return (handler(pc, handler_base, sp, local, ctx))
#define NEXT_OP_TAIL() MUSTTAIL return (handler(pc, handler_base, sp, local, ctx))
#endif

#define CHECK_STACK() (assert(sp >= ctx->stack_base && sp <= ctx->stack_base + ctx->fn->stack_size_max))
#define pop() (CHECK_STACK(), *--sp)
#define pop_drop() (CHECK_STACK(), --sp)
#define push(val) (CHECK_STACK(), (*sp++) = val)

#define MEM_LOAD_OP(name, dst_type, src_type)                                     \
    OP(name) {                                                                    \
        stream_seek_unchecked(pc, 1);                                             \
        u32 offset;                                                               \
        stream_read_vu32_unchecked(offset, pc);                                   \
        READ_NEXT_OP();                                                           \
        u64 mem_idx = (u64)(u32)(pop().u_i32) + offset;                           \
        if (unlikely(mem_idx + sizeof(src_type) > ctx->mem0_size)) {              \
            return "t.load: out-of-bound memory access";                          \
        }                                                                         \
        value_u val = {.u_##dst_type = mem_read_##src_type(ctx->mem0 + mem_idx)}; \
        push(val);                                                                \
        NEXT_OP();                                                                \
    }

#define MEM_STORE_OP(name, dst_type, src_type)                                     \
    OP(name) {                                                                     \
        stream_seek_unchecked(pc, 1);                                              \
        u32 offset;                                                                \
        stream_read_vu32_unchecked(offset, pc);                                    \
        READ_NEXT_OP();                                                            \
        value_u val = pop();                                                       \
        u64 mem_idx = (u64)(u32)(pop().u_i32) + offset;                            \
        if (unlikely(mem_idx + sizeof(dst_type) > ctx->mem0_size)) {               \
            return "t.store: out-of-bound memory access";                          \
        }                                                                          \
        mem_write_##dst_type(ctx->mem0 + mem_idx, ((dst_type)(val.u_##src_type))); \
        NEXT_OP();                                                                 \
    }

#define UNOP(name, type, op)                                 \
    OP(name) {                                               \
        READ_NEXT_OP();                                      \
        (sp - 1)->u_##type = (type)(op((sp - 1)->u_##type)); \
        NEXT_OP();                                           \
    }

#define BINOP(name, tgt_type, op, op_type)                              \
    OP(name) {                                                          \
        READ_NEXT_OP();                                                 \
        value_u v2 = pop();                                             \
        tgt_type v = (tgt_type)(op(pop().u_##op_type, v2.u_##op_type)); \
        push((value_u){.u_##tgt_type = v});                             \
        NEXT_OP();                                                      \
    }

#define RELOP(name, op_type, op) BINOP(name, i32, op, op_type)

#define CONVERT(tgt_type, op, src_type) ((sp - 1)->u_##tgt_type = (tgt_type)(op((sp - 1)->u_##src_type)))
#define CONVERT_OP(name, tgt_type, op, src_type) \
    OP(name) {                                   \
        READ_NEXT_OP();                          \
        CONVERT(tgt_type, op, src_type);         \
        NEXT_OP();                               \
    }
#define REINTERPRET(tgt_type, src_type) ((sp - 1)->u_##tgt_type = *((tgt_type *)(&((sp - 1)->u_##src_type))))
#define REINTERPRET_OP(name, tgt_type, src_type)      \
    OP(name) {                                        \
        READ_NEXT_OP();                               \
        assert(sizeof(tgt_type) == sizeof(src_type)); \
        REINTERPRET(tgt_type, src_type);              \
        NEXT_OP();                                    \
    }

////////////////////////////////////////////////////////////////////////////////

OP(unreachable) {
    return "unreachable: unreachable";
}

OP(nop) {
    READ_NEXT_OP();
    NEXT_OP();
}

OP(block) {
    stream_seek_unchecked(pc, 1); //block type
    READ_NEXT_OP();
    NEXT_OP();
}

OP(loop) {
    stream_seek_unchecked(pc, 1); //block type
    READ_NEXT_OP();
    NEXT_OP();
}

OP(if) {
    i32 c = pop().u_i32;
    op_handler handler;
    if (c) {
        stream_seek_unchecked(pc, 1); //block type
        READ_NEXT_OP_NODECL();
        ctx->next_jt_idx++;
    } else {
        jump_table * tbl = ctx->jt + ctx->next_jt_idx;
        assert(ctx->code.ptr + tbl->pc == pc);
        ctx->next_jt_idx = tbl->next_idx;
        pc += tbl->target_offset;
        READ_NEXT_OP_NODECL();
    }
    NEXT_OP();
}

OP(else) {
    jump_table * tbl = ctx->jt + ctx->next_jt_idx;
    assert(ctx->code.ptr + tbl->pc == pc);
    ctx->next_jt_idx = tbl->next_idx;
    pc += tbl->target_offset;
    READ_NEXT_OP();
    NEXT_OP();
}

OP(end) {
    if (unlikely(pc == ctx->code.ptr + ctx->code.len)) {
        u32 arity = ctx->fn->fn_type.result_count;
        assert(sp - ctx->stack_base >= arity);
        // local and sp will not overlap because they belong to two different call frames
        memcpy(local, sp - arity, sizeof(value_u) * arity);
        return NULL;
    }
    READ_NEXT_OP();
    NEXT_OP();
}

OP(br) {
    jump_table * tbl = ctx->jt + ctx->next_jt_idx;
    assert(ctx->code.ptr + tbl->pc == pc);
    ctx->next_jt_idx = tbl->next_idx;
    pc += tbl->target_offset;
    READ_NEXT_OP();
    if (unlikely(tbl->stack_offset)) {
        u32 stack_offset = tbl->stack_offset;
        u16 arity = tbl->arity;
        assert(stack_offset < 32767);
        assert(sp - ctx->stack_base >= stack_offset + arity);
        memmove(sp - stack_offset - arity, sp - arity, sizeof(value_u) * arity);
        sp -= stack_offset;
    }
    NEXT_OP();
}

OP(br_if) {
    i32 c = pop().u_i32;
    op_handler handler;
    if (c) {
        // copy of OP(br), except for the read next op
        jump_table * tbl = ctx->jt + ctx->next_jt_idx;
        assert(ctx->code.ptr + tbl->pc == pc);
        ctx->next_jt_idx = tbl->next_idx;
        pc += tbl->target_offset;
        READ_NEXT_OP_NODECL();
        if (unlikely(tbl->stack_offset)) {
            u32 stack_offset = tbl->stack_offset;
            u16 arity = tbl->arity;
            assert(stack_offset < 32767);
            assert(sp - ctx->stack_base >= stack_offset + arity);
            value_u * dst = sp - stack_offset - arity;
            value_u * src = sp - arity;
            memmove(dst, src, sizeof(value_u) * arity);
            sp -= stack_offset;
        }
    } else {
        stream_seek_unchecked(pc, 1); //lth
        READ_NEXT_OP_NODECL();
        ctx->next_jt_idx++;
    }
    NEXT_OP();
}

OP(br_table) {
    u32 i = pop().u_u32;
    u32 table_len;
    stream_read_vu32_unchecked(table_len, pc);
    stream_seek_unchecked(pc, table_len + 1);
    if (i > table_len) {
        i = table_len;
    }
    ctx->next_jt_idx += (u16)i;
    // copy of OP(br)
    jump_table * tbl = ctx->jt + ctx->next_jt_idx;
    assert(ctx->code.ptr + tbl->pc == pc);
    ctx->next_jt_idx = tbl->next_idx;
    pc += tbl->target_offset;
    READ_NEXT_OP();
    if (unlikely(tbl->stack_offset)) {
        u32 stack_offset = tbl->stack_offset;
        u16 arity = tbl->arity;
        assert(stack_offset < 32767);
        assert(sp - ctx->stack_base >= stack_offset + arity);
        value_u * dst = sp - stack_offset - arity;
        value_u * src = sp - arity;
        memmove(dst, src, sizeof(value_u) * arity);
        sp -= stack_offset;
    }
    NEXT_OP();
}

OP(return ) {
    u32 arity = ctx->fn->fn_type.result_count;
    assert(sp - ctx->stack_base >= arity);
    memmove(local, sp - arity, sizeof(value_u) * arity);
    return NULL;
}

OP(call) {
    u32 fn_idx_local;
    stream_read_vu32_unchecked(fn_idx_local, pc);
    READ_NEXT_OP();
    assert(fn_idx_local < vec_size_func(&ctx->mod->funcs));
    func_addr callee_addr = *vec_at_func_addr(&ctx->mod_inst->f_addrs, fn_idx_local);
    func * callee_fn = callee_addr->fn;
    func_type callee_type = callee_fn->fn_type;
    assert(sp - ctx->stack_base >= callee_type.param_count);
    sp -= callee_type.param_count;
    assert(sp + callee_fn->local_count <= ctx->stack_base + ctx->fn->stack_size_max);
    r ret;
    if (unlikely(callee_fn->tr)) {
        // native call, we're passing in the caller's context.
        ret = (callee_fn->tr((tr_ctx){
                                 .f_addr = callee_addr,
                                 .args = sp,
                                 .mem0 = ctx->mem_inst0,
                             },
                             callee_fn->host_func));

    } else {
        ret = (in_place_tco_call(ctx->t, callee_addr, sp));

        // the callee might have called mem.grow.
        if (ctx->mem_inst0) {
            ctx->mem0 = ctx->mem_inst0->mdata._data;
            ctx->mem0_size = vec_size_u8(&ctx->mem_inst0->mdata);
        }
    }
    if (!is_ok(ret)) {
        return ret.msg;
    }
    sp += callee_type.result_count;
    NEXT_OP_TAIL();
}

OP(call_indirect) {
    u32 type_idx;
    stream_read_vu32_unchecked(type_idx, pc);
    func_type src_type = *vec_at_func_type(&ctx->mod->func_types, type_idx);
    // target
    u32 table_idx;
    stream_read_vu32_unchecked(table_idx, pc);
    READ_NEXT_OP();
    assert(table_idx < vec_size_tab_addr(&ctx->mod_inst->t_addrs));
    tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
    size_t i = pop().u_i32;
    if (i >= vec_size_ref(&t_addr->tdata)) {
        return "call_indirect: invalid table element index";
    }
    ref fref = *vec_at_ref(&t_addr->tdata, i);
    if (fref == nullref) {
        return "call_indirect: element is ref.null";
    }
    func_addr callee_addr = to_func_addr(fref);
    func * callee_fn = callee_addr->fn;
    func_type callee_type = callee_fn->fn_type;
    if (!func_type_eq(callee_type, src_type)) {
        return "call_indirect: function type mismatch";
    }
    assert(sp - ctx->stack_base >= callee_type.param_count);
    sp -= callee_type.param_count;
    r ret;
    if (unlikely(callee_fn->tr)) {
        // native call, we're passing in the caller's context.
        ret = (callee_fn->tr((tr_ctx){
                                 .f_addr = callee_addr,
                                 .args = sp,
                                 .mem0 = ctx->mem_inst0,
                             },
                             callee_fn->host_func));
    } else {
        ret = (in_place_tco_call(ctx->t, callee_addr, sp));
    }
    if (!is_ok(ret)) {
        return ret.msg;
    }
    sp += callee_type.result_count;
    NEXT_OP_TAIL();
}

OP(drop) {
    READ_NEXT_OP();
    pop_drop();
    NEXT_OP();
}

OP(select) {
    READ_NEXT_OP();
    i32 cond = pop().u_i32;
    value_u false_val = pop();
    value_u true_val = pop();
    if (cond) {
        push(true_val);
    } else {
        push(false_val);
    }
    NEXT_OP();
}

OP(select_t) {
    stream_seek_unchecked(pc, 2);
    // copied from OP(select)
    READ_NEXT_OP();
    i32 cond = pop().u_i32;
    value_u false_val = pop();
    value_u true_val = pop();
    if (cond) {
        push(true_val);
    } else {
        push(false_val);
    }
    NEXT_OP();
}

OP(local_get) {
    u32 local_idx;
    stream_read_vu32_unchecked(local_idx, pc);
    value_u v = local[local_idx];
    READ_NEXT_OP();
    push(v);
    NEXT_OP();
}

OP(local_set) {
    u32 local_idx;
    value_u v = pop();
    stream_read_vu32_unchecked(local_idx, pc);
    READ_NEXT_OP();
    local[local_idx] = v;
    NEXT_OP();
}

OP(local_tee) {
    // why not just local[local_idx] = *(sp -1); ? Because we must separate dependent load/store as much as we can.
    u32 local_idx;
    value_u v = *(sp - 1);
    stream_read_vu32_unchecked(local_idx, pc);
    READ_NEXT_OP();
    local[local_idx] = v;
    NEXT_OP();
}

OP(global_get) {
    u32 global_idx;
    stream_read_vu32_unchecked(global_idx, pc);
    READ_NEXT_OP();
    assert(global_idx < vec_size_global_inst(&ctx->mod_inst->globals));
    glob_addr g_addr = *vec_at_glob_addr(&ctx->mod_inst->g_addrs, global_idx);
    push(g_addr->gvalue);
    NEXT_OP();
}

OP(global_set) {
    u32 global_idx;
    stream_read_vu32_unchecked(global_idx, pc);
    READ_NEXT_OP();
    assert(global_idx < vec_size_global_inst(&ctx->mod_inst->globals));
    glob_addr g_addr = *vec_at_glob_addr(&ctx->mod_inst->g_addrs, global_idx);
    g_addr->gvalue = pop();
    NEXT_OP();
}

OP(table_get) {
    u32 table_idx;
    stream_read_vu32_unchecked(table_idx, pc);
    READ_NEXT_OP();
    assert(table_idx < vec_size_tab_addr(&ctx->mod_inst->t_addrs));
    tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
    u32 elem_idx = pop().u_i32;
    if (unlikely(elem_idx >= vec_size_ref(&t_addr->tdata))) {
        return "table_get: invalid table element index";
    }
    value_u ref = {.u_ref = *vec_at_ref(&t_addr->tdata, elem_idx)};
    push(ref);
    NEXT_OP();
}

OP(table_set) {
    u32 table_idx;
    stream_read_vu32_unchecked(table_idx, pc);
    READ_NEXT_OP();
    assert(table_idx < vec_size_tab_addr(&ctx->mod_inst->t_addrs));
    tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
    ref ref = pop().u_ref;
    u32 elem_idx = pop().u_u32;
    if (unlikely(elem_idx >= vec_size_ref(&t_addr->tdata))) {
        return "table_get: invalid table element index";
    }
    *vec_at_ref(&t_addr->tdata, elem_idx) = ref;
    NEXT_OP();
}

MEM_LOAD_OP(i32_load, i32, i32)
MEM_LOAD_OP(i64_load, i64, i64)
MEM_LOAD_OP(f32_load, f32, f32)
MEM_LOAD_OP(f64_load, f64, f64)
MEM_LOAD_OP(i32_load8_s, i32, i8)
MEM_LOAD_OP(i32_load8_u, i32, u8)
MEM_LOAD_OP(i32_load16_s, i32, i16)
MEM_LOAD_OP(i32_load16_u, i32, u16)
MEM_LOAD_OP(i64_load8_s, i64, i8)
MEM_LOAD_OP(i64_load8_u, i64, u8)
MEM_LOAD_OP(i64_load16_s, i64, i16)
MEM_LOAD_OP(i64_load16_u, i64, u16)
MEM_LOAD_OP(i64_load32_s, i64, i32)
MEM_LOAD_OP(i64_load32_u, i64, u32)
MEM_STORE_OP(i32_store, u32, i32)
MEM_STORE_OP(i64_store, u64, i64)
MEM_STORE_OP(f32_store, f32, f32)
MEM_STORE_OP(f64_store, f64, f64)
MEM_STORE_OP(i32_store8, u8, i32)
MEM_STORE_OP(i32_store16, u16, i32)
MEM_STORE_OP(i64_store8, u8, i64)
MEM_STORE_OP(i64_store16, u16, i64)
MEM_STORE_OP(i64_store32, u32, i64)

OP(memory_size) {
    stream_seek_unchecked(pc, 1);
    READ_NEXT_OP();
    push((value_u){.u_i32 = (i32)(ctx->mem0_size / WASM_PAGE_SIZE)});
    NEXT_OP();
}

OP(memory_grow) {
    stream_seek_unchecked(pc, 1);
    READ_NEXT_OP();
    u32 pages = (u32)(ctx->mem0_size / WASM_PAGE_SIZE);
    size_t n_pages = (size_t)(pop().u_u32);
    if (pages + n_pages <= ctx->mem_inst0->mem->lim.max) {
        size_t new_size = (pages + n_pages) * WASM_PAGE_SIZE;
        r ret_resize = vec_resize_u8(&ctx->mem_inst0->mdata, new_size);
        if (is_ok(ret_resize)) {
            ctx->mem0_size = new_size;
            ctx->mem0 = vec_at_u8(&ctx->mem_inst0->mdata, 0);
        } else {
            pages = (u32)(-1);
        }
    } else {
        pages = (u32)(-1);
    }
    push((value_u){.u_i32 = (i32)(pages)});
    NEXT_OP();
}

OP(i32_const) {
    value_u v;
    stream_read_vi32_unchecked(v.u_i32, pc);
    READ_NEXT_OP();
    push(v);
    NEXT_OP();
}

OP(i64_const) {
    value_u v;
    stream_read_vi64_unchecked(v.u_i64, pc);
    READ_NEXT_OP();
    push(v);
    NEXT_OP();
}

OP(f32_const) {
    value_u v;
    stream_read_f32_unchecked(v.u_f32, pc);
    READ_NEXT_OP();
    push(v);
    NEXT_OP();
}

OP(f64_const) {
    value_u v;
    stream_read_f64_unchecked(v.u_f64, pc);
    READ_NEXT_OP();
    push(v);
    NEXT_OP();
}

UNOP(i32_eqz, i32, s_eqz)
RELOP(i32_eq, i32, s_eq)
RELOP(i32_ne, i32, s_ne)
RELOP(i32_lt_s, i32, s_lt)
RELOP(i32_lt_u, u32, s_lt)
RELOP(i32_gt_s, i32, s_gt)
RELOP(i32_gt_u, u32, s_gt)
RELOP(i32_le_s, i32, s_le)
RELOP(i32_le_u, u32, s_le)
RELOP(i32_ge_s, i32, s_ge)
RELOP(i32_ge_u, u32, s_ge)
UNOP(i64_eqz, i64, s_eqz)
RELOP(i64_eq, i64, s_eq)
RELOP(i64_ne, i64, s_ne)
RELOP(i64_lt_s, i64, s_lt)
RELOP(i64_lt_u, u64, s_lt)
RELOP(i64_gt_s, i64, s_gt)
RELOP(i64_gt_u, u64, s_gt)
RELOP(i64_le_s, i64, s_le)
RELOP(i64_le_u, u64, s_le)
RELOP(i64_ge_s, i64, s_ge)
RELOP(i64_ge_u, u64, s_ge)
RELOP(f32_eq, f32, s_eq)
RELOP(f32_ne, f32, s_ne)
RELOP(f32_lt, f32, s_lt)
RELOP(f32_gt, f32, s_gt)
RELOP(f32_le, f32, s_le)
RELOP(f32_ge, f32, s_ge)
RELOP(f64_eq, f64, s_eq)
RELOP(f64_ne, f64, s_ne)
RELOP(f64_lt, f64, s_lt)
RELOP(f64_gt, f64, s_gt)
RELOP(f64_le, f64, s_le)
RELOP(f64_ge, f64, s_ge)
UNOP(i32_clz, i32, s_clz32)
UNOP(i32_ctz, i32, s_ctz32)
UNOP(i32_popcnt, i32, s_popcnt32)
BINOP(i32_add, i32, s_add, i32)
BINOP(i32_sub, i32, s_sub, i32)
BINOP(i32_mul, i32, s_mul, i32)

OP(i32_div_s) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(((v1.u_i32 == i32_MIN) && (v2.u_i32 == -1)) || (v2.u_i32 == 0))) {
        return "div trap";
    }
    push((value_u){.u_i32 = s_div(v1.u_i32, v2.u_i32)});
    NEXT_OP();
}

OP(i32_div_u) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(v2.u_u32 == 0)) {
        return "div trap";
    }
    push((value_u){.u_u32 = s_div(v1.u_u32, v2.u_u32)});
    NEXT_OP();
}

OP(i32_rem_s) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if ((v1.u_i32 == i32_MIN) && (v2.u_i32 == -1)) {
        push((value_u){.u_i32 = 0});
    } else if (unlikely(v2.u_i32 == 0)) {
        return "trap";
    } else {
        push((value_u){.u_i32 = s_rem(v1.u_i32, v2.u_i32)});
    }
    NEXT_OP();
}

OP(i32_rem_u) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(v2.u_u32 == 0)) {
        return "rem trap";
    }
    push((value_u){.u_u32 = s_rem(v1.u_u32, v2.u_u32)});
    NEXT_OP();
}

BINOP(i32_and, i32, s_and, i32)
BINOP(i32_or, i32, s_or, i32)
BINOP(i32_xor, i32, s_xor, i32)
BINOP(i32_shl, i32, s_shl, i32)
BINOP(i32_shr_s, i32, s_shr, i32)
BINOP(i32_shr_u, i32, s_shr, u32)
BINOP(i32_rotl, i32, s_rotl32, u32)
BINOP(i32_rotr, i32, s_rotr32, u32)
UNOP(i64_clz, i64, s_clz64)
UNOP(i64_ctz, i64, s_ctz64)
UNOP(i64_popcnt, i64, s_popcnt64)
BINOP(i64_add, i64, s_add, i64)
BINOP(i64_sub, i64, s_sub, i64)
BINOP(i64_mul, i64, s_mul, i64)

OP(i64_div_s) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(((v1.u_i64 == i64_MIN) && (v2.u_i64 == -1)) || (v2.u_i64 == 0))) {
        return "div trap";
    }
    push((value_u){.u_i64 = s_div(v1.u_i64, v2.u_i64)});
    NEXT_OP();
}

OP(i64_div_u) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(v2.u_u64 == 0)) {
        return "div trap";
    }
    push((value_u){.u_u64 = s_div(v1.u_u64, v2.u_u64)});
    NEXT_OP();
}

OP(i64_rem_s) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if ((v1.u_i64 == i64_MIN) && (v2.u_i64 == -1)) {
        push((value_u){.u_i64 = 0});
    } else if (unlikely(v2.u_i64 == 0)) {
        return "trap";
    } else {
        push((value_u){.u_i64 = s_rem(v1.u_i64, v2.u_i64)});
    }
    NEXT_OP();
}

OP(i64_rem_u) {
    READ_NEXT_OP();
    value_u v2 = pop();
    value_u v1 = pop();
    if (unlikely(v2.u_u64 == 0)) {
        return "rem trap";
    }
    push((value_u){.u_u64 = s_rem(v1.u_u64, v2.u_u64)});
    NEXT_OP();
}

BINOP(i64_and, i64, s_and, i64)
BINOP(i64_or, i64, s_or, i64)
BINOP(i64_xor, i64, s_xor, i64)
BINOP(i64_shl, i64, s_shl, i64)
BINOP(i64_shr_s, i64, s_shr, i64)
BINOP(i64_shr_u, i64, s_shr, u64)
BINOP(i64_rotl, i64, s_rotl64, u64)
BINOP(i64_rotr, i64, s_rotr64, u64)

// TODO: we might need a math lib that matches wasm standard..
UNOP(f32_abs, f32, s_fabs32)
UNOP(f32_neg, f32, s_fneg32)
UNOP(f32_ceil, f32, s_ceil)
UNOP(f32_floor, f32, s_floor)
UNOP(f32_trunc, f32, s_trunc)
UNOP(f32_nearest, f32, s_rint)
UNOP(f32_sqrt, f32, s_sqrt)
BINOP(f32_add, f32, s_add, f32)
BINOP(f32_sub, f32, s_sub, f32)
BINOP(f32_mul, f32, s_mul, f32)
BINOP(f32_div, f32, s_div, f32)
BINOP(f32_min, f32, s_fmin32, f32)
BINOP(f32_max, f32, s_fmax32, f32)
BINOP(f32_copysign, f32, s_copysign32, f32)
UNOP(f64_abs, f64, s_fabs64)
UNOP(f64_neg, f64, s_fneg64)
UNOP(f64_ceil, f64, s_ceil)
UNOP(f64_floor, f64, s_floor)
UNOP(f64_trunc, f64, s_trunc)
UNOP(f64_nearest, f64, s_rint)
UNOP(f64_sqrt, f64, s_sqrt)
BINOP(f64_add, f64, s_add, f64)
BINOP(f64_sub, f64, s_sub, f64)
BINOP(f64_mul, f64, s_mul, f64)
BINOP(f64_div, f64, s_div, f64)
BINOP(f64_min, f64, s_fmin64, f64)
BINOP(f64_max, f64, s_fmax64, f64)
BINOP(f64_copysign, f64, s_copysign64, f64)
CONVERT_OP(i32_wrap_i64, i32, s_nop, i64)

OP(i32_trunc_f32_s) {
    READ_NEXT_OP();
    if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 < -2147483648.f || (sp - 1)->u_f32 >= 2147483648.f) {
        return "trap";
    }
    CONVERT(i32, s_truncf32i, f32);
    NEXT_OP();
}

OP(i32_trunc_f32_u) {
    READ_NEXT_OP();
    if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f || (sp - 1)->u_f32 >= 4294967296.f) {
        return "trap";
    }
    CONVERT(i32, s_truncf32u, f32);
    NEXT_OP();
}

OP(i32_trunc_f64_s) {
    READ_NEXT_OP();
    if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -2147483649. || (sp - 1)->u_f64 >= 2147483648.) {
        return "trap";
    }
    CONVERT(i32, s_truncf64i, f64);
    NEXT_OP();
}

OP(i32_trunc_f64_u) {
    READ_NEXT_OP();
    if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1. || (sp - 1)->u_f64 >= 4294967296.) {
        return "trap";
    }
    CONVERT(i32, s_truncf64u, f64);
    NEXT_OP();
}

CONVERT_OP(i64_extend_i32_s, i64, s_extend32, i32)
CONVERT_OP(i64_extend_i32_u, i64, s_extend32u, i32)

OP(i64_trunc_f32_s) {
    READ_NEXT_OP();
    if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 < -9223372036854775808.f || (sp - 1)->u_f32 >= 9223372036854775808.f) {
        return "trap";
    }
    CONVERT(i64, s_truncf32i, f32);
    NEXT_OP();
}

OP(i64_trunc_f32_u) {
    READ_NEXT_OP();
    if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f || (sp - 1)->u_f32 >= 18446744073709551616.f) {
        return "trap";
    }
    CONVERT(i64, s_truncf32u, f32);
    NEXT_OP();
}

OP(i64_trunc_f64_s) {
    READ_NEXT_OP();
    if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 < -9223372036854775808. || (sp - 1)->u_f64 >= 9223372036854775808.) {
        return "trap";
    }
    CONVERT(i64, s_truncf64i, f64);
    NEXT_OP();
}

OP(i64_trunc_f64_u) {
    READ_NEXT_OP();
    if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1. || (sp - 1)->u_f64 >= 18446744073709551616.) {
        return "trap";
    }
    CONVERT(i64, s_truncf64u, f64);
    NEXT_OP();
}

CONVERT_OP(f32_convert_i32_s, f32, s_nop, i32)
CONVERT_OP(f32_convert_i32_u, f32, s_nop, u32)
CONVERT_OP(f32_convert_i64_s, f32, s_nop, i64)
CONVERT_OP(f32_convert_i64_u, f32, s_nop, u64)
CONVERT_OP(f32_demote_f64, f32, s_nop, f64)
CONVERT_OP(f64_convert_i32_s, f64, s_nop, i32)
CONVERT_OP(f64_convert_i32_u, f64, s_nop, u32)
CONVERT_OP(f64_convert_i64_s, f64, s_nop, i64)
CONVERT_OP(f64_convert_i64_u, f64, s_nop, u64)
CONVERT_OP(f64_promote_f32, f64, s_nop, f32)
REINTERPRET_OP(i32_reinterpret_f32, i32, f32)
REINTERPRET_OP(i64_reinterpret_f64, i64, f64)
REINTERPRET_OP(f32_reinterpret_i32, f32, i32)
REINTERPRET_OP(f64_reinterpret_i64, f64, i64)
CONVERT_OP(i32_extend8_s, i32, s_extend8, i32)
CONVERT_OP(i32_extend16_s, i32, s_extend16, i32)
CONVERT_OP(i64_extend8_s, i64, s_extend8, i32)
CONVERT_OP(i64_extend16_s, i64, s_extend16, i32)
CONVERT_OP(i64_extend32_s, i64, s_extend32, i64)

OP(ref_null) {
    stream_seek_unchecked(pc, 1);
    READ_NEXT_OP();
    push((value_u){.u_ref = nullref});
    NEXT_OP();
}

OP(ref_is_null) {
    READ_NEXT_OP();
    (sp - 1)->u_i32 = ((sp - 1)->u_ref == nullref);
    NEXT_OP();
}

OP(ref_func) {
    u32 f_idx;
    stream_read_vu32_unchecked(f_idx, pc);
    READ_NEXT_OP();
    ref fref = to_ref(*vec_at_func_addr(&ctx->mod_inst->f_addrs, f_idx));
    push((value_u){.u_ref = fref});
    NEXT_OP();
}

// TODO
OP(prefix_fc) {
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
                CONVERT(i32, s_truncf32i, f32);
            }
            break;
        }
        case op_i32_trunc_sat_f32_u: {
            if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f) {
                (sp - 1)->u_u32 = 0;
            } else if ((sp - 1)->u_f32 >= 4294967296.f) {
                (sp - 1)->u_u32 = u32_MAX;
            } else {
                CONVERT(i32, s_truncf32u, f32);
            }
            break;
        }
        case op_i32_trunc_sat_f64_s: {
            if (s_isnan64((sp - 1)->u_f64)) {
                (sp - 1)->u_i32 = 0;
            } else if ((sp - 1)->u_f64 <= -2147483649.) {
                (sp - 1)->u_i32 = i32_MIN;
            } else if ((sp - 1)->u_f64 >= 2147483648.) {
                (sp - 1)->u_i32 = i32_MAX;
            } else {
                CONVERT(i32, s_truncf64i, f64);
            }
            break;
        }
        case op_i32_trunc_sat_f64_u: {
            if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1.) {
                (sp - 1)->u_u32 = 0;
            } else if ((sp - 1)->u_f64 >= 4294967296.) {
                (sp - 1)->u_u32 = u32_MAX;
            } else {
                CONVERT(i32, s_truncf64u, f64);
            }
            break;
        }
        case op_i64_trunc_sat_f32_s: {
            if (s_isnan32((sp - 1)->u_f32)) {
                (sp - 1)->u_i64 = 0;
            } else if ((sp - 1)->u_f32 < -9223372036854775808.f) {
                (sp - 1)->u_i64 = i64_MIN;
            } else if ((sp - 1)->u_f32 >= 9223372036854775808.f) {
                (sp - 1)->u_i64 = i64_MAX;
            } else {
                CONVERT(i64, s_truncf32i, f32);
            }
            break;
        }
        case op_i64_trunc_sat_f32_u: {
            if (s_isnan32((sp - 1)->u_f32) || (sp - 1)->u_f32 <= -1.f) {
                (sp - 1)->u_u64 = 0;
            } else if ((sp - 1)->u_f32 >= 18446744073709551616.f) {
                (sp - 1)->u_u64 = u64_MAX;
            } else {
                CONVERT(i64, s_truncf32u, f32);
            }
            break;
        }
        case op_i64_trunc_sat_f64_s: {
            if (s_isnan64((sp - 1)->u_f64)) {
                (sp - 1)->u_i64 = 0;
            } else if ((sp - 1)->u_f64 < -9223372036854775808.) {
                (sp - 1)->u_i64 = i64_MIN;
            } else if ((sp - 1)->u_f64 >= 9223372036854775808.) {
                (sp - 1)->u_i64 = i64_MAX;
            } else {
                CONVERT(i64, s_truncf64i, f64);
            }
            break;
        }
        case op_i64_trunc_sat_f64_u: {
            if (s_isnan64((sp - 1)->u_f64) || (sp - 1)->u_f64 <= -1.) {
                (sp - 1)->u_u64 = 0;
            } else if ((sp - 1)->u_f64 >= 18446744073709551616.) {
                (sp - 1)->u_u64 = u64_MAX;
            } else {
                CONVERT(i64, s_truncf64u, f64);
            }
            break;
        }
        case op_memory_init: {
            u32 data_idx;
            stream_read_vu32_unchecked(data_idx, pc);
            stream_seek_unchecked(pc, 1);
            module * mod = ctx->mod;
            assert(mod);
            assert(data_idx < vec_size_data(&mod->data));
            data * d = vec_at_data(&mod->data, data_idx);
            u64 size = pop().u_u32;
            u64 src = pop().u_u32;
            u64 dst = pop().u_u32;
            if (((src + size) > str_len(d->bytes)) || ((dst + size) > ctx->mem0_size)) {
                return "Invalid data access";
            }
            if (size) {
                if (*vec_at_u8(&ctx->mod_inst->dropped_data, data_idx)) {
                    return "Data has been dropped";
                }
                memcpy(ctx->mem0 + dst, d->bytes.ptr + src, size);
            };
            break;
        }
        case op_data_drop: {
            u32 data_idx;
            stream_read_vu32_unchecked(data_idx, pc);
            assert(vec_size_u8(&ctx->mod_inst->dropped_data) == vec_size_data(&ctx->mod->data));
            *vec_at_u8(&ctx->mod_inst->dropped_data, data_idx) = true;
            break;
        }
        case op_memory_copy: {
            stream_seek_unchecked(pc, 2);
            u64 size = pop().u_u32;
            u64 src = pop().u_u32;
            u64 dst = pop().u_u32;
            if (((src + size) > ctx->mem0_size) || ((dst + size) > ctx->mem0_size)) {
                return "Invalid memory access";
            }
            if (size) {
                memmove(ctx->mem0 + dst, ctx->mem0 + src, size * sizeof(u8));
            }
            break;
        }
        case op_memory_fill: {
            stream_seek_unchecked(pc, 1);
            u64 size = pop().u_u32;
            u64 val = pop().u_i32;
            u64 dst = pop().u_u32;
            if ((dst + size) > ctx->mem0_size) {
                return "Invalid memory access";
            }
            if (size) {
                memset(ctx->mem0 + dst, (int)val, size * sizeof(u8));
            }
            break;
        }
        case op_table_init: {
            u32 elem_idx;
            stream_read_vu32_unchecked(elem_idx, pc);
            u32 table_idx;
            stream_read_vu32_unchecked(table_idx, pc);
            tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
            module * mod = ctx->mod;
            assert(mod);
            element * elem = vec_at_element(&mod->elements, elem_idx);
            u64 size = pop().u_u32;
            u64 src = pop().u_u32;
            u64 dst = pop().u_u32;
            if (((src + size) > elem->data_len) || ((dst + size) > vec_size_ref(&t_addr->tdata))) {
                return "Invalid table access";
            }
            if (size) {
                if (*vec_at_u8(&ctx->mod_inst->dropped_elements, elem_idx)) {
                    return "Element has been dropped";
                }
                // TODO, the code is duplicated from vm.c
                if (vec_size_u32(&elem->v_funcidx)) {
                    for (u32 j = 0; j < size; j++) {
                        u32 f_idx = *vec_at_u32(&elem->v_funcidx, src + j);
                        ref fref;
                        if (f_idx != nullfidx) {
                            // here we convert the index into the actual func_addr, if not null.
                            fref = to_ref(*vec_at_func_addr(&ctx->mod_inst->f_addrs, f_idx));
                        } else {
                            fref = nullref;
                        }
                        *vec_at_ref(&t_addr->tdata, dst + j) = fref;
                    }
                    // calculate and convert the function indexes in the v_expr to references
                } else if (vec_size_str(&elem->v_expr)) {
                    for (u32 j = 0; j < size; j++) {
                        str expr = *vec_at_str(&elem->v_expr, src + j);
                        r_typed_value rval = interp_reduce_const_expr(ctx->mod_inst, stream_from(expr), true);
                        if (!is_ok(rval)) {
                            return rval.msg;
                        }
                        typed_value val = rval.value;
                        if (!is_ref(val.type)) {
                            return "Incorrect element constexpr return type";
                        }
                        *vec_at_ref(&t_addr->tdata, dst + j) = val.val.u_ref;
                    }
                } else if (elem->data_len) {
                    // both v_funcidx and v_expr are empty. should not happen.
                    assert(0);
                }
            }
            break;
        }
        case op_elem_drop: {
            u32 elem_idx;
            stream_read_vu32_unchecked(elem_idx, pc);
            assert(vec_size_u8(&ctx->mod_inst->dropped_elements) == vec_size_element(&ctx->mod->elements));
            *vec_at_u8(&ctx->mod_inst->dropped_elements, elem_idx) = true;
            break;
        }
        case op_table_copy: {
            u32 table_idx_dst;
            stream_read_vu32_unchecked(table_idx_dst, pc);
            u32 table_idx_src;
            stream_read_vu32_unchecked(table_idx_src, pc);
            tab_addr t_addr_dst = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx_dst);
            tab_addr t_addr_src = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx_src);
            u64 size = pop().u_u32;
            u64 src = pop().u_u32;
            u64 dst = pop().u_u32;
            if (((src + size) > vec_size_ref(&t_addr_src->tdata)) || ((dst + size) > vec_size_ref(&t_addr_dst->tdata))) {
                return "Invalid table access";
            }
            if (size) {
                memmove(vec_at_ref(&t_addr_dst->tdata, dst), vec_at_ref(&t_addr_src->tdata, src), size * sizeof(ref));
            }
            break;
        }
        case op_table_grow: {
            u32 table_idx;
            stream_read_vu32_unchecked(table_idx, pc);
            u64 size = pop().u_u32;
            ref type = pop().u_ref;
            tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
            size_t curr_size = vec_size_ref(&t_addr->tdata);
            if ((curr_size + size > t_addr->tab->lim.max) || !is_ok(vec_resize_ref(&t_addr->tdata, curr_size + size))) {
                push((value_u){.u_i32 = -1});
                break;
            }
            r ret_resize = vec_resize_ref(&t_addr->tdata, curr_size + size);
            if (!is_ok(ret_resize)) {
                return ret_resize.msg;
            }
            for (size_t i = 0; i < size; i++) {
                *vec_at_ref(&t_addr->tdata, curr_size + i) = type;
            }
            push((value_u){.u_i32 = (i32)curr_size});
            break;
        }
        case op_table_size: {
            u32 table_idx;
            stream_read_vu32_unchecked(table_idx, pc);
            tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
            size_t curr_size = vec_size_ref(&t_addr->tdata);
            push((value_u){.u_i32 = (i32)curr_size});
            break;
        }
        case op_table_fill: {
            u32 table_idx;
            stream_read_vu32_unchecked(table_idx, pc);
            tab_addr t_addr = *vec_at_tab_addr(&ctx->mod_inst->t_addrs, table_idx);
            u64 size = pop().u_u32;
            ref type = pop().u_ref;
            u64 offset = pop().u_i32;
            if (offset + size > vec_size_ref(&t_addr->tdata)) {
                return "Invalid table access";
            }
            for (size_t i = 0; i < size; i++) {
                *vec_at_ref(&t_addr->tdata, offset + i) = type;
            }
            break;
        }
        default: {
            return "Invalid opcode";
        }
    }
    READ_NEXT_OP();
    NEXT_OP();
}

OP(prefix_fd) {
    return "Unsupported opcode";
}

#define HANDLER_ADDR(name, _1, _2, _3) [op_##name] = h_##name,
static const op_handler handlers[256] = {FOR_EACH_WASM_OPCODE(HANDLER_ADDR)};

r in_place_tco_call(thread * t, func_addr f_addr, value_u * args) {
    assert(t);
    assert(f_addr);
    assert(args);
    check_prep(r);

    if (unlikely(t->frame_depth > SILVERFIR_STACK_FRAME_LIMIT)) {
        return err(e_exhaustion, "Stack Overflow");
    }

    if (unlikely(t->stack_size > SILVERFIR_STACK_SIZE_LIMIT)) {
        return err(e_exhaustion, "Stack reached size limit");
    }

    const u8 * pc;
    value_u * sp;
    value_u * local = args;

    call_ctx ctx = {0};
    ctx.t = t;
    ctx.f_addr = f_addr;
    ctx.fn = f_addr->fn;

    // zero-out the reset of the locals. This is *required* by the spec.
    memset(local + ctx.fn->fn_type.param_count, 0, (ctx.fn->local_count - ctx.fn->fn_type.param_count) * sizeof(value_u));

    ctx.code = f_addr->fn->code;
    pc = ctx.code.ptr;
    op_handler handler = handlers[stream_read_u8_unchecked(pc)];
    ctx.mod_inst = f_addr->mod_inst;
    ctx.mod = ctx.mod_inst->mod;
    ctx.stack_base = array_alloca(value_u, ctx.fn->stack_size_max);
    if (!ctx.stack_base) {
        return err(e_exhaustion, "OOM");
    }
    sp = ctx.stack_base;
    if (vec_size_mem_addr(&f_addr->mod_inst->m_addrs)) {
        ctx.mem_inst0 = *vec_at_mem_addr(&ctx.mod_inst->m_addrs, 0);
        ctx.mem0 = ctx.mem_inst0->mdata._data;
        ctx.mem0_size = vec_size_u8(&ctx.mem_inst0->mdata);
    }
    ctx.jt = ctx.fn->jt._data;
    ctx.next_jt_idx = 0;
    t->frame_depth++;
    t->stack_size += ctx.fn->stack_size_max;
    err_msg_t result = handler(pc, (void *)(&handlers[0]), sp, local, &ctx);
    t->frame_depth--;
    t->stack_size -= ctx.fn->stack_size_max;
    // the return value should be in the local, which is the args now.
    return (r){.msg = result};
}

#endif // SILVERFIR_INTERP_INPLACE_TCO

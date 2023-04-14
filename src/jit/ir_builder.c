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

#include "ir_builder.h"

#include "logger.h"
#include "op_decoder.h"
#include "parser.h"
#include "vec_impl.h"
#include "vm.h"
#include "wasm_format.h"

#define LOGI(fmt, ...) LOG_INFO(log_channel_ir_builder, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_ir_builder, fmt, ##__VA_ARGS__)

typedef struct local_lifespan_context {
    module * mod;
    func * f;
    vec_u32 local_lifespan_table;
} local_lifespan_context;

static r local_lifespan_on_decode_begin(void * payload) {
    check_prep(r);
    local_lifespan_context * ctx = (local_lifespan_context *)payload;
    u32 local_count = ctx->f->local_count;
    check(vec_resize_u32(&ctx->local_lifespan_table, local_count));
    return ok_r;
}

static r local_lifespan_on_local_accessed(void * payload, stream imm, u32 local_idx) {
    local_lifespan_context * ctx = (local_lifespan_context *)payload;
    u32 offset = (u32)(imm.p - imm.s.ptr);
    assert(local_idx < vec_size_u32(&ctx->local_lifespan_table));
    *vec_at_u32(&ctx->local_lifespan_table, local_idx) = offset;
    return ok_r;
}

static const op_decoder_callbacks local_lifespan_callbacks = {
    .on_decode_begin = local_lifespan_on_decode_begin,
    .on_local_get = local_lifespan_on_local_accessed,
    .on_local_set = local_lifespan_on_local_accessed,
    .on_local_tee = local_lifespan_on_local_accessed,
};

//////////////////////////////////////////////////////////////////////////////

VEC_IMPL_FOR_TYPE(local_slot)
VEC_IMPL_FOR_TYPE(stack_slot)
VEC_IMPL_FOR_TYPE(reg_state)

typedef struct ir_builder_context {
    module * mod;
    func * f;
    vec_u32 local_lifespan_table;
    vec_local_slot local_mappings;
    vec_stack_slot stack_mappings;
    vec_reg_state reg_mappings;
    u32 sp; // just an index
} ir_builder_context;

static bool is_float(type_id id) {
    return id == TYPE_ID_f32 || id == TYPE_ID_f64;
}

#define TYPEID_TO_REG_TYPE(typeid) (is_float(typeid) ? FPR : GPR)

static void ir_builder_context_drop(ir_builder_context * ctx) {
    vec_clear_local_slot(&ctx->local_mappings);
    vec_clear_stack_slot(&ctx->stack_mappings);
    vec_clear_reg_state(&ctx->reg_mappings);
}

static r ir_builder_on_decode_begin(void * payload) {
    // ir_builder_context * ctx = (ir_builder_context*)payload;
    LOGI("ir_builder begin");
    return ok_r;
}

static r ir_builder_on_opcode(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_unreachable(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_block(void * payload, wasm_opcode opcode, stream imm, func_type type) {
    return ok_r;
}

static r ir_builder_on_else(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_end(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_br_or_if(void * payload, wasm_opcode opcode, stream imm, u8 lth) {
    return ok_r;
}

static r ir_builder_on_br_table(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_return(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_call(void * payload, stream imm, u32 func_idx) {
    return ok_r;
}

static r ir_builder_on_call_indirect(void * payload, stream imm, u32 type_idx, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_drop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_select(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_select_t(void * payload, wasm_opcode opcode, stream imm, type_id type) {
    return ok_r;
}

// The register content at the returned index is invalid.
static u16 get_reg(ir_builder_context * ctx, reg_type type, stream imm) {
    u32 pc_offset = (u32)(imm.p - imm.s.ptr);
    for (size_t i = 0; i < vec_size_reg_state(&ctx->reg_mappings); i++) {
        reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, i);
        if (!reg->pinned && (reg->tgt_idx == INVALID_IDX_U16) && (reg->type == type)) {
            return (u16)i;
        }
    }
    // now we need to spill one register
    // let's go with the unused locals first.
    for (size_t i = 0; i < vec_size_reg_state(&ctx->reg_mappings); i++) {
        reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, i);
        if (!reg->pinned && (reg->type == type) && (reg->target == tgt_local)) {
            assert(reg->tgt_idx != INVALID_IDX_U16);
            local_slot * local_val = vec_at_local_slot(&ctx->local_mappings, reg->tgt_idx);
            assert(local_val->reg_idx == i);
            assert(local_val->type == type);
            // skip if it's still aliased
            if (local_val->alias_count) {
                continue;
            }
            // End of life offset
            u32 eol_offset = *vec_at_u32(&ctx->local_lifespan_table, reg->tgt_idx);
            if (pc_offset > eol_offset) {
                // found, spill it.
                LOGI("[emit] r%" PRIu32 " --> local[%" PRIu32 "]", (u32)i, (u32)(reg->tgt_idx));
                local_val->reg_idx = INVALID_IDX_U16;
                return (u16)i;
            }
        }
    }
    // Well, all mapped locals are in use, let's try to spill some stacks
    if (ctx->sp > REG_AS_STACK_MIN) {
        for (u32 i = 0; i < ctx->sp - REG_AS_STACK_MIN; i++) {
            stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, i);
            if (!stack_val->aliased && stack_val->u.reg_idx != INVALID_IDX_U16) {
                u16 reg_idx = stack_val->u.reg_idx;
                reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, reg_idx);
                if (!reg->pinned && (reg->type == type)) {
                    // spill this one
                    LOGI("[emit] r%" PRIu32 " --> stack[%" PRIu32 "]", (u32)(reg_idx), i);
                    stack_val->u.reg_idx = INVALID_IDX_U16;
                    return reg_idx;
                }
            }
        }
    }
    // All locals are in use and stacks are minimum. We must free a local to make space.
    // TODO: there needs an algorithm to determine the best one to spill.
    for (size_t i = 0; i < vec_size_reg_state(&ctx->reg_mappings); i++) {
        reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, i);
        if (!reg->pinned && (reg->type == type) && (reg->target == tgt_local)) {
            assert(reg->tgt_idx != INVALID_IDX_U16);
            local_slot * local_val = vec_at_local_slot(&ctx->local_mappings, reg->tgt_idx);
            // not aliased locals are better choice because we don't need go through all the stack value to find all the alias.
            if (local_val->alias_count == 0) {
                // spill this one.
                LOGI("[emit] r%" PRIu32 " --> local[%" PRIu32 "]", (u32)i, (u32)(reg->tgt_idx));
                local_val->reg_idx = INVALID_IDX_U16;
                return i;
            }
        }
    }
    // Finally, if all above steps have failed, we have to find an aliased local, and walk through the stack to clear all aliases.
    for (size_t i = 0; i < vec_size_reg_state(&ctx->reg_mappings); i++) {
        reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, i);
        if (!reg->pinned && (reg->type == type) && (reg->target == tgt_local)) {
            assert(reg->tgt_idx != INVALID_IDX_U16);
            local_slot * local_val = vec_at_local_slot(&ctx->local_mappings, reg->tgt_idx);
            assert(local_val->alias_count != 0);
            // now we walk through the stack.
            u32 found_count = 0;
            for (u32 i = 0; i < ctx->sp - REG_AS_STACK_MIN; i++) {
                stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, i);
                if (stack_val->aliased && stack_val->u.local_idx == reg->tgt_idx) {
                    found_count++;
                    stack_val->aliased = false;
                    stack_val->u.local_idx = INVALID_IDX_U16;
                }
            }
            assert(found_count == local_val->alias_count);
            LOGI("[emit] r%" PRIu32 " --> local[%" PRIu32 "]", (u32)i, (u32)(reg->tgt_idx));
            local_val->reg_idx = INVALID_IDX_U16;
            return (u16)i;
        }
    }
    // should not reach here.
    assert(false);
    return INVALID_IDX_U16;
}

void reg_set_pinned(ir_builder_context * ctx, u16 reg_idx, bool pinned) {
    vec_at_reg_state(&ctx->reg_mappings, reg_idx)->pinned = pinned;
}

static r ir_builder_on_local_get(void * payload, stream imm, u32 local_idx) {
    ir_builder_context * ctx = (ir_builder_context *)payload;
    assert(local_idx < vec_size_local_slot(&ctx->local_mappings));
    local_slot * local_val = vec_at_local_slot(&ctx->local_mappings, local_idx);
    if (local_val->reg_idx == INVALID_IDX_U16) {
        // needs to assign a register
        u16 reg_idx = get_reg(ctx, local_val->type, imm);
        assert(reg_idx < vec_size_reg_state(&ctx->reg_mappings));
        reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, reg_idx);
        reg->target = tgt_local;
        assert(reg->tgt_idx == INVALID_IDX_U16); // must be unassigned
        reg->tgt_idx = local_idx;
        local_val->reg_idx = reg_idx;
        LOGI("[emit] r%" PRIu32 " <-- local[%" PRIu32 "]", (u32)reg_idx, (u32)(local_idx));
    }
    // make an alias
    stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, ctx->sp++);
    stack_val->aliased = true;
    stack_val->u.local_idx = local_idx;
    local_val->alias_count++;
    return ok_r;
}

static r ir_builder_on_local_set_tee(ir_builder_context * ctx, stream imm, u32 local_idx, bool is_tee) {
    assert(ctx->sp);
    stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, (ctx->sp - 1));
    if (is_tee) {
        ctx->sp--;
    }
    if (stack_val->aliased) {
        u16 src_local_idx = stack_val->u.local_idx;
        assert(src_local_idx != INVALID_IDX_U16);
        local_slot * src_local_val = vec_at_local_slot(&ctx->local_mappings, src_local_idx);
        assert(src_local_val->alias_count);
        assert(src_local_val->reg_idx != INVALID_IDX_U16);
        if (!is_tee) {
            src_local_val->alias_count--;
        }
        // Allocate a register for the target local variable and copy to it.
        if (src_local_idx != local_idx) {
            local_slot * tgt_local_val = vec_at_local_slot(&ctx->local_mappings, local_idx);
            if (tgt_local_val->reg_idx == INVALID_IDX_U16) {
                reg_set_pinned(ctx, src_local_val->reg_idx, true);
                tgt_local_val->reg_idx = get_reg(ctx, src_local_val->type, imm);
                reg_set_pinned(ctx, src_local_val->reg_idx, false);
                tgt_local_val->type = src_local_val->type;
                tgt_local_val->alias_count = 0;
            }
            LOGI("[emit] r%" PRIu32 " <-- r%" PRIu32 "", (u32)(tgt_local_val->reg_idx), (u32)(src_local_val->reg_idx));
        }
    } else {
        local_slot * tgt_local_val = vec_at_local_slot(&ctx->local_mappings, local_idx);
        if (tgt_local_val->reg_idx == INVALID_IDX_U16) {
            // No register assigned yet, so we simply pass the register into this local var
            reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, tgt_local_val->reg_idx);
            tgt_local_val->reg_idx = stack_val->u.reg_idx;
            tgt_local_val->type = reg->type;
            tgt_local_val->alias_count = 0;
        } else {
            // Already assigned an register, so we discard it and use the one with the stack var
            reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, tgt_local_val->reg_idx);
            reg->tgt_idx = INVALID_IDX_U16;
            tgt_local_val->reg_idx = stack_val->u.reg_idx;
        }
        if (is_tee) {
            // make the stack variable an alias
            tgt_local_val->alias_count++;
            stack_val->aliased = true;
            stack_val->u.local_idx = local_idx;
        }
    }
    return ok_r;
}

static r ir_builder_on_local_set(void * payload, stream imm, u32 local_idx) {
    ir_builder_context * ctx = (ir_builder_context *)payload;
    return ir_builder_on_local_set_tee(ctx, imm, local_idx, false);
}

static r ir_builder_on_local_tee(void * payload, stream imm, u32 local_idx) {
    ir_builder_context * ctx = (ir_builder_context *)payload;
    return ir_builder_on_local_set_tee(ctx, imm, local_idx, true);
}

static r ir_builder_on_global_get(void * payload, stream imm, u32 global_idx) {
    ir_builder_context * ctx = (ir_builder_context *)payload;
    // Get the global type
    global * g = vec_at_global(&ctx->mod->globals, global_idx);
    reg_type type = TYPEID_TO_REG_TYPE(g->valtype);

    // Allocate a register
    u16 reg_idx = get_reg(ctx, type, imm);
    assert(reg_idx < vec_size_reg_state(&ctx->reg_mappings));
    reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, reg_idx);
    reg->target = tgt_stack;
    reg->type = type;
    reg->tgt_idx = ctx->sp;

    // register to the stack slot
    stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, ctx->sp);
    stack_val->aliased = false;
    stack_val->u.reg_idx = reg_idx;
    ctx->sp++;

    // emit
    LOGI("[emit] r%" PRIu32 " <-- global[%" PRIu32 "]", (u32)reg_idx, (u32)(global_idx));
    return ok_r;
}

static r ir_builder_on_global_set(void * payload, stream imm, u32 global_idx) {
    ir_builder_context * ctx = (ir_builder_context *)payload;
    // Get the global type
    global * g = vec_at_global(&ctx->mod->globals, global_idx);
    reg_type type = TYPEID_TO_REG_TYPE(g->valtype);

    // get the stack slot
    assert(ctx->sp);
    stack_slot * stack_val = vec_at_stack_slot(&ctx->stack_mappings, (--ctx->sp));
    if (stack_val->aliased) {
        assert(stack_val->u.local_idx);
        local_slot * local_val = vec_at_local_slot(&ctx->local_mappings, stack_val->u.local_idx);
        assert(local_val->alias_count);
        local_val->alias_count--;
        LOGI("[emit] r%" PRIu32 " --> global[%" PRIu32 "]", (u32)(local_val->reg_idx), (u32)(global_idx));
    } else {
        if (stack_val->u.reg_idx == INVALID_IDX_U16) {
            // spilled. Let's allocate a register
            u16 reg_idx = get_reg(ctx, type, imm);
            assert(reg_idx < vec_size_reg_state(&ctx->reg_mappings));
            reg_state * reg = vec_at_reg_state(&ctx->reg_mappings, reg_idx);
            reg->target = tgt_stack;
            reg->type = type;
            reg->tgt_idx = ctx->sp;
            // register to the stack slot
            stack_val->u.reg_idx = reg_idx;

            // load the data from the stack to the register
        }
    }
    return ok_r;
}

static r ir_builder_on_table_get(void * payload, stream imm, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_table_set(void * payload, stream imm, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_memory_load_store(void * payload, wasm_opcode opcode, stream imm, u8 align, u32 offset) {
    return ok_r;
}

static r ir_builder_on_memory_size(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_memory_grow(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_i32_const(void * payload, stream imm, i32 val) {
    return ok_r;
}

static r ir_builder_on_i64_const(void * payload, stream imm, i64 val) {
    return ok_r;
}

static r ir_builder_on_f32_const(void * payload, stream imm, f32 val) {
    return ok_r;
}

static r ir_builder_on_f64_const(void * payload, stream imm, f64 val) {
    return ok_r;
}

static r ir_builder_on_itestop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_iunop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_irelop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_ibinop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_funop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_frelop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_fbinop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_wrapop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_truncop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_convertop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_rankop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_reinterpretop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_extendop(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_ref_null(void * payload, wasm_opcode opcode, stream imm, type_id type) {
    return ok_r;
}

static r ir_builder_on_ref_is_null(void * payload, wasm_opcode opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_ref_func(void * payload, stream imm, u32 fref) {
    return ok_r;
}

static r ir_builder_on_opcode_fc(void * payload, wasm_opcode_fc opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_trunc_sat(void * payload, wasm_opcode_fc opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_memory_init(void * payload, stream imm, u32 data_idx) {
    return ok_r;
}

static r ir_builder_on_memory_copy(void * payload, wasm_opcode_fc opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_memory_fill(void * payload, wasm_opcode_fc opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_data_drop(void * payload, stream imm, u32 data_idx) {
    return ok_r;
}

static r ir_builder_on_table_init(void * payload, stream imm, u32 elem_idx, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_elem_drop(void * payload, stream imm, u32 elem_idx) {
    return ok_r;
}

static r ir_builder_on_table_copy(void * payload, stream imm, u32 table_idx_dst, u32 table_idx_src) {
    return ok_r;
}

static r ir_builder_on_table_grow(void * payload, stream imm, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_table_size(void * payload, stream imm, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_table_fill(void * payload, stream imm, u32 table_idx) {
    return ok_r;
}

static r ir_builder_on_opcode_fd(void * payload, wasm_opcode_fd opcode, stream imm) {
    return ok_r;
}

static r ir_builder_on_decode_end(void * payload) {
    LOGI("ir_builder end");
    return ok_r;
}

static const op_decoder_callbacks ir_builder_callbacks = {
    .on_decode_begin = ir_builder_on_decode_begin,
    .on_opcode = ir_builder_on_opcode,
    .on_unreachable = ir_builder_on_unreachable,
    .on_nop = NULL,
    .on_block = ir_builder_on_block,
    .on_else = ir_builder_on_else,
    .on_end = ir_builder_on_end,
    .on_br_or_if = ir_builder_on_br_or_if,
    .on_br_table = ir_builder_on_br_table,
    .on_return = ir_builder_on_return,
    .on_call = ir_builder_on_call,
    .on_call_indirect = ir_builder_on_call_indirect,
    .on_drop = ir_builder_on_drop,
    .on_select = ir_builder_on_select,
    .on_select_t = ir_builder_on_select_t,
    .on_local_get = ir_builder_on_local_get,
    .on_local_set = ir_builder_on_local_set,
    .on_local_tee = ir_builder_on_local_tee,
    .on_global_get = ir_builder_on_global_get,
    .on_global_set = ir_builder_on_global_set,
    .on_table_get = ir_builder_on_table_get,
    .on_table_set = ir_builder_on_table_set,
    .on_memory_load_store = ir_builder_on_memory_load_store,
    .on_memory_size = ir_builder_on_memory_size,
    .on_memory_grow = ir_builder_on_memory_grow,
    .on_i32_const = ir_builder_on_i32_const,
    .on_i64_const = ir_builder_on_i64_const,
    .on_f32_const = ir_builder_on_f32_const,
    .on_f64_const = ir_builder_on_f64_const,
    .on_iunop = ir_builder_on_iunop,
    .on_funop = ir_builder_on_funop,
    .on_ibinop = ir_builder_on_ibinop,
    .on_fbinop = ir_builder_on_fbinop,
    .on_itestop = ir_builder_on_itestop,
    .on_irelop = ir_builder_on_irelop,
    .on_frelop = ir_builder_on_frelop,
    .on_wrapop = ir_builder_on_wrapop,
    .on_truncop = ir_builder_on_truncop,
    .on_convertop = ir_builder_on_convertop,
    .on_rankop = ir_builder_on_rankop,
    .on_reinterpretop = ir_builder_on_reinterpretop,
    .on_extendop = ir_builder_on_extendop,
    .on_ref_null = ir_builder_on_ref_null,
    .on_ref_is_null = ir_builder_on_ref_is_null,
    .on_ref_func = ir_builder_on_ref_func,
    .on_opcode_fc = ir_builder_on_opcode_fc,
    .on_trunc_sat = ir_builder_on_trunc_sat,
    .on_memory_init = ir_builder_on_memory_init,
    .on_memory_copy = ir_builder_on_memory_copy,
    .on_memory_fill = ir_builder_on_memory_fill,
    .on_data_drop = ir_builder_on_data_drop,
    .on_table_init = ir_builder_on_table_init,
    .on_elem_drop = ir_builder_on_elem_drop,
    .on_table_copy = ir_builder_on_table_copy,
    .on_table_grow = ir_builder_on_table_grow,
    .on_table_size = ir_builder_on_table_size,
    .on_table_fill = ir_builder_on_table_fill,
    .on_opcode_fd = ir_builder_on_opcode_fd,
    .on_decode_end = ir_builder_on_decode_end,
};

r build_ir(module * mod, func * f) {
    check_prep(r);
    assert(mod);
    assert(f);
    vec_u32 local_lifespan_table = {0};
    // build the lifespan table of the locals
    {
        local_lifespan_context ctx = {0};
        ctx.mod = mod;
        ctx.f = f;
        check(decode_function(mod, f, &local_lifespan_callbacks, &ctx), vec_clear_u32(&ctx.local_lifespan_table));
        local_lifespan_table = ctx.local_lifespan_table;
    }
    // emit ir
    {
        ir_builder_context ctx = {0};
        ctx.mod = mod;
        ctx.f = f;
        ctx.local_lifespan_table = local_lifespan_table;
        ctx.sp = 0;

        // fill in the local vector with the types from params and locals
        check(vec_resize_local_slot(&ctx.local_mappings, f->local_count), ir_builder_context_drop(&ctx));
        assert(f->fn_type.param_count + vec_size_type_id(&f->local_types) == f->local_count);
        unwrap(vec_type_id, param_types, parse_types(f->fn_type.param_count, f->fn_type.params));
        for (size_t i = 0; i < vec_size_type_id(&param_types); i++) {
            vec_at_local_slot(&ctx.local_mappings, i)->type = TYPEID_TO_REG_TYPE(*vec_at_type_id(&param_types, i));
        }
        vec_clear_type_id(&param_types);
        for (size_t j = f->fn_type.param_count; j < f->local_count; j++) {
            vec_at_local_slot(&ctx.local_mappings, j)->type = TYPEID_TO_REG_TYPE(*vec_at_type_id(&f->local_types, j - f->fn_type.param_count));
        }
        // defaults to not assigned.
        VEC_FOR_EACH(&ctx.local_mappings, local_slot, slot) {
            slot->reg_idx = INVALID_IDX_U16;
        }

        // not prepare the stack
        check(vec_resize_stack_slot(&ctx.stack_mappings, f->stack_size_max), ir_builder_context_drop(&ctx));
        VEC_FOR_EACH(&ctx.stack_mappings, stack_slot, slot) {
            slot->aliased = false;
            slot->u.reg_idx = INVALID_IDX_U16;
        }

        // now the registers
        check(vec_resize_reg_state(&ctx.reg_mappings, GPR_COUNT + FPR_COUNT), ir_builder_context_drop(&ctx));
        for (size_t i = 0; i < vec_size_reg_state(&ctx.reg_mappings); i++) {
            reg_state * reg = vec_at_reg_state(&ctx.reg_mappings, i);
            // it's a bit hacky but let's make it simple: GPR are always in front of the FPR
            reg->type = i < GPR_COUNT ? GPR : FPR;
            reg->tgt_idx = INVALID_IDX_U16;
        }

        // decode start
        check(decode_function(mod, f, &ir_builder_callbacks, &ctx), ir_builder_context_drop(&ctx));
        ir_builder_context_drop(&ctx);
    }
    return ok_r;
}

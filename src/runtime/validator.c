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

#include "validator.h"

#include "logger.h"
#include "op_decoder.h"
#include "opcode.h"
#include "parser.h"
#include "stream.h"
#include "vec_impl.h"

#define LOGI(fmt, ...) LOG_INFO(log_channel_validator, fmt, __VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_validator, fmt, __VA_ARGS__)

VEC_DECL_FOR_TYPE(ctrl_frame)
VEC_IMPL_FOR_TYPE(ctrl_frame)

typedef struct validator_context {
    vec_type_id locals;
    vec_type_id val_stack;
    vec_ctrl_frame ctrl_stack;
    u32 stack_size_max;
    module * mod;
    func * f;
} validator_context;

static r push_val(type_id type, validator_context * ctx) {
    assert(ctx);
    check_prep(r);
    check(vec_push_type_id(&ctx->val_stack, type));
    if (vec_size_type_id(&ctx->val_stack) > ctx->stack_size_max) {
        ctx->stack_size_max = (u32)vec_size_type_id(&ctx->val_stack);
    }
    return ok_r;
}

static r push_vals(vec_type_id * vals, validator_context * ctx) {
    assert(vals);
    assert(ctx);
    check_prep(r);
    VEC_FOR_EACH(vals, type_id, v) {
        check(push_val(*v, ctx));
    }
    return ok_r;
}

static r_type_id pop_val(validator_context * ctx) {
    assert(ctx);
    ctrl_frame * frame = vec_back_ctrl_frame(&ctx->ctrl_stack);
    check_prep(r_type_id);
    // stack-polymorphic
    if (frame->unreachable && (frame->height == vec_size_type_id(&ctx->val_stack))) {
        return ok(TYPE_ID_unknown);
    }
    if (frame->height == vec_size_type_id(&ctx->val_stack)) {
        return err(e_invalid, "Stack underrun");
    }
    type_id real_type = *vec_back_type_id(&ctx->val_stack);
    vec_pop_type_id(&ctx->val_stack);
    return ok(real_type);
}

static r_type_id pop_val_expect(type_id expected_type, validator_context * ctx) {
    assert(ctx);
    check_prep(r_type_id);
    unwrap(type_id, actual, pop_val(ctx));

    if ((actual != expected_type) && (actual != TYPE_ID_unknown) && (expected_type != TYPE_ID_unknown)) {
        if (!((expected_type == TYPE_ID_ref) && is_ref(actual))) {
            return err(e_invalid, "Value type mismatch");
        }
    }
    return ok(actual);
}

static r_vec_type_id pop_vals_to_vec(vec_type_id * vals, validator_context * ctx) {
    assert(vals);
    assert(ctx);
    check_prep(r_vec_type_id);

    vec_type_id popped_vals = {0};
    VEC_FOR_EACH_REVERSE(vals, type_id, v) {
        unwrap(type_id, t, pop_val_expect(*v, ctx), {
            vec_clear_type_id(&popped_vals);
        });
        check(vec_push_type_id(&popped_vals, t), {
            vec_clear_type_id(&popped_vals);
        });
    }
    return ok(popped_vals);
}

// pop vals and then drop it.
static r pop_vals(vec_type_id * vals, validator_context * ctx) {
    assert(vals);
    assert(ctx);
    check_prep(r);
    unwrap(vec_type_id, popped_vals, pop_vals_to_vec(vals, ctx));
    vec_clear_type_id(&popped_vals);
    return ok_r;
}

static r push_ctrl(blocktype bt, func_type ftype, validator_context * ctx) {
    assert(ctx);
    check_prep(r);

    check(vec_push_ctrl_frame(&ctx->ctrl_stack, (ctrl_frame){0}));
    ctrl_frame * frame = vec_back_ctrl_frame(&ctx->ctrl_stack);
    frame->type = bt;
    frame->ftype = ftype;
    frame->height = vec_size_type_id(&ctx->val_stack);
    frame->unreachable = false;
    unwrap(vec_type_id, params, parse_types(ftype.param_count, ftype.params));
    frame->start_types = params;
    unwrap(vec_type_id, results, parse_types(ftype.result_count, ftype.results));
    frame->end_types = results;
    check(push_vals(&frame->start_types, ctx));
    return ok_r;
}

// be sure to release the ctrl frame after the pop
static r_ctrl_frame pop_ctrl(validator_context * ctx) {
    assert(ctx);
    check_prep(r_ctrl_frame);

    if (!vec_size_ctrl_frame(&ctx->ctrl_stack)) {
        return err(e_invalid, "Control stack underrun");
    }
    ctrl_frame top_frame = *vec_back_ctrl_frame(&ctx->ctrl_stack);
    check(pop_vals(&top_frame.end_types, ctx));
    vec_pop_ctrl_frame(&ctx->ctrl_stack);
    return ok(top_frame);
}

void release_ctrl_frame(ctrl_frame * frame) {
    vec_clear_type_id(&frame->start_types);
    vec_clear_type_id(&frame->end_types);
    vec_clear_u32(&frame->pending_jt_slots);
}

vec_type_id * label_types(ctrl_frame * frame) {
    if (frame->type == bt_loop) {
        return &frame->start_types;
    } else {
        return &frame->end_types;
    }
}

static r unreachable(validator_context * ctx) {
    assert(ctx);
    check_prep(r);

    ctrl_frame * frame = vec_back_ctrl_frame(&ctx->ctrl_stack);

    if (vec_size_type_id(&ctx->val_stack) < frame->height) {
        return err(e_invalid, "Stack underrun");
    }
    check(vec_resize_type_id(&ctx->val_stack, frame->height));
    frame->unreachable = true;
    return ok_r;
}

static void validator_context_drop(validator_context * ctx) {
    VEC_FOR_EACH(&ctx->ctrl_stack, ctrl_frame, fr) {
        release_ctrl_frame(fr);
    }
    vec_clear_ctrl_frame(&ctx->ctrl_stack);
    vec_clear_type_id(&ctx->val_stack);
    vec_clear_type_id(&ctx->locals);
}

//////////////////////////////////////////////////////////////////////////////
// callback impl
static r validator_on_decode_begin(void * payload) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    func * f = ctx->f;
    LOGI("%s", "validator begin");

    assert(!vec_size_type_id(&ctx->val_stack));
    vec_type_id * locals = &ctx->locals;

    // prepare the locals vector.
    assert(f->fn_type.param_count <= f->local_count);
    check(vec_reserve_type_id(locals, f->local_count));
    unwrap(vec_type_id, func_params, parse_types(f->fn_type.param_count, f->fn_type.params));
    VEC_FOR_EACH(&func_params, type_id, param) {
        check(vec_push_type_id(locals, *param), {
            vec_clear_type_id(&func_params);
        });
    }
    vec_clear_type_id(&func_params);
    VEC_FOR_EACH(&f->local_types, type_id, local) {
        check(vec_push_type_id(locals, *local));
    }
    assert(vec_size_type_id(locals) == f->local_count);

    // push the first frame
    assert(!vec_size_ctrl_frame(&ctx->ctrl_stack));
    check(push_ctrl(bt_func, f->fn_type, ctx));
    // we need to clear the pushed vals because the params of the 1st frame is in the local
    vec_clear_type_id(&ctx->val_stack);
    return ok_r;
}

static r validator_on_opcode(void * payload, wasm_opcode opcode, stream imm) {
#ifdef LOG_INFO_ENABLED
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    const char * op_name = get_op_name(opcode);
    u32 local_count = ctx->f->local_count;
    u32 stack_height = (u32)vec_size_type_id(&ctx->val_stack);
    u32 op_offset = (u32)(imm.p - mod->wasm_bin.s.ptr - 1);
    u32 ctrl_frames = (u32)vec_size_ctrl_frame(&ctx->ctrl_stack) - 1;
    if (ctrl_frames && (opcode == op_end || opcode == op_else)) {
        ctrl_frames--;
    }
    LOGI("%08x locals:%-3u stack:%-3u |%*s%s", op_offset, local_count, stack_height, ctrl_frames * 2, "", op_name);
#endif // LOG_INFO_ENABLED
    return ok_r;
}

static r validator_on_unreachable(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    check(unreachable((validator_context *)payload));
    return ok_r;
}

static r validator_on_block(void * payload, wasm_opcode opcode, stream imm, func_type type) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_jump_table * jt = &ctx->f->jt;

    if (opcode == op_if) {
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    }
    unwrap(vec_type_id, param_types, parse_types(type.param_count, type.params));
    check(pop_vals(&param_types, ctx), {
        vec_clear_type_id(&param_types);
    });
    vec_clear_type_id(&param_types);
    blocktype bt = (opcode == op_block ? bt_block : (opcode == op_loop ? bt_loop : bt_if));
    check(push_ctrl(bt, type, ctx));
    // update the jump table
    u32 pc_offset = (u32)(imm.p - imm.s.ptr);
    // The pc_offset target for loop is pointing at the instruction next to the loop because
    // all the block instructions are just nop
    if (opcode == op_loop) {
        pc_offset += 1;
        vec_back_ctrl_frame(&ctx->ctrl_stack)->pc = pc_offset;
    }
    // the "if" block needs a jump table slot.
    if (opcode == op_if) {
        check(vec_push_jump_table(jt, (jump_table){
                                          .pc = pc_offset,
                                          .stack_offset = 0,
                                          .arity = 0,
                                          .next_idx = JUMP_TABLE_IDX_INVALID,
                                      }));
        check(vec_push_u32(&vec_back_ctrl_frame(&ctx->ctrl_stack)->pending_jt_slots, (u32)(vec_size_jump_table(jt) - 1)));
    }
    return ok_r;
}

static r validator_on_else(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    ctrl_frame * current_frame = vec_back_ctrl_frame(&ctx->ctrl_stack);
    vec_jump_table * jt = &ctx->f->jt;

    if (current_frame->type != bt_if) {
        return err(e_invalid, "if/else mismatch");
    }
    // the ctrl_frame is ripped out of the stack, so it has to be
    // released because it owns two vectors.
    unwrap(ctrl_frame, cf, pop_ctrl(ctx));
    check(push_ctrl(bt_else, cf.ftype, ctx), {
        release_ctrl_frame(&cf);
    });
    // move the pending jt slots except the first one to the else block
    // the if slot's jump target is the else block, and all other br always point to the end
    // and if there's no else block, the "if" block will be just like a normal br that points
    // to the end
    ctrl_frame * else_frame = vec_back_ctrl_frame(&ctx->ctrl_stack);
    u32 pc_offset = (u32)(imm.p - imm.s.ptr);
    for (u32 i = 0; i < vec_size_u32(&cf.pending_jt_slots); i++) {
        u32 slot_idx = *vec_at_u32(&cf.pending_jt_slots, i);
        if (i == 0) {
            jump_table * record = vec_at_jump_table(jt, slot_idx);
            record->target_offset = pc_offset - record->pc;
            continue;
        }
        check(vec_push_u32(&else_frame->pending_jt_slots, slot_idx), {
            release_ctrl_frame(&cf);
        });
    }
    // push the else itself. else is like a "br 0". When we run into an else, we jump out of the frame
    u16 arity = (u16)vec_size_type_id(label_types(else_frame));
    check(vec_push_jump_table(jt, (jump_table){
                                      .pc = pc_offset,
                                      .stack_offset = (u32)(vec_size_type_id(&ctx->val_stack) - arity - else_frame->height),
                                      .arity = arity,
                                      .next_idx = JUMP_TABLE_IDX_INVALID,
                                  }),
          release_ctrl_frame(&cf));
    u32 idx = (u32)vec_size_jump_table(jt) - 1;
    check(vec_push_u32(&else_frame->pending_jt_slots, idx));
    //done.
    release_ctrl_frame(&cf);
    return ok_r;
}

static r validator_on_end(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_jump_table * jt = &ctx->f->jt;

    // Notice that the cf.type could be bt_func, so when we pop the last frame
    // we're actually checking the return values. So no need to double check on
    // the on_decode_end callback.
    unwrap(ctrl_frame, cf, pop_ctrl(ctx));

    // update the pending jump slots with the correct PC offset
    if (cf.type == bt_loop) {
        VEC_FOR_EACH(&cf.pending_jt_slots, u32, slot_idx) {
            jump_table * record = vec_at_jump_table(jt, *slot_idx);
            record->target_offset = cf.pc - record->pc;
        }
    } else {
        VEC_FOR_EACH(&cf.pending_jt_slots, u32, slot_idx) {
            jump_table * record = vec_at_jump_table(jt, *slot_idx);
            u32 pc_offset = (u32)(imm.p - imm.s.ptr);
            record->target_offset = pc_offset - record->pc;
            if (cf.type == bt_func) {
                // a tiny optimization for the br that jumps out of a function.
                // when it happens the branch target will be pointing to the last end.
                record->target_offset--;
            }
        }
    }
    // done
    check(push_vals(&cf.end_types, ctx), {
        release_ctrl_frame(&cf);
    });
    release_ctrl_frame(&cf);
    return ok_r;
}

static r validator_on_br_or_if(void * payload, wasm_opcode opcode, stream imm, u8 lth) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_jump_table * jt = &ctx->f->jt;

    if (lth >= vec_size_ctrl_frame(&ctx->ctrl_stack)) {
        return err(e_invalid, "invalid br n");
    }
    if (opcode == op_br_if) {
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    }
    ctrl_frame * target_frame = vec_at_ctrl_frame(&ctx->ctrl_stack, vec_size_ctrl_frame(&ctx->ctrl_stack) - lth - 1);
    // update the jump table
    u16 arity = (u16)vec_size_type_id(label_types(target_frame));
    check(vec_push_jump_table(jt, (jump_table){
                                      .pc = (u32)(imm.p - imm.s.ptr),
                                      .stack_offset = (u32)(vec_size_type_id(&ctx->val_stack) - arity - target_frame->height),
                                      .arity = arity,
                                      .next_idx = JUMP_TABLE_IDX_INVALID,
                                  }));
    u32 idx = (u32)(vec_size_jump_table(jt) - 1);
    check(vec_push_u32(&target_frame->pending_jt_slots, idx));
    // done.
    check(pop_vals(label_types(target_frame), ctx));
    if (opcode == op_br) {
        check(unreachable(ctx));
    } else {
        check(push_vals(label_types(target_frame), ctx));
    }
    return ok_r;
}

static r validator_on_br_table(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_jump_table * jt = &ctx->f->jt;

    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap(u32, table_len, stream_read_vu32(&imm));
    // we need to find out the arity, which depends on the last table item, so we
    // there has to be two passes.
    size_t arity = 0;
    stream pc_copy = imm;
    ctrl_frame * m_frame = NULL;
    for (u32 i = 0; i < (table_len + 1); i++) {
        unwrap(u32, l, stream_read_vu32(&pc_copy));
        if (l >= vec_size_ctrl_frame(&ctx->ctrl_stack)) {
            return err(e_invalid, "invalid br table n");
        }
        if (i == table_len) {
            m_frame = vec_at_ctrl_frame(&ctx->ctrl_stack, vec_size_ctrl_frame(&ctx->ctrl_stack) - l - 1);
            arity = vec_size_type_id(label_types(m_frame));
        }
    }
    assert(m_frame);
    // second pass, check each frame's arity
    for (u32 j = 0; j < table_len + 1; j++) {
        unwrap(u32, l, stream_read_vu32(&imm));
        ctrl_frame * target_frame = vec_at_ctrl_frame(&ctx->ctrl_stack, vec_size_ctrl_frame(&ctx->ctrl_stack) - l - 1);
        if (arity != vec_size_type_id(label_types(target_frame))) {
            return err(e_invalid, "br table arity mismatch");
        }
        // update the jump table
        // br_table is an exception where the pc offset points to the end of the instruction. This way we can
        // simply reuse the code between all br handlers.
        check(vec_push_jump_table(jt, (jump_table){
                                          .pc = (u32)(pc_copy.p - pc_copy.s.ptr),
                                          .stack_offset = (u32)(vec_size_type_id(&ctx->val_stack) - arity - target_frame->height),
                                          .arity = (u16)arity,
                                          .next_idx = JUMP_TABLE_IDX_INVALID,
                                      }));
        check(vec_push_u32(&target_frame->pending_jt_slots, (u32)(vec_size_jump_table(jt) - 1)));
        // done.
        unwrap(vec_type_id, popped, pop_vals_to_vec(label_types(target_frame), ctx));
        VEC_REVERSE(&popped);
        check(push_vals(&popped, ctx), {
            vec_clear_type_id(&popped);
        });
        vec_clear_type_id(&popped);
    }
    check(pop_vals(label_types(m_frame), ctx));
    check(unreachable(ctx));
    return ok_r;
}

static r validator_on_return(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    ctrl_frame * target_frame = vec_at_ctrl_frame(&ctx->ctrl_stack, 0);
    check(pop_vals(label_types(target_frame), ctx));
    check(unreachable(ctx));
    return ok_r;
}

static r validator_on_call(void * payload, stream imm, u32 func_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (func_idx >= vec_size_func(&mod->funcs)) {
        return err(e_invalid, "Invalid function index");
    }
    func * fn = vec_at_func(&mod->funcs, func_idx);
    // pop params
    unwrap(vec_type_id, param_types, parse_types(fn->fn_type.param_count, fn->fn_type.params));
    check(pop_vals(&param_types, ctx), {
        vec_clear_type_id(&param_types);
    });
    vec_clear_type_id(&param_types);
    // calculate the potential maximum stack size with the callee's locals
    u32 required_size = (u32)(vec_size_type_id(&ctx->val_stack) + fn->local_count);
    if (required_size > ctx->stack_size_max) {
        ctx->stack_size_max = required_size;
    }
    // push results
    unwrap(vec_type_id, result_types, parse_types(fn->fn_type.result_count, fn->fn_type.results));
    check(push_vals(&result_types, ctx), {
        vec_clear_type_id(&result_types);
    });
    vec_clear_type_id(&result_types);
    return ok_r;
}

static r validator_on_call_indirect(void * payload, stream imm, u32 type_idx, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "Invalid table index");
    }
    if (type_idx >= vec_size_func_type(&mod->func_types)) {
        return err(e_invalid, "Invalid type index");
    }
    func_type * ft = vec_at_func_type(&mod->func_types, type_idx);
    // pop params
    unwrap(vec_type_id, param_types, parse_types(ft->param_count, ft->params));
    check(pop_vals(&param_types, ctx), {
        vec_clear_type_id(&param_types);
    });
    vec_clear_type_id(&param_types);
    // push results
    unwrap(vec_type_id, result_types, parse_types(ft->result_count, ft->results));
    check(push_vals(&result_types, ctx), {
        vec_clear_type_id(&result_types);
    });
    vec_clear_type_id(&result_types);
    return ok_r;
}

static r validator_on_drop(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    unwrap_drop(type_id, pop_val(ctx));
    return ok_r;
}

static r validator_on_select(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap(type_id, t1, pop_val(ctx));
    unwrap(type_id, t2, pop_val(ctx));
    if (!((is_num(t1) && is_num(t2)) || (is_vec(t1) && is_vec(t2)))) {
        return err(e_invalid, "select operand type mismatch");
    }
    if ((t1 != t2) && (t1 != TYPE_ID_unknown) && (t2 != TYPE_ID_unknown)) {
        return err(e_invalid, "select operand type mismatch");
    }
    check(push_val(t1 == TYPE_ID_unknown ? t2 : t1, ctx));
    return ok_r;
}

static r validator_on_select_t(void * payload, wasm_opcode opcode, stream imm, type_id type) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(type, ctx));
    unwrap_drop(type_id, pop_val_expect(type, ctx));
    check(push_val(type, ctx));
    return ok_r;
}

static r validator_on_local_get(void * payload, stream imm, u32 local_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_type_id * locals = &ctx->locals;
    if (local_idx >= vec_size_type_id(locals)) {
        return err(e_invalid, "Invalid local index");
    }
    type_id local_type = *vec_at_type_id(locals, local_idx);
    check(push_val(local_type, ctx));
    return ok_r;
}

static r validator_on_local_set(void * payload, stream imm, u32 local_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_type_id * locals = &ctx->locals;
    if (local_idx >= vec_size_type_id(locals)) {
        return err(e_invalid, "Invalid local index");
    }
    type_id local_type = *vec_at_type_id(locals, local_idx);
    unwrap_drop(type_id, pop_val_expect(local_type, ctx));
    return ok_r;
}

static r validator_on_local_tee(void * payload, stream imm, u32 local_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_type_id * locals = &ctx->locals;
    if (local_idx >= vec_size_type_id(locals)) {
        return err(e_invalid, "Invalid local index");
    }
    type_id local_type = *vec_at_type_id(locals, local_idx);
    unwrap_drop(type_id, pop_val_expect(local_type, ctx));
    check(push_val(local_type, ctx));
    return ok_r;
}

static r validator_on_global_get(void * payload, stream imm, u32 global_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (global_idx >= vec_size_global(&mod->globals)) {
        return err(e_invalid, "Invalid global index");
    }
    global * g = vec_at_global(&mod->globals, global_idx);
    check(push_val(g->valtype, ctx));
    return ok_r;
}

static r validator_on_global_set(void * payload, stream imm, u32 global_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (global_idx >= vec_size_global(&mod->globals)) {
        return err(e_invalid, "Invalid global index");
    }
    global * g = vec_at_global(&mod->globals, global_idx);
    unwrap_drop(type_id, pop_val_expect(g->valtype, ctx));
    return ok_r;
}

static r validator_on_table_get(void * payload, stream imm, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "Invalid table index");
    }
    table * tab = vec_at_table(&mod->tables, table_idx);
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    check(push_val(tab->valtype, ctx));
    return ok_r;
}

static r validator_on_table_set(void * payload, stream imm, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "Invalid table index");
    }
    table * tab = vec_at_table(&mod->tables, table_idx);
    unwrap_drop(type_id, pop_val_expect(tab->valtype, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_memory_load_store(void * payload, wasm_opcode opcode, stream imm, u8 align, u32 offset) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (!vec_size_memory(&mod->memories)) {
        return err(e_invalid, "mem0 not found");
    }
    // TODO: check the offset against the limits
#define MEM_LOAD(dst_type)                                      \
    {                                                           \
        if (align > sizeof(dst_type)) {                         \
            return err(e_invalid, "Invalid alignment");         \
        }                                                       \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx)); \
        check(push_val(TYPE_ID_##dst_type, ctx));               \
        break;                                                  \
    }

#define MEM_STORE(src_type)                                            \
    {                                                                  \
        if (align > sizeof(src_type)) {                                \
            return err(e_invalid, "Invalid alignment");                \
        }                                                              \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_##src_type, ctx)); \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));        \
        break;                                                         \
    }

    switch (opcode) {
        case op_i32_load:
        case op_i32_load8_s:
        case op_i32_load8_u:
        case op_i32_load16_s:
        case op_i32_load16_u:
            MEM_LOAD(i32);
        case op_i64_load:
        case op_i64_load8_s:
        case op_i64_load8_u:
        case op_i64_load16_s:
        case op_i64_load16_u:
        case op_i64_load32_s:
        case op_i64_load32_u:
            MEM_LOAD(i64);
        case op_f32_load:
            MEM_LOAD(f32);
        case op_f64_load:
            MEM_LOAD(f64);
        case op_i32_store:
        case op_i32_store8:
        case op_i32_store16:
            MEM_STORE(i32);
        case op_i64_store:
        case op_i64_store8:
        case op_i64_store16:
        case op_i64_store32:
            MEM_STORE(i64);
        case op_f32_store:
            MEM_STORE(f32);
        case op_f64_store:
            MEM_STORE(f64);
        default:
            assert(false);
            break;
    }
    return ok_r;
}

static r validator_on_memory_size(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_memory_grow(void * payload, wasm_opcode opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    check(push_val(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_i32_const(void * payload, stream imm, i32 val) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_i64_const(void * payload, stream imm, i64 val) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(TYPE_ID_i64, ctx));
    return ok_r;
}

static r validator_on_f32_const(void * payload, stream imm, f32 val) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(TYPE_ID_f32, ctx));
    return ok_r;
}

static r validator_on_f64_const(void * payload, stream imm, f64 val) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(TYPE_ID_f64, ctx));
    return ok_r;
}

#define POP1_PUSH(pop_type, push_type)                                 \
    {                                                                  \
        check_prep(r);                                                 \
        validator_context * ctx = (validator_context *)payload;        \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_##pop_type, ctx)); \
        check(push_val(TYPE_ID_##push_type, ctx));                     \
        return ok_r;                                                   \
    }

#define POP2_PUSH(pop_type, push_type)                                 \
    {                                                                  \
        check_prep(r);                                                 \
        validator_context * ctx = (validator_context *)payload;        \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_##pop_type, ctx)); \
        unwrap_drop(type_id, pop_val_expect(TYPE_ID_##pop_type, ctx)); \
        check(push_val(TYPE_ID_##push_type, ctx));                     \
        return ok_r;                                                   \
    }

static r validator_on_itestop(void * payload, wasm_opcode opcode, stream imm) {
    if (opcode == op_i32_eqz) {
        POP1_PUSH(i32, i32);
    } else { // op_i64_eqz
        POP1_PUSH(i64, i32);
    }
}

static r validator_on_iunop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_i32_clz:
        case op_i32_ctz:
        case op_i32_popcnt:
            POP1_PUSH(i32, i32);
        case op_i64_clz:
        case op_i64_ctz:
        case op_i64_popcnt:
            POP1_PUSH(i64, i64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_irelop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
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
            POP2_PUSH(i32, i32);
        case op_i64_eq:
        case op_i64_ne:
        case op_i64_lt_s:
        case op_i64_lt_u:
        case op_i64_gt_s:
        case op_i64_gt_u:
        case op_i64_le_s:
        case op_i64_le_u:
        case op_i64_ge_s:
        case op_i64_ge_u:
            POP2_PUSH(i64, i32);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_ibinop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
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
            POP2_PUSH(i32, i32);
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
        case op_i64_rotr:
            POP2_PUSH(i64, i64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_funop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_f32_abs:
        case op_f32_neg:
        case op_f32_ceil:
        case op_f32_floor:
        case op_f32_trunc:
        case op_f32_nearest:
        case op_f32_sqrt:
            POP1_PUSH(f32, f32);
        case op_f64_abs:
        case op_f64_neg:
        case op_f64_ceil:
        case op_f64_floor:
        case op_f64_trunc:
        case op_f64_nearest:
        case op_f64_sqrt:
            POP1_PUSH(f64, f64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_frelop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_f32_eq:
        case op_f32_ne:
        case op_f32_lt:
        case op_f32_gt:
        case op_f32_le:
        case op_f32_ge:
            POP2_PUSH(f32, i32);
        case op_f64_eq:
        case op_f64_ne:
        case op_f64_lt:
        case op_f64_gt:
        case op_f64_le:
        case op_f64_ge:
            POP2_PUSH(f64, i32);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_fbinop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_f32_add:
        case op_f32_sub:
        case op_f32_mul:
        case op_f32_div:
        case op_f32_min:
        case op_f32_max:
        case op_f32_copysign:
            POP2_PUSH(f32, f32);
        case op_f64_add:
        case op_f64_sub:
        case op_f64_mul:
        case op_f64_div:
        case op_f64_min:
        case op_f64_max:
        case op_f64_copysign:
            POP2_PUSH(f64, f64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_wrapop(void * payload, wasm_opcode opcode, stream imm) {
    assert(opcode == op_i32_wrap_i64);
    POP1_PUSH(i64, i32);
}

static r validator_on_truncop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_i32_trunc_f32_s:
        case op_i32_trunc_f32_u:
            POP1_PUSH(f32, i32);
        case op_i32_trunc_f64_s:
        case op_i32_trunc_f64_u:
            POP1_PUSH(f64, i32);
        case op_i64_trunc_f32_s:
        case op_i64_trunc_f32_u:
            POP1_PUSH(f32, i64);
        case op_i64_trunc_f64_s:
        case op_i64_trunc_f64_u:
            POP1_PUSH(f64, i64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_convertop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_f32_convert_i32_s:
        case op_f32_convert_i32_u:
            POP1_PUSH(i32, f32);
        case op_f32_convert_i64_s:
        case op_f32_convert_i64_u:
            POP1_PUSH(i64, f32);
        case op_f64_convert_i32_s:
        case op_f64_convert_i32_u:
            POP1_PUSH(i32, f64);
        case op_f64_convert_i64_s:
        case op_f64_convert_i64_u:
            POP1_PUSH(i64, f64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_rankop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_f32_demote_f64:
            POP1_PUSH(f64, f32);
        case op_f64_promote_f32:
            POP1_PUSH(f32, f64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_reinterpretop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_i32_reinterpret_f32:
            POP1_PUSH(f32, i32);
        case op_i64_reinterpret_f64:
            POP1_PUSH(f64, i64);
        case op_f32_reinterpret_i32:
            POP1_PUSH(i32, f32);
        case op_f64_reinterpret_i64:
            POP1_PUSH(i64, f64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_extendop(void * payload, wasm_opcode opcode, stream imm) {
    switch (opcode) {
        case op_i64_extend_i32_s:
        case op_i64_extend_i32_u:
            POP1_PUSH(i32, i64);
        case op_i32_extend8_s:
        case op_i32_extend16_s:
            POP1_PUSH(i32, i32);
        case op_i64_extend8_s:
        case op_i64_extend16_s:
        case op_i64_extend32_s:
            POP1_PUSH(i64, i64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_ref_null(void * payload, wasm_opcode opcode, stream imm, type_id type) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    check(push_val(type, ctx));
    return ok_r;
}

static r validator_on_ref_is_null(void * payload, wasm_opcode opcode, stream imm) {
    POP1_PUSH(ref, i32);
}

static r validator_on_ref_func(void * payload, stream imm, u32 fref) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (fref >= vec_size_func(&mod->funcs)) {
        return err(e_invalid, "Invalid function index");
    }
    check(push_val(TYPE_ID_funcref, ctx));
    return ok_r;
}

// TODO: duplicated code
static r validator_on_opcode_fc(void * payload, wasm_opcode_fc opcode, stream imm) {
#ifdef LOG_INFO_ENABLED
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    const char * op_name = get_op_fc_name(opcode);
    u32 local_count = ctx->f->local_count;
    u32 stack_height = (u32)vec_size_type_id(&ctx->val_stack);
    u32 op_offset = (u32)(imm.p - mod->wasm_bin.s.ptr - 1);
    u32 ctrl_frames = (u32)vec_size_ctrl_frame(&ctx->ctrl_stack) - 1;
    LOGI("%08x locals:%-3u stack:%-3u |%*s%s", op_offset, local_count, stack_height, ctrl_frames * 2, "", op_name);
#endif // LOG_INFO_ENABLED
    return ok_r;
}

static r validator_on_trunc_sat(void * payload, wasm_opcode_fc opcode, stream imm) {
    switch (opcode) {
        case op_i32_trunc_sat_f32_s:
        case op_i32_trunc_sat_f32_u:
            POP1_PUSH(f32, i32);
        case op_i32_trunc_sat_f64_s:
        case op_i32_trunc_sat_f64_u:
            POP1_PUSH(f64, i32);
        case op_i64_trunc_sat_f32_s:
        case op_i64_trunc_sat_f32_u:
            POP1_PUSH(f32, i64);
        case op_i64_trunc_sat_f64_s:
        case op_i64_trunc_sat_f64_u:
            POP1_PUSH(f64, i64);
        default:
            check_prep(r);
            assert(false);
            return err(e_general, "Should not happen");
    }
}

static r validator_on_memory_init(void * payload, stream imm, u32 data_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (data_idx >= vec_size_data(&mod->data)) {
        return err(e_invalid, "invalid data index");
    }
    if (!vec_size_memory(&mod->memories)) {
        return err(e_invalid, "mem0 not found");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_memory_copy(void * payload, wasm_opcode_fc opcode, stream imm) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (!vec_size_memory(&mod->memories)) {
        return err(e_invalid, "mem0 not found");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

// same impl
#define validator_on_memory_fill validator_on_memory_copy

static r validator_on_data_drop(void * payload, stream imm, u32 data_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (data_idx >= vec_size_data(&mod->data)) {
        return err(e_invalid, "invalid data index");
    }
    return ok_r;
}

static r validator_on_table_init(void * payload, stream imm, u32 elem_idx, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    table * tab = vec_at_table(&mod->tables, table_idx);
    if (elem_idx >= vec_size_element(&mod->elements)) {
        return err(e_invalid, "invalid element index");
    }
    element * elem = vec_at_element(&mod->elements, elem_idx);
    if (tab->valtype != elem->valtype) {
        return err(e_invalid, "table and element type mismatch");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_elem_drop(void * payload, stream imm, u32 elem_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (elem_idx >= vec_size_element(&mod->elements)) {
        return err(e_invalid, "invalid element index");
    }
    return ok_r;
}

static r validator_on_table_copy(void * payload, stream imm, u32 table_idx_dst, u32 table_idx_src) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx_src >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    table * tab_x = vec_at_table(&mod->tables, table_idx_src);
    if (table_idx_dst >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    table * tab_y = vec_at_table(&mod->tables, table_idx_dst);
    if (tab_x->valtype != tab_y->valtype) {
        return err(e_invalid, "table type mismatch");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_table_grow(void * payload, stream imm, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_ref, ctx));
    check(push_val(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_table_size(void * payload, stream imm, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    check(push_val(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_table_fill(void * payload, stream imm, u32 table_idx) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    module * mod = ctx->mod;
    if (table_idx >= vec_size_table(&mod->tables)) {
        return err(e_invalid, "invalid table index");
    }
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_ref, ctx));
    unwrap_drop(type_id, pop_val_expect(TYPE_ID_i32, ctx));
    return ok_r;
}

static r validator_on_opcode_fd(void * payload, wasm_opcode_fd opcode, stream imm) {
    check_prep(r);
    return err(e_invalid, "prefix_fd opcodes is not supported yet");
}

static r validator_on_decode_end(void * payload) {
    check_prep(r);
    validator_context * ctx = (validator_context *)payload;
    vec_jump_table * jt = &ctx->f->jt;

    LOGI("%s", "validator end");

    // Link the jump table. TODO: optimize it
    check(vec_shrink_to_fit_jump_table(jt));
    for (u32 i = 0; i < vec_size_jump_table(jt); i++) {
        jump_table * slot = vec_at_jump_table(jt, i);
        if (slot->target_offset > 0) {
            for (u32 j = i + 1; j < vec_size_jump_table(jt); j++) {
                jump_table * next_slot = vec_at_jump_table(jt, j);
                if (next_slot->pc > (slot->pc + slot->target_offset)) {
                    slot->next_idx = (u16)j;
                    break;
                } else {
                    continue;
                }
            }
            continue;
        } else {
            for (i32 j = i; j >= 0; j--) {
                jump_table * next_slot = vec_at_jump_table(jt, j);
                if (next_slot->pc > (slot->pc + slot->target_offset)) {
                    slot->next_idx = (u16)j;
                } else {
                    break;
                }
            }
            continue;
        }
    }

#ifdef LOG_INFO_ENABLED
    {
        str code = ctx->f->code;
        module * mod = ctx->mod;
        // dump the jump table
        if (vec_size_jump_table(jt)) {
            LOGI("%s", "Jump table:");
            for (u32 i = 0; i < vec_size_jump_table(jt); i++) {
                jump_table * tbl_item = vec_at_jump_table(jt, i);
                u32 opcode_offset = (u32)(code.ptr - mod->wasm_bin.s.ptr + tbl_item->pc);
                u32 target_offset = opcode_offset + tbl_item->target_offset;
                LOGI("[%u] PC:%06X  TGT:%06X  Arity:%d   StackOffset:%d   Next:%d", i, opcode_offset, target_offset, tbl_item->arity, tbl_item->stack_offset, tbl_item->next_idx);
            }
        }
    }
#endif // LOG_INFO_ENABLED

    // Update the max stack size. Callee's local size included.
    ctx->f->stack_size_max = ctx->stack_size_max;

    // No need to verify the returns here.
    // check out validator_on_end for more info.
    return ok_r;
}

static const op_decoder_callbacks validator_callbacks = {
    .on_decode_begin = validator_on_decode_begin,
    .on_opcode = validator_on_opcode,
    .on_unreachable = validator_on_unreachable,
    .on_nop = NULL,
    .on_block = validator_on_block,
    .on_else = validator_on_else,
    .on_end = validator_on_end,
    .on_br_or_if = validator_on_br_or_if,
    .on_br_table = validator_on_br_table,
    .on_return = validator_on_return,
    .on_call = validator_on_call,
    .on_call_indirect = validator_on_call_indirect,
    .on_drop = validator_on_drop,
    .on_select = validator_on_select,
    .on_select_t = validator_on_select_t,
    .on_local_get = validator_on_local_get,
    .on_local_set = validator_on_local_set,
    .on_local_tee = validator_on_local_tee,
    .on_global_get = validator_on_global_get,
    .on_global_set = validator_on_global_set,
    .on_table_get = validator_on_table_get,
    .on_table_set = validator_on_table_set,
    .on_memory_load_store = validator_on_memory_load_store,
    .on_memory_size = validator_on_memory_size,
    .on_memory_grow = validator_on_memory_grow,
    .on_i32_const = validator_on_i32_const,
    .on_i64_const = validator_on_i64_const,
    .on_f32_const = validator_on_f32_const,
    .on_f64_const = validator_on_f64_const,
    .on_iunop = validator_on_iunop,
    .on_funop = validator_on_funop,
    .on_ibinop = validator_on_ibinop,
    .on_fbinop = validator_on_fbinop,
    .on_itestop = validator_on_itestop,
    .on_irelop = validator_on_irelop,
    .on_frelop = validator_on_frelop,
    .on_wrapop = validator_on_wrapop,
    .on_truncop = validator_on_truncop,
    .on_convertop = validator_on_convertop,
    .on_rankop = validator_on_rankop,
    .on_reinterpretop = validator_on_reinterpretop,
    .on_extendop = validator_on_extendop,
    .on_ref_null = validator_on_ref_null,
    .on_ref_is_null = validator_on_ref_is_null,
    .on_ref_func = validator_on_ref_func,
    .on_opcode_fc = validator_on_opcode_fc,
    .on_trunc_sat = validator_on_trunc_sat,
    .on_memory_init = validator_on_memory_init,
    .on_memory_copy = validator_on_memory_copy,
    .on_memory_fill = validator_on_memory_fill,
    .on_data_drop = validator_on_data_drop,
    .on_table_init = validator_on_table_init,
    .on_elem_drop = validator_on_elem_drop,
    .on_table_copy = validator_on_table_copy,
    .on_table_grow = validator_on_table_grow,
    .on_table_size = validator_on_table_size,
    .on_table_fill = validator_on_table_fill,
    .on_opcode_fd = validator_on_opcode_fd,
    .on_decode_end = validator_on_decode_end,
};

r validate_func(module * mod, func * f) {
    assert(mod);
    assert(f);

    validator_context ctx = {0};
    ctx.mod = mod;
    ctx.f = f;
    r ret = decode_function(mod, f, &validator_callbacks, &ctx);

    validator_context_drop(&ctx);

    return ret;
}

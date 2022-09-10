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

#include "vm.h"

#include "interpreter.h"
#include "list_impl.h"
#include "module.h"
#include "validator.h"
#include "vec_impl.h"

LIST_IMPL_FOR_TYPE(module_inst)
LIST_IMPL_FOR_TYPE(vm)

VEC_IMPL_FOR_TYPE(func_inst)
VEC_IMPL_FOR_TYPE(func_addr)
VEC_IMPL_FOR_TYPE(table_inst)
VEC_IMPL_FOR_TYPE(tab_addr)
VEC_IMPL_FOR_TYPE(memory_inst)
VEC_IMPL_FOR_TYPE(mem_addr)
VEC_IMPL_FOR_TYPE(global_inst)
VEC_IMPL_FOR_TYPE(glob_addr)

// This function must only do simply initializations like resize the vectors and such.
// Any complicated initialization that might involve self-referencing pointers must be
// done after the linking. Therefore, except for the OOM it's unlikely to fail.
static r module_inst_new(module * mod, module_inst * mod_inst) {
    assert(mod);
    assert(mod_inst);
    check_prep(r);

    *mod_inst = (module_inst){0};
    mod_inst->mod = mod;

    // increase the refcount
    mod->ref_count++;

    // we will leave the actual func_inst data filling to the linking stage.
    check(vec_resize_func_inst(&mod_inst->funcs, vec_size_func(&mod->funcs)));
    vec_set_fixed(&mod_inst->funcs, true);
    for (size_t i = 0; i < vec_size_func(&mod->funcs); i++) {
        func_addr f_inst = vec_at_func_inst(&mod_inst->funcs, i);
        func * fn = vec_at_func(&mod->funcs, i);
        f_inst->fn = fn;
        f_inst->mod = mod;
        // This is illegal because it will make the mod_inst object pinned.
        // f_inst->mod_inst = mod_inst;
    }

    // table contents will be initialized with element section.
    check(vec_resize_table_inst(&mod_inst->tables, vec_size_table(&mod->tables)));
    vec_set_fixed(&mod_inst->tables, true);
    for (size_t i = 0; i < vec_size_table(&mod->tables); i++) {
        tab_addr tab_inst = vec_at_table_inst(&mod_inst->tables, i);
        table * tab = vec_at_table(&mod->tables, i);
        tab_inst->tab = tab;
        if (i >= mod->imported_table_count) {
            vec_resize_ref(&tab_inst->tdata, tab->lim.min);
            // nullref is not zero, so it needs to be explicitly initialized.
            VEC_FOR_EACH(&tab_inst->tdata, ref, iter) {
                *iter = nullref;
            }
        }
    }

    // memory will be initialized with data section.
    check(vec_resize_memory_inst(&mod_inst->memories, vec_size_memory(&mod->memories)));
    vec_set_fixed(&mod_inst->memories, true);
    for (size_t i = 0; i < vec_size_memory(&mod->memories); i++) {
        mem_addr mem_inst = vec_at_memory_inst(&mod_inst->memories, i);
        memory * mem = vec_at_memory(&mod->memories, i);
        mem_inst->mem = mem;
        // zero initialize the non-imported modules.
        if (i >= mod->imported_mem_count) {
            vec_resize_u8(&mem_inst->mdata, mem->lim.min * WASM_PAGE_SIZE);
        }
    }
    // glob
    check(vec_resize_global_inst(&mod_inst->globals, vec_size_global(&mod->globals)));
    vec_set_fixed(&mod_inst->globals, true);
    for (size_t i = 0; i < vec_size_global(&mod->globals); i++) {
        glob_addr glob_inst = vec_at_global_inst(&mod_inst->globals, i);
        global * glob = vec_at_global(&mod->globals, i);
        glob_inst->glob = glob;
    }

    check(vec_resize_u8(&mod_inst->dropped_elements, vec_size_element(&mod->elements)));
    vec_set_fixed(&mod_inst->dropped_elements, true);
    check(vec_resize_u8(&mod_inst->dropped_data, vec_size_data(&mod->data)));
    vec_set_fixed(&mod_inst->dropped_data, true);

    // Initialize the tables for linking
    vec_resize_func_addr(&mod_inst->f_addrs, vec_size_func(&mod->funcs));
    vec_set_fixed(&mod_inst->f_addrs, true);
    vec_resize_mem_addr(&mod_inst->m_addrs, vec_size_memory(&mod->memories));
    vec_set_fixed(&mod_inst->m_addrs, true);
    vec_resize_tab_addr(&mod_inst->t_addrs, vec_size_table(&mod->tables));
    vec_set_fixed(&mod_inst->t_addrs, true);
    vec_resize_glob_addr(&mod_inst->g_addrs, vec_size_global(&mod->globals));
    vec_set_fixed(&mod_inst->g_addrs, true);

    return ok_r;
}

static r vm_link_module_inst(vm * vm, module_inst * mod_inst) {
    assert(vm);
    assert(mod_inst);
    check_prep(r);

    if (mod_inst->state != module_inst_uninitialized) {
        return err(e_general, "Invalid module state");
    }
    module * mod = mod_inst->mod;
    // functions
    for (size_t i = 0; i < vec_size_func(&mod->funcs); i++) {
        func_addr f_inst = vec_at_func_inst(&mod_inst->funcs, i);
        func * f = f_inst->fn;
        f_inst->mod_inst = mod_inst;
        if (!is_imported(f->linkage)) {
            *vec_at_func_addr(&mod_inst->f_addrs, i) = f_inst;
            continue;
        }
        module_inst * target_mod_inst = vm_find_module_inst(vm, f->path.module);
        if (!target_mod_inst) {
            return err(e_invalid, "Imports: module instance not found");
        }
        module * target_mod = target_mod_inst->mod;
        assert(target_mod);
        bool linked = false;
        VEC_FOR_EACH(&target_mod->exports, export, exp) {
            if ((exp->external_kind == EXTERNAL_KIND_Function) && (str_eq(exp->name, f->path.field))) {
                func_addr tgt_f_addr = *vec_at_func_addr(&target_mod_inst->f_addrs, exp->external_idx);
                func * tgt_f = tgt_f_addr->fn;
                assert(is_exported(tgt_f->linkage));
                // let's check the function signature
                if (!func_type_eq(f->fn_type, tgt_f->fn_type)) {
                    return err(e_invalid, "Function type mismatch");
                }
                *vec_at_func_addr(&mod_inst->f_addrs, i) = tgt_f_addr;
                linked = true;
                break;
            }
        }
        if (!linked) {
            return err(e_invalid, "Imports: function not found");
        }
    }
    // globals
    for (size_t i = 0; i < vec_size_global(&mod->globals); i++) {
        glob_addr g_inst = vec_at_global_inst(&mod_inst->globals, i);
        global * g = g_inst->glob;
        if (!is_imported(g->linkage)) {
            *vec_at_glob_addr(&mod_inst->g_addrs, i) = g_inst;
            continue;
        }
        module_inst * target_mod_inst = vm_find_module_inst(vm, g->path.module);
        if (!target_mod_inst) {
            return err(e_invalid, "Imports: module instance not found");
        }
        module * target_mod = target_mod_inst->mod;
        assert(target_mod);
        bool linked = false;
        VEC_FOR_EACH(&target_mod->exports, export, exp) {
            if ((exp->external_kind == EXTERNAL_KIND_Global) && (str_eq(exp->name, g->path.field))) {
                glob_addr tgt_g_addr = *vec_at_glob_addr(&target_mod_inst->g_addrs, exp->external_idx);
                global * tgt_g = tgt_g_addr->glob;
                assert(tgt_g);
                assert(is_exported(tgt_g->linkage));
                if (tgt_g->mut != g->mut) {
                    return err(e_invalid, "Imported global value's mut flag doesn't match the target");
                }
                if (tgt_g->valtype != g->valtype) {
                    return err(e_invalid, "Imported global value value type mismatch");
                }
                *vec_at_glob_addr(&mod_inst->g_addrs, i) = tgt_g_addr;
                linked = true;
                break;
            }
        }
        if (!linked) {
            return err(e_invalid, "Imports: global not found");
        }
    }
    // mem
    for (size_t i = 0; i < vec_size_memory(&mod->memories); i++) {
        mem_addr mem_inst = vec_at_memory_inst(&mod_inst->memories, i);
        memory * mem = mem_inst->mem;
        if (!is_imported(mem->linkage)) {
            *vec_at_mem_addr(&mod_inst->m_addrs, i) = mem_inst;
            continue;
        }
        module_inst * target_mod_inst = vm_find_module_inst(vm, mem->path.module);
        if (!target_mod_inst) {
            return err(e_invalid, "Imports: module instance not found");
        }
        module * target_mod = target_mod_inst->mod;
        assert(target_mod);
        bool linked = false;
        VEC_FOR_EACH(&target_mod->exports, export, exp) {
            if ((exp->external_kind == EXTERNAL_KIND_Memory) && (str_eq(exp->name, mem->path.field))) {
                mem_addr tgt_m_addr = *vec_at_mem_addr(&target_mod_inst->m_addrs, exp->external_idx);
                memory * tgt_m = tgt_m_addr->mem;
                assert(tgt_m);
                assert(is_exported(tgt_m->linkage));
                assert(!(vec_size_u8(&tgt_m_addr->mdata) % WASM_PAGE_SIZE));
                size_t tgt_curr_pages = vec_size_u8(&tgt_m_addr->mdata) / WASM_PAGE_SIZE;
                if (mem->lim.min > tgt_curr_pages) {
                    return err(e_invalid, "mem import's min length should be at most the imported mem's min length");
                }
                if (mem->lim.max < tgt_m->lim.max) {
                    return err(e_invalid, "mem import's max length should be at least the imported mem's max length");
                }
                // so, if the target_m_inst is also imported, we will follow the link to the final destination.
                *vec_at_mem_addr(&mod_inst->m_addrs, i) = tgt_m_addr;
                linked = true;
                break;
            }
        }
        if (!linked) {
            return err(e_invalid, "Imports: memory not found");
        }
    }
    // table
    for (size_t i = 0; i < vec_size_table(&mod->tables); i++) {
        tab_addr tab_inst = vec_at_table_inst(&mod_inst->tables, i);
        table * tab = tab_inst->tab;
        if (!is_imported(tab->linkage)) {
            *vec_at_tab_addr(&mod_inst->t_addrs, i) = tab_inst;
            continue;
        }
        module_inst * target_mod_inst = vm_find_module_inst(vm, tab->path.module);
        if (!target_mod_inst) {
            return err(e_invalid, "Imports: module instance not found");
        }
        module * target_mod = target_mod_inst->mod;
        assert(target_mod);
        bool linked = false;
        VEC_FOR_EACH(&target_mod->exports, export, exp) {
            if ((exp->external_kind == EXTERNAL_KIND_Table) && (str_eq(exp->name, tab->path.field))) {
                tab_addr tgt_t_addr = *vec_at_tab_addr(&target_mod_inst->t_addrs, exp->external_idx);
                table * tgt_t = tgt_t_addr->tab;
                assert(is_exported(tgt_t->linkage));
                if (tab->valtype != tgt_t->valtype) {
                    return err(e_invalid, "table type mismatch");
                }
                if (tab->lim.min > tgt_t->lim.min) {
                    return err(e_invalid, "table import's min length should be at most the imported table's min length");
                }
                if (tab->lim.max < tgt_t->lim.max) {
                    return err(e_invalid, "table import's max length should be at least the imported table's max length");
                }
                *vec_at_tab_addr(&mod_inst->t_addrs, i) = tgt_t_addr;
                linked = true;
                break;
            }
        }
        if (!linked) {
            return err(e_invalid, "Imports: table not found");
        }
    }
    return ok_r;
}

static r module_inst_init_global(module_inst * mod_inst) {
    assert(mod_inst);
    check_prep(r);

    module * mod = mod_inst->mod;
    assert(mod);

    assert(mod->imported_global_count <= vec_size_global(&mod->globals));
    for (u32 i = mod->imported_global_count; i < vec_size_global(&mod->globals); i++) {
        global_inst * g_inst = vec_at_global_inst(&mod_inst->globals, i);
        global * g = vec_at_global(&mod->globals, i);
        unwrap(typed_value, val, interp_reduce_const_expr(mod_inst, stream_from(g->expr), false));
        // here the reference should be already converted from function indexes to addresses
        if (g->valtype != val.type) {
            return err(e_invalid, "Incorrect global constexpr return type");
        }
        g_inst->gvalue = val.val;
    }
    return ok_r;
}

static r module_inst_init_elem(module_inst * mod_inst) {
    assert(mod_inst);
    check_prep(r);

    module * mod = mod_inst->mod;
    assert(mod);

    for (u32 i = 0; i < vec_size_element(&mod->elements); i++) {
        element * elem = vec_at_element(&mod->elements, i);
        if (elem->mode == elem_mode_active) {
            u32 table_idx = elem->tableidx;
            if (table_idx >= vec_size_tab_addr(&mod_inst->t_addrs)) {
                return err(e_invalid, "Element without matching table instance");
            }
            tab_addr t_addr = *vec_at_tab_addr(&mod_inst->t_addrs, table_idx);
            assert(t_addr);
            if (table_idx >= vec_size_table(&mod->tables)) {
                return err(e_invalid, "Invalid table index");
            }
            u32 offset = 0;
            if (!str_is_null(elem->offset_expr)) {
                unwrap(typed_value, val, interp_reduce_const_expr(mod_inst, stream_from(elem->offset_expr), false));
                if (val.type != TYPE_ID_i32) {
                    return err(e_invalid, "Incorrect element constexpr return type");
                }
                offset = (u32)(val.val.u_i32);
            }
            if (offset + elem->data_len > vec_size_ref(&t_addr->tdata)) {
                return err(e_invalid, "Invalid offset");
            }
            // convert the function indexes in the v_funcidx to references
            if (vec_size_u32(&elem->v_funcidx)) {
                assert(elem->data_len == vec_size_u32(&elem->v_funcidx));
                for (u32 j = 0; j < elem->data_len; j++) {
                    u32 f_idx = *vec_at_u32(&elem->v_funcidx, j);
                    ref fref;
                    if (f_idx != nullfidx) {
                        // here we convert the index into the actual func_addr, if not null.
                        fref = to_ref(*vec_at_func_addr(&mod_inst->f_addrs, f_idx));
                    } else {
                        fref = nullref;
                    }
                    *vec_at_ref(&t_addr->tdata, offset + j) = fref;
                }
            // calculate and convert the function indexes in the v_expr to references
            } else if (vec_size_str(&elem->v_expr)) {
                assert(elem->data_len == vec_size_str(&elem->v_expr));
                for (u32 j = 0; j < elem->data_len; j++) {
                    str expr = *vec_at_str(&elem->v_expr, j);
                    unwrap(typed_value, val, interp_reduce_const_expr(mod_inst, stream_from(expr), false));
                    if (!is_ref(val.type)) {
                        return err(e_invalid, "Incorrect element constexpr return type");
                    }
                    *vec_at_ref(&t_addr->tdata, offset + j) = val.val.u_ref;
                }
            } else if (elem->data_len) {
                // both v_funcidx and v_expr are empty. should not happen.
                assert(0);
            }
        }
        // after the active element initialization, we need to mark it as dropped.
        if (elem->mode != elem_mode_passive) {
            *vec_at_u8(&mod_inst->dropped_elements, i) = true;
        }
    }
    return ok_r;
}

static r module_inst_init_data(module_inst * mod_inst) {
    assert(mod_inst);
    check_prep(r);

    module * mod = mod_inst->mod;
    assert(mod);

    for (u32 i = 0; i < vec_size_data(&mod->data); i++) {
        data * d = vec_at_data(&mod->data, i);
        if (!d->is_active) {
            continue;
        }
        if (d->memidx >= vec_size_memory_inst(&mod_inst->memories)) {
            return err(e_invalid, "Missing memory section");
        }
        // The "inst" should be pointing to the linked target memory instance.
        mem_addr m_addr = *vec_at_mem_addr(&mod_inst->m_addrs, d->memidx);
        unwrap(typed_value, val, interp_reduce_const_expr(mod_inst, stream_from(d->offset_expr), false));
        if (val.type != TYPE_ID_i32) {
            return err(e_invalid, "Incorrect data constexpr return type");
        }
        u32 offset = (u32)(val.val.u_i32);
        if (offset + str_len(d->bytes) > vec_size_u8(&m_addr->mdata)) {
            return err(e_invalid, "Invalid data section");
        }
        if (str_len(d->bytes)) {
            assert(m_addr->mdata._data);
            memcpy(vec_at_u8(&m_addr->mdata, offset), d->bytes.ptr, str_len(d->bytes));
        }
        *vec_at_u8(&mod_inst->dropped_data, i) = true;
    }
    return ok_r;
}

static r module_inst_init(module_inst * mod_inst) {
    assert(mod_inst);
    check_prep(r);

    check(module_inst_init_global(mod_inst));
    // based on the spec the element and data sections needs to be copied
    // in order.
    check(module_inst_init_elem(mod_inst));
    check(module_inst_init_data(mod_inst));

    // TODO: call the module.start if not empty
    option_u32 o_start_idx = mod_inst->mod->start_funcidx;
    if (is_some(o_start_idx)) {
        // Use a new thread for this job because we don't want to mess with
        // VM's main thread state.
        thread t = {0};
        func_addr f_addr = *vec_at_func_addr(&mod_inst->f_addrs, (u32)value(o_start_idx));
        vec_typed_value argv = {0};
        check(interp_call_in_thread(&t, f_addr, argv), thread_reset(&t));
        thread_reset(&t);
    }

    return ok_r;
}

static void module_inst_drop(module_inst * mod_inst) {
    assert(mod_inst);
    assert(mod_inst->mod);

    assert(mod_inst->mod->ref_count);
    mod_inst->mod->ref_count--;

    // func
    vec_clear_func_inst(&mod_inst->funcs);
    // table
    VEC_FOR_EACH(&mod_inst->tables, table_inst, tab_inst) {
        vec_clear_ref(&tab_inst->tdata);
    }
    vec_clear_table_inst(&mod_inst->tables);
    // memory
    VEC_FOR_EACH(&mod_inst->memories, memory_inst, mem_inst) {
        vec_clear_u8(&mem_inst->mdata);
    }
    vec_clear_memory_inst(&mod_inst->memories);
    vec_clear_mem_addr(&mod_inst->m_addrs);
    // global
    vec_clear_global_inst(&mod_inst->globals);
    // dropped elem and data
    vec_clear_u8(&mod_inst->dropped_elements);
    vec_clear_u8(&mod_inst->dropped_data);
    // drop the address table
    vec_clear_func_addr(&mod_inst->f_addrs);
    vec_clear_mem_addr(&mod_inst->m_addrs);
    vec_clear_tab_addr(&mod_inst->t_addrs);
    vec_clear_glob_addr(&mod_inst->g_addrs);
}

r vm_instantiate_module(vm * vm, module * mod) {
    assert(vm);
    assert(mod);
    check_prep(r);

    // if it's already instantiated, return it.
    LIST_FOR_EACH(&vm->instances, module_inst, mod_inst) {
        if (mod_inst->mod == mod) {
            return ok_r;
        }
    }
    // instantiate according to the https://webassembly.github.io/spec/core/exec/modules.html

    // 1: validate the module
    VEC_FOR_EACH(&mod->funcs, func, fn) {
        if (fn->linkage & linkage_imported) {
            continue;
        }
        check(validate_func(mod, fn));
    }

    // 2: create module instances and allocate internal storage
    module_inst mod_ins = {0};
    check(module_inst_new(mod, &mod_ins), {
        module_inst_drop(&mod_ins);
    });
    mod_ins.state = module_inst_uninitialized;

    // push the mod_inst to the list.
    // we need to push it into the list before linking because after that the
    // module instance will be pinned (having self-pointer pointers)
    check(list_push_module_inst(&vm->instances, mod_ins), {
        module_inst_drop(&mod_ins);
    });
    module_inst * new_mod_ins = list_back_module_inst(&vm->instances);
    assert(new_mod_ins);

    // 3: link the module instance
    check(vm_link_module_inst(vm, new_mod_ins), {
        // Keep the half-instantiated module instance in the vm
        new_mod_ins->state = module_inst_init_failed;
        // module_inst_drop(new_mod_ins);
        // list_pop_module_inst(&vm->instances);
    });

    // 4: initialize the globals/elements/etc and call the module.start if not empty
    check(module_inst_init(&mod_ins), {
        new_mod_ins->state = module_inst_init_failed;
    });

    new_mod_ins->state = module_inst_ready;
    return ok_r;
}

void thread_reset(thread * t) {
    assert(t);
    vec_clear_typed_value(&t->results);
    *t = (thread){0};
}

void vm_drop(vm * vm) {
    LIST_FOR_EACH(&vm->instances, module_inst, mod_inst) {
        assert(mod_inst->mod->ref_count);
        module_inst_drop(mod_inst);
    }
    list_clear_module_inst(&vm->instances);
    thread_reset(&vm->thread);
}

module_inst * vm_find_module_inst(vm * vm, str name) {
    assert(vm);

    module_inst * inst = NULL;
    LIST_FOR_EACH(&vm->instances, module_inst, mod_inst) {
        VEC_FOR_EACH(&mod_inst->mod->names, vstr, n) {
            if (str_eq(n->s, name)) {
                inst = mod_inst;
                break;
            }
        }
    }
    return inst;
}

static bool vm_owns_mod_inst(vm * vm, module_inst * mod_inst) {
    assert(vm);
    assert(mod_inst);

    // make sure the mod instance is owned by the vm
    LIST_FOR_EACH(&vm->instances, module_inst, iter) {
        if (mod_inst == iter) {
            return true;
        }
    }
    return false;
}

func_addr vm_find_module_func(vm * vm, module_inst * mod_inst, str func_name) {
    assert(vm);
    assert(mod_inst);

    if (!vm_owns_mod_inst(vm, mod_inst) || mod_inst->state != module_inst_ready) {
        return NULL;
    }
    module * mod = mod_inst->mod;
    VEC_FOR_EACH(&mod->exports, export, exp) {
        if (str_eq(func_name, exp->name) && (exp->external_kind == EXTERNAL_KIND_Function)) {
            size_t func_idx = exp->external_idx;
            return *vec_at_func_addr(&mod_inst->f_addrs, func_idx);
        }
    }
    return NULL;
}

glob_addr vm_find_module_global(vm * vm, module_inst * mod_inst, str glob_name) {
    assert(vm);
    assert(mod_inst);

    if (!vm_owns_mod_inst(vm, mod_inst)) {
        return NULL;
    }
    module * mod = mod_inst->mod;
    VEC_FOR_EACH(&mod->exports, export, exp) {
        if (str_eq(glob_name, exp->name) && (exp->external_kind == EXTERNAL_KIND_Global)) {
            size_t glob_idx = exp->external_idx;
            return *vec_at_glob_addr(&mod_inst->g_addrs, glob_idx);
        }
    }
    return NULL;
}

func_addr vm_find_func(vm * vm, str mod_name, str func_name) {
    assert(vm);

    module_inst * mod_inst = vm_find_module_inst(vm, mod_name);
    if (mod_inst) {
        return vm_find_module_func(vm, mod_inst, func_name);
    }
    return NULL;
}

thread * vm_get_thread(vm * vm) {
    assert(vm);
    return &vm->thread;
}

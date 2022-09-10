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
#include "module.h"
#include "stream.h"
#include "vec.h"
#include "wasm_format.h"

// func
struct module_inst;
typedef struct func_inst {
    module * mod;
    struct module_inst * mod_inst;
    func * fn;
} func_inst;
VEC_DECL_FOR_TYPE(func_inst)

typedef func_inst * func_addr;
VEC_DECL_FOR_TYPE(func_addr)

// converting back and forth between func_addr and ref
#define to_func_addr(x) ((func_addr)(x))
#define to_ref(x) ((ref)(x))

// table
typedef struct table_inst {
    table * tab;
    vec_ref tdata;
} table_inst;
VEC_DECL_FOR_TYPE(table_inst)

typedef table_inst * tab_addr;
VEC_DECL_FOR_TYPE(tab_addr)

// mem
typedef struct memory_inst {
    // mem pointer will always point to the memory struct in the same module.
    memory * mem;
    vec_u8 mdata;
} memory_inst;
VEC_DECL_FOR_TYPE(memory_inst)

typedef memory_inst * mem_addr;
VEC_DECL_FOR_TYPE(mem_addr)

// global
typedef struct global_inst {
    global * glob;
    value_u gvalue;
} global_inst;
VEC_DECL_FOR_TYPE(global_inst)

typedef global_inst * glob_addr;
VEC_DECL_FOR_TYPE(glob_addr)

typedef enum module_inst_state {
    module_inst_uninitialized = 0,
    // instantiation failed.
    module_inst_init_failed,
    // linked, elem,data etc initialized, start called without trap.
    module_inst_ready,
} module_inst_state;
// store, according to the spec we should have a unified storage for all instances
// and having address tables in the instances that points to the store. However, at
// the moment it's more efficient to separate the store into each instance to avoid
// an additional pointer dereference.
typedef struct module_inst {
    module_inst_state state;
    module * mod;
    vec_func_inst funcs;
    vec_table_inst tables;
    vec_u8 dropped_elements;
    vec_memory_inst memories;
    vec_u8 dropped_data;
    vec_global_inst globals;
    // linked tables.
    vec_func_addr f_addrs;
    vec_mem_addr m_addrs;
    vec_tab_addr t_addrs;
    vec_glob_addr g_addrs;
} module_inst;
LIST_DECL_FOR_TYPE(module_inst)
RESULT_TYPE_DECL(module_inst)

// TODO: aligned 64bit stack is kinda wasteful.
typedef struct thread {
    bool trapped;
    u32 frame_depth;
    u32 stack_size;
    // saved return value from the last run. It will be cleared out in the next function call.
    vec_typed_value results;
} thread;

struct runtime;
// In wasm term, it's the 'configuration' which consists of the store and thread.
// vm can only be initialized by a runtime with it's internal modules. So there's
// no standalone vm_init.
typedef struct vm {
    // the main instance is always the first element.
    list_module_inst instances;
    thread thread;
    // point back to its owner
    struct runtime * rt;
} vm;
LIST_DECL_FOR_TYPE(vm)

typedef vm * vm_ptr;
RESULT_TYPE_DECL(vm_ptr)

// instantiate a module in the vm.
// According to the spec, a half-initialized instance stays in the vm
// so that the changes it has already done will be valid. For instance the active
// element init can put function reference to other module instance's tables. The
// function reference should be valid even if the instance failed to init.
// That's said, if a module can't even pass a validation, it will simply never be
// instantiated.
r vm_instantiate_module(vm * vm, module * mod);

// find an instantiated module instance in the vm.
module_inst * vm_find_module_inst(vm * vm, str name);

// find a exported function instance inside a giving vm and module instance.
// The module instance must be owned by the vm, and it also must be in the ready state.
// We disallow running anything inside a half-instantiated module by applying the
// restriction here.
func_addr vm_find_module_func(vm * vm, module_inst * mod_inst, str func_name);

// find an exported glob instance
// Unlike function, fetching global variables from a half-instantiated module is allowed.
glob_addr vm_find_module_global(vm * vm, module_inst * mod_inst, str glob_name);

// A helper function to find a function directly by name.
func_addr vm_find_func(vm * vm, str mod_name, str func_name);

thread * vm_get_thread(vm * vm);

// Once a thread enters a trapped state, all furthur reducing attempts will fail until it's reset.
void thread_reset(thread * t);

// drop all the internal store and stacks and then clear the vm
void vm_drop(vm * vm);

// stack helpers
INLINE value_u stack_pop(vec_value_u * stack) {
    assert(stack);
    value_u i = *vec_back_value_u(stack);
    vec_pop_value_u(stack);
    return i;
}

INLINE r stack_push(vec_value_u * stack, value_u val) {
    assert(stack);
    check_prep(r);
    check(vec_push_value_u(stack, val));
    return ok_r;
}




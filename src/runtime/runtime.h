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

#include "vm.h"
#include "module.h"

// runtime, includes all immutable modules and vm instances.
// runtime -> { module_1, module_2, ... vm_1, vm_2, ...}
//      vm -> { store, thread }
//              store -> { instance_1, instance_2, ... }
//              thread -> { frame stack : frame,frame,frame...
//                          value stack : args, locals, args, locals, ...}

typedef struct runtime {
    list_module modules;
    list_vm vms;
} runtime;

// delete all vm and modules and clear the runtime.
r runtime_drop(runtime * rt);

// add a parsed module instance. Mainly used on host modules.
r runtime_module_add_mod(runtime * rt, module mod);

// add a wasm binary and parse into a module directly
r runtime_module_add(runtime * rt, vstr bin, vstr name);

// drop a runtime owned module
r runtime_module_drop(runtime * rt, module * mod);

// find a module by name
module * runtime_module_find(runtime * rt, str name);

// create an empty vm from the main module.
r_vm_ptr runtime_vm_new(runtime * rt);

// drop a runtime owned vm instance.
r runtime_vm_drop(runtime * rt, vm * v);

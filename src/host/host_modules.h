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
#include "str.h"
#include "trampoline.h"
#include "vm.h"

typedef struct host_func_info {
    char * name;
    trampoline tr;
    void * host_func;
} host_func_info;

// native module instances works just like normal module instances where
// exporting functions/memories/globals/tables are all allowed.
// Except for the functions, everything else in the native modules are handles
// the same way as normal modules.

r_module host_mod_new(str name);

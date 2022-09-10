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

#include "result.h"
#include "vm.h"

// Call a function in a thread. The state (including the return values) will be recorded in the thread.
// This function will take the ownership of the argv.
r interp_call_in_thread(thread * t, func_addr f_addr, vec_typed_value argv);

// Note: passive element section allows ref.null and ref.func only.
r_typed_value interp_reduce_const_expr(module_inst * mod_inst, stream code, bool passive_elem);

r in_place_dt_call(thread * t, func_addr f_addr, value_u * args);

r in_place_tco_call(thread * t, func_addr f_addr, value_u * args);



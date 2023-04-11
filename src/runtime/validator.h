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

// It's mostly following the algorithm described at
// https://webassembly.github.io/spec/core/appendix/algorithm.html
// to ensure the soundness

#include "module.h"
#include "result.h"
#include "types.h"
#include "vec.h"

typedef enum blocktype {
    bt_func,
    bt_block,
    bt_loop,
    bt_if,
    bt_else,
} blocktype;

typedef struct ctrl_frame {
    blocktype type;
    func_type ftype;
    vec_type_id start_types;
    vec_type_id end_types;
    size_t height;
    bool unreachable;
    // used by constructing the jump table
    vec_u32 pending_jt_slots;
    u32 pc;
} ctrl_frame;
RESULT_TYPE_DECL(ctrl_frame)

r validate_func(module * mod, func * f);

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

#include "types.h"
#include "vec.h"
#include "wasm_format.h"

// TODO: move it to platform-specific files.
#define GPR_COUNT 6
#define FPR_COUNT 4
// minimum number of stack values that are mapped to registers
#define REG_AS_STACK_MIN 2

#define INVALID_IDX_U16 u16_MAX

typedef enum reg_target {
    tgt_local,
    tgt_stack,
} reg_target;

typedef enum reg_type {
    GPR,
    FPR,
} reg_type;

typedef struct local_slot {
    reg_type type;
    u16 reg_idx; // INVALID_IDX_U16 for invalid slot
    u16 alias_count;
} local_slot;
VEC_DECL_FOR_TYPE(local_slot)

typedef struct stack_slot {
    bool aliased;
    union {
        // aliased == false, mapped to a register
        u16 reg_idx; // INVALID_IDX_U16 for invalid slot
        // aliased == true, pointing to a local
        u16 local_idx; // INVALID_IDX_U16 for invalid slot
    } u;
} stack_slot;
VEC_DECL_FOR_TYPE(stack_slot)

typedef struct reg_state {
    reg_type type;
    reg_target target;
    u16 tgt_idx; // INVALID_IDX_U16 for invalid slot
    bool pinned; // immune to spill
} reg_state;
VEC_DECL_FOR_TYPE(reg_state)


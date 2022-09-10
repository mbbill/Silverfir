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
#include "types.h"
#include "vec.h"
#include "wasm_format.h"

struct func_inst;
struct memory_inst;
typedef struct tr_ctx {
    struct func_inst * f_addr;
    value_u * args;
    struct memory_inst * mem0;
} tr_ctx;

typedef r (*trampoline)(tr_ctx, void * f);


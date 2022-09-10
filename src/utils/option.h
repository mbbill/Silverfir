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

#include "silverfir.h"
#include "types.h"

#define OPTION_TYPE_DECL_BUILTIN(type, _) \
    typedef struct option_##type {        \
        u8 tag;                           \
        type value;                       \
    } option_##type;

FOR_EACH_TYPE(OPTION_TYPE_DECL_BUILTIN, _)

#define OPTION_TYPE_DECL(type) OPTION_TYPE_DECL_BUILTIN(type, _)

#define some(type, v) \
    (type) {          \
        .tag = 1,     \
        .value = v,   \
    }

#define none(type) \
    (type) {       \
        .tag = 0   \
    }

#define is_some(v) (v.tag)
#define value(v) (v.value)

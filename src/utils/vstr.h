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

// A vector based string.
// The string can have three different states:
// - stored in a static buffer.
// - stored in heap (vector)
// In both cases it can be accessed through the vstr.s member.

// The vstr can only have one owner. It can be copy-passed around, and the owner is
// responsible for releasing the vstr. No other copies should be accessed after one
// of the copy is released. Therefore, you need to have a design that can make sure
// of the memory assumptions.

// The internal storage might change but that's transparent to the user. For instance
// when a static vstr get's modified, its contents will be copied into the vector,
// and its internal 'vstr.s' will point to the new location.

#pragma once

#include "result.h"
#include "silverfir.h"
#include "str.h"
#include "vec.h"

typedef struct vstr {
    str s;
    vec_u8 v;
} vstr;
RESULT_TYPE_DECL(vstr)
VEC_DECL_FOR_TYPE(vstr)

// A default zero initialized vstr is a valid null vstr instance
#define VSTR_NULL ((vstr){0})

#define vstr_is_null(vs) (str_is_null(vs.s))

// vstr from literal, like vs("abc")
#define vs(p) ((vstr){.s = s(p)})

// vstr from pointer
#define vs_p(p) ((vstr){.s = s_p(p)})

// vs from p(pointer) + l(length)
#define vs_pl(p, l) ((vstr){.s = s_pl(p, l)})

void vstr_drop(vstr * vs);

r vstr_append(vstr * vs, str s);

r vstr_append_c(vstr * vs, u8 c);

r_vstr vstr_dup(str s);

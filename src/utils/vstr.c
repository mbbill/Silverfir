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

#include "vstr.h"

#include "vec_impl.h"

VEC_IMPL_FOR_TYPE(vstr)

void vstr_drop(vstr * vs) {
    assert(vs);
    vec_clear_u8(&vs->v);
    *vs = VSTR_NULL;
}

r vstr_append(vstr * vs, str s) {
    assert(vs);
    check_prep(r);
    if (str_is_null(s)) {
        return ok_r;
    }
    check(vec_resize_u8(&vs->v, vs->s.len + s.len));
    if (!vs->v._size) {
        memcpy(vs->v._data, vs->s.ptr, vs->s.len);
    }
    memcpy(vs->v._data + vs->s.len, s.ptr, s.len);
    vs->s.len = vs->s.len + s.len;
    vs->s.ptr = vs->v._data;
    return ok_r;
}

r vstr_append_c(vstr * vs, u8 c) {
    check_prep(r);
    check(vec_push_u8(&vs->v, c));
    vs->s.len += 1;
    vs->s.ptr = vs->v._data;
    return ok_r;
}

r_vstr vstr_dup(str s) {
    check_prep(r_vstr);
    vstr new_vs = {0};
    if (str_is_null(s)) {
        return ok(new_vs);
    }
    check(vec_resize_u8(&new_vs.v, s.len));
    memcpy(new_vs.v._data, s.ptr, s.len);
    new_vs.s.len = s.len;
    new_vs.s.ptr = new_vs.v._data;
    return ok(new_vs);
}

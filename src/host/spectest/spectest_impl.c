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

#include "compiler.h"
#include "host_modules.h"
#include "spectest_gen.h"
#include "types.h"

r spectest_print(tr_ctx ctx) {
    return ok_r;
}

r spectest_print_i32(tr_ctx ctx, i32 i1) {
    return ok_r;
}

r spectest_print_i64(tr_ctx ctx, i64 I1) {
    return ok_r;
}

r spectest_print_f32(tr_ctx ctx, f32 f1) {
    return ok_r;
}

r spectest_print_f64(tr_ctx ctx, f64 F1) {
    return ok_r;
}

r spectest_print_i32_f32(tr_ctx ctx, i32 i1, f32 f2) {
    return ok_r;
}

r spectest_print_f64_f64(tr_ctx ctx, f64 F1, f64 F2) {
    return ok_r;
}

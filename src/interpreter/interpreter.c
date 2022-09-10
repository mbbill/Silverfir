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

// It's actually a hybrid interpreter with both DT and TCO. It uses the
// direct threading table and doesn't compile, so it's still an in-place
// interpreter, yet it also leverages TCO for further performance improvements.

#include "silverfir.h"
#include "interpreter.h"

r interp_call_in_thread(thread * t, func_addr f_addr, vec_typed_value argv) {
    check_prep(r);

    if (!is_little_endian()) {
        vec_clear_typed_value(&argv);
        // TODO: memory load and store instructions currently doesn't support big-endian.
        return err(e_general, "Big endian is currently unsupported");
    }

    if (t->trapped) {
        vec_clear_typed_value(&argv);
        return err(e_general, "Thread is in trapped state");
    }

    // check the arguments
    func_type ft = f_addr->fn->fn_type;
    if (ft.param_count != vec_size_typed_value(&argv)) {
        vec_clear_typed_value(&argv);
        return err(e_general, "Incorrect amount of args");
    }
    stream st = stream_from(ft.params);
    for (u32 i = 0; i < ft.param_count; i++) {
        unwrap(i8, type, stream_read_vi7(&st));
        if (vec_at_typed_value(&argv, i)->type != type) {
            vec_clear_typed_value(&argv);
            return err(e_general, "Argument type mismatch");
        }
    }

    // copy the arguments to the stack (local)
    u32 stack_size_needed = f_addr->fn->local_count > ft.result_count ? f_addr->fn->local_count : ft.result_count;
    value_u * stack_base = array_alloca(value_u, stack_size_needed);
    if (!stack_base) {
        vec_clear_typed_value(&argv);
        return err(e_general, "Stack overflow!");
    }
    for (u32 i = 0; i < vec_size_typed_value(&argv); i++) {
        stack_base[i] = vec_at_typed_value(&argv, i)->val;
    }

// Use DT first
#if SILVERFIR_INTERP_INPLACE_DT
    check(in_place_dt_call(t, f_addr, stack_base), {
        t->trapped = true;
        vec_clear_typed_value(&argv);
    });
#elif SILVERFIR_INTERP_INPLACE_TCO
    check(in_place_tco_call(t, f_addr, stack_base), {
        t->trapped = true;
        vec_clear_typed_value(&argv);
    });
#endif

    vec_clear_typed_value(&argv);

    // prepare the typed return values
    vec_clear_typed_value(&t->results);
    st = stream_from(ft.results);
    for (u32 i = 0; i < ft.result_count; i++) {
        unwrap(i8, type, stream_read_vi7(&st));
        check(vec_push_typed_value(&t->results, (typed_value){
                                                    .type = type,
                                                    .val = stack_base[i],
                                                }));
    }

    return ok_r;
}

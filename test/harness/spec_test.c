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

#include "spec_test.h"

#include "host_modules.h"
#include "interpreter.h"
#include "logger.h"
#include "runtime.h"
#include "smath.h"
#include "vstr.h"

#include <assert.h>
#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(fmt, ...) LOG_INFO(log_channel_test, fmt, ##__VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_test, fmt, ##__VA_ARGS__)

typedef struct runner_ctx {
    bool verbose;
    runtime rt;
    vm * current_vm;
    vstr current_module_name;
    module * last_module;
    vec_typed_value returned_values;
    vstr field;
} runner_ctx;

static void runner_ctx_drop(runner_ctx * ctx) {
    runtime_drop(&ctx->rt);
    vstr_drop(&ctx->current_module_name);
    vec_clear_typed_value(&ctx->returned_values);
    vstr_drop(&ctx->field);
}

typedef enum load_results {
    load_ok,
    assert_invalid,
    assert_malformed,
    assert_uninstantiable,
    assert_unlinkable,
} load_results;

static r register_module_as(runner_ctx * ctx, str module_name, str as_name) {
    assert(!str_is_null(as_name));
    check_prep(r);
    module * m;

    if (str_is_null(module_name)) {
        m = ctx->last_module;
    } else {
        m = runtime_module_find(&ctx->rt, module_name);
        if (!m) {
            return err(e_general, "[spectest] module not found");
        }
    }
    unwrap(vstr, newname, vstr_dup(as_name));
    check(module_register_name(m, newname));
    return ok_r;
}

static r asserted_load(runner_ctx * ctx, json_string filename, str dir, str module_name, load_results expect) {
    assert(!str_is_null(filename));
    check_prep(r);
    r ret = {0};
    FILE * fp = NULL;
    u8 * wasm_bin = NULL;

    vstr full_name = {0};
    if (!str_is_null(dir)) {
        check(vstr_append(&full_name, dir));
#ifdef _WIN32
        check(vstr_append(&full_name, s("\\")));
#else
        check(vstr_append(&full_name, s("/")));
#endif
    }
    check(vstr_append(&full_name, filename), {
        vstr_drop(&full_name);
    });
    check(vstr_append(&full_name, STR_ZERO_END), {
        vstr_drop(&full_name);
    });

    r_vstr name = {0};
    if (!str_is_null(module_name)) {
        name = vstr_dup(module_name);
    } else {
        name = vstr_dup(filename);
    }
    assert(is_ok(name));

    fp = fopen((const char *)full_name.s.ptr, "rb");
    vstr_drop(&full_name);
    if (!fp) {
        ret = err(e_general, "[spectest] Can't open wasm binary file");
        goto end;
    }
    r_vstr bin = {0};
    fseek(fp, 0, SEEK_END);
    size_t size = ftell(fp);
    if (size) {
        rewind(fp);
        wasm_bin = array_alloc(u8, size);
        if (!wasm_bin) {
            ret = err(e_general, "[spectest] OOM");
            goto end;
        }
        size_t read_size = fread(wasm_bin, sizeof(u8), size, fp);
        if (!read_size) {
            ret = err(e_general, "[spectest] fread failed");
            goto end;
        }
        bin = vstr_dup(s_pl(wasm_bin, read_size));
        assert(is_ok(bin));
        array_free(wasm_bin);
        wasm_bin = NULL;
    }
    ret = runtime_module_add(&ctx->rt, bin.value, name.value);
    if (!is_ok(ret)) {
        goto check_load_result;
    }
    if (!ctx->current_vm) {
        r_vm_ptr pvm = runtime_vm_new(&ctx->rt);
        if (!is_ok(pvm)) {
            goto check_load_result;
        }
        ctx->current_vm = pvm.value;
        module * spectest_mod = runtime_module_find(&ctx->rt, s("spectest"));
        if (!spectest_mod) {
            ret = err(e_general, "spectest module not installed");
        }
        ret = vm_instantiate_module(ctx->current_vm, spectest_mod);
        if (!is_ok(ret)) {
            goto end;
        }
    }
    module * m = runtime_module_find(&ctx->rt, name.value.s);
    if (!m) {
        ret = err(e_general, "[spectest] Can't find the module");
        goto end;
    }
    ret = vm_instantiate_module(ctx->current_vm, m);
    if ((expect == assert_uninstantiable) || (expect == assert_unlinkable)) {
        if (!is_ok(ret)) {
            ret = ok_r;
            goto end;
        } else {
            ret = err(e_general, "[spectest] vm should not be instantiable");
            goto end;
        }
    }
    if (!is_ok(ret)) {
        goto check_load_result;
    }
    ctx->last_module = m;
    r_vstr new_name = vstr_dup(name.value.s);
    if (!is_ok(new_name)) {
        ret = to_r(new_name);
        goto end;
    }
    vstr_drop(&ctx->current_module_name);
    ctx->current_module_name = new_name.value;
    goto end;

check_load_result:
    if ((expect == assert_malformed) && err_is(ret.msg, e_malformed)) {
        return ok_r;
    }
    if ((expect == assert_invalid) && err_is(ret.msg, e_invalid)) {
        return ok_r;
    }

end:
    if (fp) {
        fclose(fp);
    }
    if (wasm_bin) {
        array_free(wasm_bin);
    }
    return ret;
}

typedef enum action_results {
    action_success,
    action_returns,
    action_exhaustion,
    action_trap,
} action_results;

static r_typed_value read_value(str val_type, str val) {
    assert(!str_is_null(val_type));
    assert(!str_is_null(val));
    check_prep(r_typed_value);
    value_u vu = {0};
    if (str_eq(val, s("nan:canonical")) || str_eq(val, s("nan:arithmetic"))) {
        if (str_eq(val_type, s("f32"))) {
            vu.u_u32 = s_qnan32;
        } else if (str_eq(val_type, s("f64"))) {
            vu.u_u64 = s_qnan64;
        } else {
            return err(e_general, "Invalid type");
        }
    } else if (str_eq(val, s("null"))) {
        vu.u_ref = nullref;
    } else {
        u64 u = 0;
        const u8 * p = val.ptr;
        size_t len = val.len;
        for (size_t i = 0; i < len; i++) {
            if ((p[i] < '0') | (p[i] > '9')) {
                return err(e_general, "Invalid value encoding");
            }
            u = u * 10 + (p[i] - '0');
        }
        vu.u_u64 = u;
    }
    typed_value tv = {.val = vu};
    if (str_eq(val_type, s("i32"))) {
        tv.type = TYPE_ID_i32;
    } else if (str_eq(val_type, s("i64"))) {
        tv.type = TYPE_ID_i64;
    } else if (str_eq(val_type, s("f32"))) {
        tv.type = TYPE_ID_f32;
    } else if (str_eq(val_type, s("f64"))) {
        tv.type = TYPE_ID_f64;
    } else if (str_eq(val_type, s("funcref"))) {
        tv.type = TYPE_ID_funcref;
    } else if (str_eq(val_type, s("externref"))) {
        tv.type = TYPE_ID_externref;
    } else {
        return err(e_general, "Invalid type");
    }
    return ok(tv);
}

static r asserted_action(runner_ctx * ctx, json_object act, action_results expect, json_array * expected_returns) {
    check_prep(r);
    assert(ctx->last_module);
    r ret = {0};

    module * mod = ctx->last_module;
    r_json_string module_name = json_obj_get_string(act, s("module"));
    if (is_ok(module_name)) {
        mod = runtime_module_find(&ctx->rt, module_name.value);
        if (!mod) {
            return err(e_general, "Module not found");
        }
    }
    module_inst * mi = vm_find_module_inst(ctx->current_vm, vec_at_vstr(&mod->names, 0)->s);
    if (!mi) {
        return err(e_general, "Module instance not found");
    }

    unwrap(json_string, field, json_obj_get_string(act, s("field")));
    unwrap(vstr, vs_field, json_unescape_string(field));
    vstr_drop(&ctx->field);
    ctx->field = vs_field;

    unwrap(json_string, type, json_obj_get_string(act, s("type")));
    if (str_eq(type, s("invoke"))) {
        unwrap(json_array, args, json_obj_get_array(act, s("args")));
        vec_typed_value argv = {0};
        JSON_ARRAY_FOR_EACH(args, iter) {
            unwrap(json_object, arg, json_val_as_object(*iter));
            unwrap(json_string, arg_type, json_obj_get_string(arg, s("type")));
            unwrap(json_string, arg_value, json_obj_get_string(arg, s("value")));
            unwrap(typed_value, tv, read_value(arg_type, arg_value));
            check(vec_push_typed_value(&argv, tv), {
                vec_clear_typed_value(&argv);
            });
        }
        func_addr f_addr = vm_find_module_func(ctx->current_vm, mi, ctx->field.s);
        if (!f_addr) {
            vec_clear_typed_value(&argv);
            return err(e_general, "Function not found");
        }
        thread * t = vm_get_thread(ctx->current_vm);
        thread_reset(t);
        ret = interp_call_in_thread(t, f_addr, argv);
        check(vec_dup_typed_value(&ctx->returned_values, &t->results));
    } else if (str_eq(type, s("get"))) {
        glob_addr g_addr = vm_find_module_global(ctx->current_vm, mi, ctx->field.s);
        if (!g_addr) {
            return err(e_general, "Global not found");
        }
        assert(!vec_size_typed_value(&ctx->returned_values));
        check(vec_push_typed_value(&ctx->returned_values, (typed_value){
                                                              .type = g_addr->glob->valtype,
                                                              .val = g_addr->gvalue,
                                                          }));
    } else {
        return err(e_general, "Invalid action type");
    }
    // now let's check the results
    if (expect == action_success) {
        if (!is_ok(ret)) {
            return ret;
        } else {
            return ok_r;
        }
    } else if (expect == action_exhaustion) {
        if (is_ok(ret) || !err_is(ret.msg, e_exhaustion)) {
            return err(e_general, "Expecting exhaustion");
        } else {
            return ok_r;
        }
    } else if (expect == action_trap) {
        if (!is_ok(ret) && vm_get_thread(ctx->current_vm)->trapped) {
            return ok_r;
        } else {
            return err(e_general, "Expecting trap");
        }
    } else if (expect == action_returns) {
        if (!is_ok(ret)) {
            return ret;
        }
        vec_typed_value * results = &ctx->returned_values;
        assert(expected_returns);
        u32 idx = 0;
        JSON_ARRAY_FOR_EACH(*expected_returns, iter) {
            if (idx >= vec_size_typed_value(results)) {
                return err(e_general, "Return value count mismatch");
            }
            typed_value * tv = vec_at_typed_value(results, idx);
            idx++;
            unwrap(json_object, arg, json_val_as_object(*iter));
            unwrap(json_string, arg_type, json_obj_get_string(arg, s("type")));
            unwrap(json_string, arg_value, json_obj_get_string(arg, s("value")));
            unwrap(typed_value, expected, read_value(arg_type, arg_value));
            if (str_eq(arg_type, s("i32"))) {
                if ((tv->type != TYPE_ID_i32) || (tv->val.u_i32 != expected.val.u_i32)) {
                    return err(e_general, "Incorrect i32 return value");
                }
            } else if (str_eq(arg_type, s("i64"))) {
                if ((tv->type != TYPE_ID_i64) || (tv->val.u_i64 != expected.val.u_i64)) {
                    return err(e_general, "Incorrect i64 return value");
                }
            } else if (str_eq(arg_type, s("f32"))) {
                // for floating points, we compare it's integer representation instead.
                if ((tv->type != TYPE_ID_f32)) {
                    return err(e_general, "Incorrect f32 return value");
                }
                if (str_eq(arg_value, s("nan:canonical"))) {
                    if (s_is_canonical_nan32(expected.val.u_f32)) {
                        return ok_r;
                    } else {
                        return err(e_general, "Expecting nan:canonical");
                    }
                }
                if (str_eq(arg_value, s("nan:arithmetic"))) {
                    if (s_is_arithmetic_nan32(expected.val.u_f32)) {
                        return ok_r;
                    } else {
                        return err(e_general, "Expecting nan:arithmetic");
                    }
                }
                if (tv->val.u_u32 == expected.val.u_u32) {
                    return ok_r;
                } else {
                    return err(e_general, "Incorect f32 return value");
                }
            } else if (str_eq(arg_type, s("f64"))) {
                if ((tv->type != TYPE_ID_f64)) {
                    return err(e_general, "Incorrect f64 return value");
                }
                if (str_eq(arg_value, s("nan:canonical"))) {
                    if (s_is_canonical_nan64(expected.val.u_f64)) {
                        return ok_r;
                    } else {
                        return err(e_general, "Expecting nan:canonical");
                    }
                }
                if (str_eq(arg_value, s("nan:arithmetic"))) {
                    if (s_is_arithmetic_nan64(expected.val.u_f64)) {
                        return ok_r;
                    } else {
                        return err(e_general, "Expecting nan:arithmetic");
                    }
                }
                if (tv->val.u_u64 == expected.val.u_u64) {
                    return ok_r;
                } else {
                    return err(e_general, "Incorect f64 return value");
                }
            } else if (str_eq(arg_type, s("funcref"))) {
                if ((tv->type != TYPE_ID_funcref) || (tv->val.u_ref != expected.val.u_ref)) {
                    return err(e_general, "Incorrect funcref return value");
                }
            } else if (str_eq(arg_type, s("externref"))) {
                if ((tv->type != TYPE_ID_externref) || (tv->val.u_ref != expected.val.u_ref)) {
                    return err(e_general, "Incorrect externref return value");
                }
            } else {
                return err(e_general, "Invalid result type");
            }
        }
        return ok_r;
    } else {
        return err(e_general, "Invalid action result");
    }
}

static r process_command_item(runner_ctx * ctx, json_object obj, str dir) {
    check_prep(r);

    // if there is a "module_type", then we simply ignore the text
    r_json_string module_type = json_obj_get_string(obj, s("module_type"));
    if (is_ok(module_type)) {
        if (str_eq(module_type.value, s("text"))) {
            if (ctx->verbose) {
                LOGI("Skipping wat tests");
            }
            return ok_r;
        }
    }

    unwrap(json_string, type, json_obj_get_string(obj, s("type")));

    r_i64 line = json_obj_get_i64(obj, s("line"));
    if (is_ok(line) && ctx->verbose) { // I think it's optional.
        LOGI("type:%.*s line:%" PRId64 "", (int)type.len, type.ptr, line.value);
    }

    if (str_eq(type, s("module"))) {
        // filename is not optional
        unwrap(json_string, filename, json_obj_get_string(obj, s("filename")));
        // but module name is
        r_json_string name = json_obj_get_string(obj, s("name"));
        json_string module_name = is_ok(name) ? name.value : STR_NULL;
        return asserted_load(ctx, filename, dir, module_name, load_ok);
    } else if (str_eq(type, s("action"))) {
        unwrap(json_object, action, json_obj_get_object(obj, s("action")));
        return asserted_action(ctx, action, action_success, NULL);
    } else if (str_eq(type, s("assert_return"))) {
        unwrap(json_object, action, json_obj_get_object(obj, s("action")));
        unwrap(json_array, expected, json_obj_get_array(obj, s("expected")));
        return asserted_action(ctx, action, action_returns, &expected);
    } else if (str_eq(type, s("assert_exhaustion"))) {
        unwrap(json_object, action, json_obj_get_object(obj, s("action")));
        return asserted_action(ctx, action, action_exhaustion, NULL);
    } else if (str_eq(type, s("assert_trap"))) {
        unwrap(json_object, action, json_obj_get_object(obj, s("action")));
        return asserted_action(ctx, action, action_trap, NULL);
    } else if (str_eq(type, s("assert_invalid"))) {
        unwrap(json_string, filename, json_obj_get_string(obj, s("filename")));
        return asserted_load(ctx, filename, dir, STR_NULL, assert_invalid);
    } else if (str_eq(type, s("assert_malformed"))) {
        unwrap(json_string, filename, json_obj_get_string(obj, s("filename")));
        return asserted_load(ctx, filename, dir, STR_NULL, assert_malformed);
    } else if (str_eq(type, s("assert_uninstantiable"))) {
        unwrap(json_string, filename, json_obj_get_string(obj, s("filename")));
        return asserted_load(ctx, filename, dir, STR_NULL, assert_uninstantiable);
    } else if (str_eq(type, s("assert_unlinkable"))) {
        unwrap(json_string, filename, json_obj_get_string(obj, s("filename")));
        return asserted_load(ctx, filename, dir, STR_NULL, assert_unlinkable);
    } else if (str_eq(type, s("register"))) {
        r_json_string module_name_r = json_obj_get_string(obj, s("name"));
        json_string module_name = is_ok(module_name_r) ? module_name_r.value : STR_NULL;
        unwrap(json_string, as_name, json_obj_get_string(obj, s("as")));
        return register_module_as(ctx, module_name, as_name);
    }
    return ok_r;
}

static r process_commands(runner_ctx * ctx, json_array commands, str dir) {
    check_prep(r);
    // {"type": <string>, "line": <number>, ...}
    JSON_ARRAY_FOR_EACH(commands, val) {
        vec_clear_typed_value(&ctx->returned_values);
        unwrap(json_object, obj, json_val_as_object(*val));
        check(process_command_item(ctx, obj, dir));
    }
    return ok_r;
}

r do_spec_test(json_value json, str dir, bool verbose) {
    check_prep(r);

    runner_ctx ctx = {0};
    ctx.verbose = verbose;
    unwrap(module, spectest_mod, host_mod_new(s("spectest")));
    check(runtime_module_add_mod(&ctx.rt, spectest_mod));

    unwrap(json_object, root, json_val_as_object(json));
    r_json_value src_name = json_object_find(root, s("source_filename"));
    unwrap(json_string, name, json_val_as_string(src_name.value));

    unwrap(json_array, commands, json_obj_get_array(root, s("commands")));

    LOGW("Running [%.*s]", (int)(name.len), name.ptr);
    r ret = process_commands(&ctx, commands, dir);
    if (is_ok(ret)) {
        LOGW("Successful [%.*s]", (int)(name.len), name.ptr);
    } else {
        LOGW("Failed [%.*s]", (int)(name.len), name.ptr);
    }
    runner_ctx_drop(&ctx);
    return ret;
}

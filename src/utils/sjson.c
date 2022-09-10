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

#include "sjson.h"

#include "alloc.h"
#include "str.h"
#include "stream.h"
#include "vec_impl.h"
#include "vstr.h"

#include <assert.h>
#include <math.h>

VEC_IMPL_FOR_TYPE(json_kv)
VEC_IMPL_FOR_TYPE(json_value)

static bool is_ws(u8 c) {
    return ((c == 0x20) || (c == 0x0a) || (c == 0x0d) || (c == 0x09));
}

static void eat_ws(stream * st) {
    assert(st);
    while (stream_remaining(st)) {
        r_u8 ret = stream_read_u8(st); // shouldn't fail.
        assert(is_ok(ret));
        if (is_ws(ret.value)) {
            continue;
        } else {
            r ret1 = stream_seek(st, -1);
            UNUSED(ret1);
            assert(is_ok(ret1));
            return;
        }
    }
}

static bool is_digit(u8 c) {
    return (c >= '0') && (c <= '9');
}

static bool is_hex(u8 c) {
    return (is_digit(c) || ((c >= 'a') && (c <= 'f'))) || ((c >= 'A') && (c <= 'F'));
}

u8 hex_to_u8(u8 h) {
    assert(is_hex(h));
    if (is_digit(h)) {
        return h - '0';
    } else {
        if ((h >= 'a') && (h <= 'f')) {
            return h - 'a' + 10;
        } else {
            return h - 'A' + 10;
        }
    }
}

static bool stream_ended(stream * st) {
    if (!stream_remaining(st)) {
        return true;
    } else {
        r_u8 c = stream_peek_u8(st);
        assert(is_ok(c));
        if (c.value == '\0') {
            return true;
        }
    }
    return false;
}

// the function doesn't produce anything and only move the stream forward.
static r json_parse_escape(stream * st) {
    assert(st);
    check_prep(r);
    unwrap(u8, c, stream_read_u8(st));
    switch (c) {
        case '\\':
        case '"':
        case '/':
        case 'b':
        case 'f':
        case 'n':
        case 'r':
        case 't':
            return ok_r;
        case 'u': {
            for (u32 i = 0; i < 4; i++) {
                unwrap(u8, h, stream_read_u8(st));
                if (is_hex(h)) {
                    continue;
                } else {
                    return err(e_general, "Invalid JSON format");
                }
            }
        }
            return ok_r;
    }
    return err(e_general, "Invalid JSON format");
}

r_vstr json_unescape_string(json_string str) {
    check_prep(r_vstr);

    vstr vs = {0};
    stream input = stream_from(str);
    while (stream_remaining(&input)) {
        unwrap(u8, c, stream_read_u8(&input), {
            vstr_drop(&vs);
        });
        if (c == '\\') {
            unwrap(u8, d, stream_read_u8(&input), {
                vstr_drop(&vs);
            });
            if (d != 'u') {
                vstr_drop(&vs);
                return err(e_general, "Invalid escape encoding");
            }
            u32 v = 0;
            for (u32 i = 0; i < 4; i++) {
                unwrap(u8, h, stream_read_u8(&input), {
                    vstr_drop(&vs);
                });
                if (!is_hex(h)) {
                    vstr_drop(&vs);
                    return err(e_general, "Invalid escape encoding");
                }
                v = v * 16 + hex_to_u8(h);
            }
            if (v > 0xff) {
                vstr_drop(&vs);
                return err(e_general, "spectest json only encode one byte hex");
            }
            c = (u8)v;
        }
        check(vstr_append_c(&vs, c), {
            vstr_drop(&vs);
        });
    }
    return ok(vs);
}

static r json_parse_string(json_string * string, stream * st) {
    assert(string);
    assert(st);
    check_prep(r);
    str_iter_t p;
    {
        eat_ws(st);
        unwrap(u8, c, stream_read_u8(st));
        if (c != '"') {
            return err(e_general, "Invalid JSON format");
        }
        p = st->p;
    }
    while (true) {
        unwrap(u8, c, stream_read_u8(st));
        if (c == '"') {
            if (st->p != p + 1) {
                *string = (str){
                    .ptr = p,
                    .len = st->p - p - 1,
                };
            }
            break;
        }
        if (c == '\\') {
            check(json_parse_escape(st));
        }
        if (c < 0x20) {
            return err(e_general, "Invalid JSON format");
        }
    }
    return ok_r;
}

static r json_parse_number(json_number * num, stream * st) {
    assert(num);
    assert(st);
    check_prep(r);

    int sign = 1;
    bool is_float = false;
    i64 num_i = 0;
    f64 num_f = 0.0;
    {
        unwrap(u8, c, stream_peek_u8(st));
        if (c == '-') {
            sign = -1;
            check(stream_seek(st, 1));
        }
    }
    {
        unwrap(u8, c, stream_peek_u8(st));
        if (!is_digit(c)) {
            return err(e_general, "Invalid JSON format");
        }
        if (c != '0') {
            while (true) {
                unwrap(u8, d, stream_read_u8(st));
                if (!is_digit(d)) {
                    check(stream_seek(st, -1));
                    break;
                }
                num_i = num_i * 10 + (d - '0');
                if (stream_ended(st)) {
                    break;
                }
            }
        } else {
            check(stream_seek(st, 1));
        }
    }
    if (!stream_ended(st)) {
        unwrap(u8, c, stream_peek_u8(st));
        if (c == '.') {
            is_float = true;
            num_f = (f64)(num_i);
            check(stream_seek(st, 1));
            f64 frac = 0.1;
            unwrap(u8, d, stream_read_u8(st));
            if (!is_digit(d)) {
                return err(e_general, "Invalid JSON format");
            }
            num_f += frac * d;
            frac = frac * frac;
            while (true) {
                if (stream_ended(st)) {
                    break;
                }
                unwrap(u8, e, stream_read_u8(st));
                if (!is_digit(e)) {
                    check(stream_seek(st, -1));
                    break;
                }
                num_f += frac * e;
                frac = frac * frac;
            }
        }
    }
    if (!stream_ended(st)) {
        unwrap(u8, c, stream_peek_u8(st));
        if (c == 'E' || c == 'e') {
            if (!is_float) {
                is_float = true;
                num_f = (f64)(num_i);
            }
            check(stream_seek(st, 1));
            unwrap(u8, d, stream_peek_u8(st));
            bool exp_neg = false;
            f64 exp = 0.0;
            if (d == '+' || d == '-') {
                check(stream_seek(st, 1));
                if (d == '-') {
                    exp_neg = true;
                }
            }
            unwrap(u8, e, stream_read_u8(st));
            if (!is_digit(e)) {
                return err(e_general, "Invalid JSON format");
            }
            exp = exp * 10 + (e - '0');
            while (true) {
                if (stream_ended(st)) {
                    break;
                }
                unwrap(u8, f, stream_read_u8(st));
                if (!is_digit(f)) {
                    check(stream_seek(st, -1));
                    break;
                }
                exp = exp * 10 + (f - '0');
            }
            if (exp_neg) {
                exp = -exp;
            }
            num_f = num_f * pow(10, exp);
        }
    }
    num->is_integer = !is_float;
    if (num->is_integer) {
        num->number_u.i = num_i * sign;
    } else {
        num->number_u.f = num_f * sign;
    }
    return ok_r;
}

static r json_parse_object(json_object * object, stream * st);
static r json_parse_value(json_value * value, stream * st);

static r json_parse_array(json_array * array, stream * st) {
    assert(array);
    assert(st);
    check_prep(r);

    eat_ws(st);
    unwrap(u8, c, stream_read_u8(st));
    if (c != '[') {
        return err(e_general, "Invalid JSON format");
    }
    eat_ws(st);
    {
        unwrap(u8, d, stream_peek_u8(st));
        if (d == ']') {
            check(stream_seek(st, 1));
            return ok_r;
        }
    }
    while (true) {
        eat_ws(st);
        check(vec_push_json_value(&array->arr, (json_value){0}));
        json_value * val = vec_back_json_value(&array->arr);
        assert(val);
        check(json_parse_value(val, st));
        eat_ws(st);
        unwrap(u8, d, stream_read_u8(st));
        if (d == ',') {
            continue;
        } else if (d == ']') {
            break;
        } else {
            return err(e_general, "Invalid JSON format");
        }
    }
    vec_shrink_to_fit_json_value(&array->arr);
    return ok_r;
}

static r json_parse_value(json_value * value, stream * st) {
    assert(value);
    assert(st);
    check_prep(r);

    eat_ws(st);
    unwrap(u8, c, stream_peek_u8(st));
    switch (c) {
        case '{': {
            value->tag = json_type_object;
            value->jvalue_u.obj = array_alloc(json_object, 1);
            if (!value->jvalue_u.obj) {
                return err(e_general, "sjson: out of memory");
            }
            *value->jvalue_u.obj = (json_object){0};
            check(json_parse_object(value->jvalue_u.obj, st));
            break;
        }
        case '[': {
            value->tag = json_type_array;
            value->jvalue_u.arr = array_alloc(json_array, 1);
            if (!value->jvalue_u.arr) {
                return err(e_general, "sjson: out of memory");
            }
            *value->jvalue_u.arr = (json_array){0};
            check(json_parse_array(value->jvalue_u.arr, st));
            break;
        }
        case 't': {
            if (!strncmp((const char *)(st->p), "true", 4)) {
                value->tag = json_type_boolean;
                value->jvalue_u.boolean = true;
                unwrap_drop(stream, stream_slice(st, 4), {
                    value->tag = json_type_invalid;
                });
                break;
            }
        }
        case 'f': {
            if (stream_remaining(st) >= 5) {
                if (!strncmp((const char *)(st->p), "false", 5)) {
                    value->tag = json_type_boolean;
                    value->jvalue_u.boolean = false;
                    unwrap_drop(stream, stream_slice(st, 5), {
                        value->tag = json_type_invalid;
                    });
                    break;
                }
            }
        }
        case 'n': {
            if (!strncmp((const char *)(st->p), "null", 4)) {
                value->tag = json_type_null;
                unwrap_drop(stream, stream_slice(st, 4), {
                    value->tag = json_type_invalid;
                });
                break;
            }
        }
        case '"': {
            value->tag = json_type_string;
            check(json_parse_string(&value->jvalue_u.str, st), {
                value->tag = json_type_invalid;
            });
            break;
        }
        default: {
            // either it's a number or invalid.
            value->tag = json_type_number;
            check(json_parse_number(&value->jvalue_u.num, st), {
                value->tag = json_type_invalid;
            });
            break;
        }
    }
    return ok_r;
}

static r json_parse_object(json_object * object, stream * st) {
    assert(object);
    assert(st);
    check_prep(r);

    {
        eat_ws(st);
        unwrap(u8, c, stream_read_u8(st));
        if (c != '{') {
            return err(e_general, "Invalid JSON format");
        }
    }
    {
        eat_ws(st);
        unwrap(u8, c, stream_peek_u8(st));
        if (c == '}') {
            check(stream_seek(st, 1));
            return ok_r;
        }
    }
    do {
        vec_push_json_kv(&object->vec_kv, (json_kv){0});
        json_kv * kv = vec_back_json_kv(&object->vec_kv);
        eat_ws(st);
        check(json_parse_string(&kv->k, st));

        eat_ws(st);
        unwrap(u8, colon, stream_read_u8(st));
        if (colon != ':') {
            return err(e_general, "Invalid JSON format");
        }

        eat_ws(st);
        check(json_parse_value(&kv->v, st));

        eat_ws(st);
        unwrap(u8, comma_or_end, stream_read_u8(st));
        if (comma_or_end == ',') {
            continue;
        } else if (comma_or_end == '}') {
            break;
        } else {
            return err(e_general, "Invalid JSON format");
        }
    } while (true);
    vec_shrink_to_fit_json_kv(&object->vec_kv);
    return ok_r;
}

void json_free_string(json_string * string) {
    assert(string);
    if (str_is_null(*string)) {
        return;
    }
    *string = (json_string){0};
}

void json_free_value(json_value * value);

// free the object's contents rather than itself because we don't
// know if the object is heap-allocated or now.
static void json_free_object(json_object * object) {
    assert(object);
    VEC_FOR_EACH(&object->vec_kv, json_kv, kv) {
        json_free_string(&kv->k);
        json_free_value(&kv->v);
    }
    vec_clear_json_kv(&object->vec_kv);
    *object = (json_object){0};
}

static void json_free_array(json_array * array) {
    assert(array);
    VEC_FOR_EACH(&array->arr, json_value, value) {
        json_free_value(value);
    }
    vec_clear_json_value(&array->arr);
}

void json_free_value(json_value * value) {
    assert(value);
    if (json_val_is_invalid(*value)) {
        return;
    }
    switch (value->tag) {
        case json_type_object: {
            if (value->jvalue_u.obj) {
                json_free_object(value->jvalue_u.obj);
                array_free(value->jvalue_u.obj);
            }
            // the object cannot be at the stack, so reclaim the memory.
            break;
        }
        case json_type_array: {
            if (value->jvalue_u.arr) {
                json_free_array(value->jvalue_u.arr);
                array_free(value->jvalue_u.arr);
            }
            break;
        }
        case json_type_string: {
            json_free_string(&value->jvalue_u.str);
            break;
        }
        case json_type_number:
        case json_type_boolean:
        case json_type_null: {
            break;
        }
        default: {
            assert(false);
        }
    }
    *value = (json_value){0};
}

r_json_value json_parse(str json) {
    check_prep(r_json_value);
    json_value value = {0};

    if (str_is_null(json)) {
        return ok(value);
    }

    stream st = stream_from(json);
    eat_ws(&st);
    // an empty space is perfectly fine
    if (stream_ended(&st)) {
        return ok(value);
    }
    // parse the actual value.
    check(json_parse_value(&value, &st), {
        json_free_value(&value);
    });

    // the remaining of the root object should be either ws or a trailing zero.
    eat_ws(&st);
    if (!stream_ended(&st)) {
        json_free_value(&value);
        return err(e_general, "Invalid JSON format");
    }
    return ok(value);
}

r_json_value json_object_find(json_object obj, json_string key) {
    check_prep(r_json_value);
    if (str_is_null(key)) {
        return err(e_general, "Key is null");
    }
    JSON_OBJECT_FOR_EACH(obj, kv) {
        if (str_eq(kv->k, key)) {
            return ok(kv->v);
        }
    }
    return err(e_general, "Key not found");
}

#define json_to_object(value) (*((value).jvalue_u.obj))
#define json_to_array(value) (*((value).jvalue_u.arr))
#define json_to_number(value) ((value).jvalue_u.num)
#define json_to_string(value) ((value).jvalue_u.str)
#define json_to_boolean(value) ((value).jvalue_u.boolean)

#define JSON_VAL_AS(type)                              \
    r_json_##type json_val_as_##type(json_value val) { \
        check_prep(r_json_##type);                     \
        if (json_val_is_##type(val)) {                 \
            return ok(json_to_##type(val));            \
        }                                              \
        return err(e_general, "The value is not an " #type);      \
    }

FOR_EACH_JSON_TYPE(JSON_VAL_AS)

r_i64 json_val_as_i64(json_value val) {
    check_prep(r_i64);
    unwrap(json_number, number, json_val_as_number(val));
    return number.is_integer ? ok(number.number_u.i) : err(e_general, "value is not an integer");
}

r_f64 json_val_as_f64(json_value val) {
    check_prep(r_f64);
    unwrap(json_number, number, json_val_as_number(val));
    return number.is_integer ? err(e_general, "value is not a float") : ok(number.number_u.f);
}

#define JSON_OBJ_GET(type)                                                \
    r_json_##type json_obj_get_##type(json_object obj, json_string key) { \
        check_prep(r_json_##type);                                        \
        if (str_is_null(key)) {                                           \
            return err(e_general, "key is null");                                    \
        }                                                                 \
        unwrap(json_value, val, json_object_find(obj, key));              \
        return json_val_as_##type(val);                                   \
    }

FOR_EACH_JSON_TYPE(JSON_OBJ_GET)

r_i64 json_obj_get_i64(json_object obj, json_string key) {
    check_prep(r_i64);
    unwrap(json_number, val, json_obj_get_number(obj, key));
    return val.is_integer ? ok(val.number_u.i) : err(e_general, "value is not an integer");
}

r_f64 json_obj_get_f64(json_object obj, json_string key) {
    check_prep(r_f64);
    unwrap(json_number, val, json_obj_get_number(obj, key));
    return val.is_integer ? err(e_general, "value is not a float") : ok(val.number_u.f);
}

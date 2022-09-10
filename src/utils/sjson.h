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
#include "str.h"
#include "types.h"
#include "vec.h"
#include "vstr.h"

#include <stdbool.h>

// A tiny JSON library implementing the ECMA-404 standard.
// Since JSON element form a tree and doesn't have circles, the entire tree
// is copyable. Therefore it's safe to copy the elements around as long as
// the root element is freed in the end.
// The decoder expects UTF-8.

// You might have noticed that most of the functions pass JSON objects around
// by value, and that's fine because once the JSON is parsed, the heap allocated
// memory are all tracable through the root object. All the copied elements will
// either carry its internal values (like number/bool etc) or holding pointers into
// the root tree. So the entire tree can be considered immutable. Therefore as long
// as the root element is freed in the end, there's no memory leak.

typedef enum json_type {
    json_type_invalid = 0,
    json_type_object,
    json_type_array,
    json_type_string,
    json_type_number,
    json_type_boolean,
    json_type_null,
} json_type;

struct json_object;
struct json_array;

typedef struct json_number {
    bool is_integer;
    union number_u {
        i64 i;
        f64 f;
    } number_u;
} json_number;
RESULT_TYPE_DECL(json_number)

typedef bool json_boolean;
RESULT_TYPE_DECL(json_boolean)

typedef str json_string;
RESULT_TYPE_DECL(json_string)

// JSON is a recursive format, so there has to be some pointers to
// break the circular dependence.
// json_null doesn't have a value field since it has only one value.
typedef struct json_value {
    json_type tag;
    union jvalue_u {
        struct json_object * obj;
        struct json_array * arr;
        json_string str;
        json_number num;
        json_boolean boolean;
    } jvalue_u;
} json_value;
VEC_DECL_FOR_TYPE(json_value)
RESULT_TYPE_DECL(json_value)

typedef struct json_array {
    vec_json_value arr;
} json_array;
RESULT_TYPE_DECL(json_array)

typedef struct json_kv {
    json_string k;
    json_value v;
} json_kv;
VEC_DECL_FOR_TYPE(json_kv)

typedef struct json_object {
    vec_json_kv vec_kv;
} json_object;
RESULT_TYPE_DECL(json_object)

// returning the root "element", which by definition is "ws + value + ws".
// Therefore we simply return the root value
r_json_value json_parse(str json);

// The root json value needs to be freed in order to release its internal heap memories.
void json_free_value(json_value * object);

// find the value with the given key field in an object.
r_json_value json_object_find(json_object obj, json_string key);

// helper function to do the check and get the value at the same time.
r_json_object json_val_as_object(json_value val);
r_json_array json_val_as_array(json_value val);
r_json_string json_val_as_string(json_value val);
r_json_number json_val_as_number(json_value val);
r_i64 json_val_as_i64(json_value val);
r_f64 json_val_as_f64(json_value val);
r_json_boolean json_val_as_boolean(json_value val);

// get a value from a object directly.
r_json_object json_obj_get_object(json_object obj, json_string key);
r_json_array json_obj_get_array(json_object obj, json_string key);
r_json_string json_obj_get_string(json_object obj, json_string key);
r_json_number json_obj_get_number(json_object obj, json_string key);
r_i64 json_obj_get_i64(json_object obj, json_string key);
r_f64 json_obj_get_f64(json_object obj, json_string key);
r_json_boolean json_obj_get_boolean(json_object obj, json_string key);

#define JSON_OBJECT_FOR_EACH(obj, iter) VEC_FOR_EACH(&((obj).vec_kv), json_kv, iter)
#define JSON_ARRAY_FOR_EACH(array, iter) VEC_FOR_EACH(&((array).arr), json_value, iter)

#define FOR_EACH_JSON_TYPE(macro) \
    macro(object)                 \
    macro(array)                  \
    macro(string)                 \
    macro(number)                 \
    macro(boolean)

#define JSON_VAL_IS(type)                            \
    INLINE bool json_val_is_##type(json_value val) { \
        return val.tag == json_type_##type;          \
    }
FOR_EACH_JSON_TYPE(JSON_VAL_IS)

INLINE bool json_val_is_invalid(json_value val) {
    return (val.tag == json_type_invalid);
}

INLINE bool json_val_is_i64(json_value val) {
    return ((val.tag == json_type_number) && (val.jvalue_u.num.is_integer));
}

INLINE bool json_val_is_f64(json_value val) {
    return ((val.tag == json_type_number) && !(val.jvalue_u.num.is_integer));
}

INLINE bool json_val_is_null(json_value val) {
    return (val.tag == json_type_null);
}

// it's only used for spec test escaping format.
r_vstr json_unescape_string(json_string str);

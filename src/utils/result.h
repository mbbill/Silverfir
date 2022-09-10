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

#include "compiler.h"
#include "silverfir.h"
#include "string.h"
#include "types.h"

// The wrapped type must be copyable, which means it cannot be self-referenced.

// Most of the errors are categorized as general errors.
// malformed error happens when parser detects wasm binary syntax errors.
// invalid error happens when the validation fails.
// trap, as the name implies, happens on trap.
#define e_general "[e_general]"
#define e_malformed "[e_malformed]"
#define e_invalid "[e_invalid]"
#define e_unlinkable "[e_unlinkable]"
#define e_exhaustion "[e_exhaustion]"
#define e_trap "[e_trap]"
#define e_exit "[e_exit]"

#define err_is(_msg_, _type_) !strncmp(_msg_, _type_, strlen(_type_))

// general result, works like bool
typedef const char * err_msg_t;
typedef struct r {
    err_msg_t msg;
} r;

#define ok_r ((r){0})

#define RESULT_TYPE_DECL_BUILTIN(type, _) \
    typedef struct r_##type {             \
        err_msg_t msg;                    \
        type value;                       \
    } r_##type;

// convert a r_type to r
#define to_r(x)      \
    (r) {            \
        .msg = x.msg \
    }

FOR_EACH_TYPE(RESULT_TYPE_DECL_BUILTIN, _)

#define RESULT_TYPE_DECL(type) RESULT_TYPE_DECL_BUILTIN(type, _)

// DEPRECATED, please ignore.
// #define RESULT_OK(type, v)  type* : (r_##type) { .result = *((type*)(&v)) },
// #define ok(v) _Generic((&v),    \
//     RESULT_FOR_EACH_STRUCT_TYPE(RESULT_OK, v) \
//     default: err_undefined \
// )

// the two macros are mostly for tests. You should use unwrap and check when possible.
#define is_ok(v) (v.msg == NULL)
#define err_msg(v) (v.msg)

// Most of the macros in this header requires one line of
// check_prep(return_type);
// at the beginning of the function body.
#define check_prep(rtype) rtype _return_val_ = {0};

#define ok(_val_) (_return_val_.msg = NULL, _return_val_.value = _val_, _return_val_)
#if !defined(NDEBUG)
#define err(_type_, _msg_) (_return_val_.msg = (err_msg_t)(_type_ " " _msg_ " (" SRC_LOCATION ")"), _return_val_)
#else
// save a bit of space and doesn't leak source info
#define err(_type_, _msg_) (_return_val_.msg = (err_msg_t)(_type_ " " _msg_), _return_val_)
#endif

// The macro works like below statement
// type v = stmt(...);
// except that it will automatically unwrap the return type r_xxx
// or return when error happened.
// if you want to do some cleanup before return, put them in the "stmt ..."
// e.g.
// unwrap(u32, a, get_u32(), clean_up_before_return(););
// In general, the macros works like the question mark operator (?) in Rust.
#define unwrap(type, v, stmt, ...)        \
    type v;                               \
    do {                                  \
        r_##type _ret_ = stmt;            \
        if (unlikely(_ret_.msg)) {        \
            __VA_ARGS__;                  \
            _return_val_.msg = _ret_.msg; \
            return _return_val_;          \
        } else {                          \
            v = _ret_.value;              \
        }                                 \
    } while (0)

// only care about the error code and drop the value
#define unwrap_drop(type, stmt, ...)      \
    {                                     \
        r_##type _ret_ = stmt;            \
        if (unlikely(_ret_.msg)) {        \
            __VA_ARGS__;                  \
            _return_val_.msg = _ret_.msg; \
            return _return_val_;          \
        }                                 \
    }

// if a function returns type r, use check(func()). The macro
// will automatically forward the error, or fall through on ok.
#define check(stmt, ...)                  \
    do {                                  \
        r _ret_ = stmt;                   \
        if (unlikely(_ret_.msg)) {        \
            __VA_ARGS__;                  \
            _return_val_.msg = _ret_.msg; \
            return _return_val_;          \
        }                                 \
    } while (0)

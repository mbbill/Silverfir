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

// including WebKit's license header for LEB decoders

/*
 * Copyright (C) 2016 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

// Protocol Buffers - Google's data interchange format
// Copyright 2008 Google Inc.  All rights reserved.
// https://developers.google.com/protocol-buffers/
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
// notice, this list of conditions and the following disclaimer.
//     * Redistributions in binary form must reproduce the above
// copyright notice, this list of conditions and the following disclaimer
// in the documentation and/or other materials provided with the
// distribution.
//     * Neither the name of Google Inc. nor the names of its
// contributors may be used to endorse or promote products derived from
// this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "alloc.h"
#include "mem_util.h"
#include "result.h"
#include "silverfir.h"
#include "str.h"
#include "types.h"

#include <assert.h>

// A stream type with internal pointer to the newest reading position.
// the LEB128 parser are from the webkit project and the protobuf project

typedef struct stream {
    str s;
    str_iter_t p;
} stream;

RESULT_TYPE_DECL(stream)

INLINE stream stream_from(str s) {
    assert(str_is_valid(s));
    return (stream){.s = s, .p = s.ptr};
}

INLINE bool stream_eq(stream a, stream b) {
    return (str_eq(a.s, b.s) && (a.p == b.p));
}

INLINE bool stream_is_valid(stream * st) {
    return (str_is_valid(st->s) && (st->p >= st->s.ptr) && (st->s.ptr + st->s.len >= st->p));
}

INLINE size_t stream_remaining(stream * st) {
    assert(stream_is_valid(st));
    return (size_t)(st->s.ptr + st->s.len - st->p);
}

INLINE r_u32 stream_read_u32(stream * st) {
    assert(stream_is_valid(st));
    check_prep(r_u32);
    if (unlikely(stream_remaining(st) < sizeof(u32))) {
        return err(e_malformed, "stream: not enough data");
    }
    u32 u = (st->p[0] | st->p[1] << 8 | st->p[2] << 16 | st->p[3] << 24);
    st->p += sizeof(u32);
    return ok(u);
}

INLINE r_u8 stream_peek_u8(stream * st) {
    assert(stream_is_valid(st));
    check_prep(r_u8);
    if (unlikely(stream_remaining(st) < sizeof(u8))) {
        return err(e_malformed, "stream: not enough data");
    }
    u8 u = st->p[0];
    return ok(u);
}

INLINE r_u8 stream_read_u8(stream * st) {
    assert(stream_is_valid(st));
    check_prep(r_u8);
    unwrap(u8, u, stream_peek_u8(st));
    st->p += sizeof(u8);
    return ok(u);
}

// Consume the original and return the slice from the beginning of the stream.
INLINE r_stream stream_slice(stream * st, size_t size) {
    assert(st);
    assert(size);
    check_prep(r_stream);
    if (stream_remaining(st) < size) {
        return err(e_malformed, "stream: not enough data");
    }
    stream slice = (stream){
        .s.ptr = st->p,
        .s.len = size,
        .p = st->p,
    };
    st->p += size;
    return ok(slice);
}

// seek forward or backward by 'len' elements. len can be negative, which means
// moving backward.
INLINE r stream_seek(stream * st, i64 len) {
    assert(st);
    assert(st->p);
    check_prep(r);
    str_iter_t p = st->p + len;
    if (unlikely((p < st->s.ptr) || (p > (st->s.ptr + st->s.len)))) {
        return err(e_general, "stream: seeking position exceeds limit");
    }
    st->p = p;
    return ok_r;
}

// The LEB decoder. Decodes varuintN, varintN
// e.g. vu7(varuint7) vi32(varint32) vu32(varuint32)
INLINE r_u8 stream_read_vu7(stream * st) {
    assert(stream_is_valid(st));
    check_prep(r_u8);
    if (unlikely(stream_remaining(st) < sizeof(u8))) {
        return err(e_malformed, "stream: not enough data");
    }
    u8 u = st->p[0];
    if (unlikely(u >= 0x80)) {
        return err(e_malformed, "stream: invalid LEB123 encoding");
    }
    st->p += sizeof(u8);
    return ok(u);
}

INLINE r_i8 stream_read_vi7(stream * st) {
    assert(stream_is_valid(st));
    check_prep(r_i8);
    if (unlikely(stream_remaining(st) < sizeof(i8))) {
        return err(e_malformed, "stream: not enough data");
    }
    u8 u = st->p[0];
    if (unlikely(u >= 0x80)) {
        return err(e_malformed, "stream: invalid LEB123 encoding");
    }
    st->p += sizeof(u8);
    return ok((i8)((u & 0x40) ? ((i8)(u | 0x80)) : u));
}

#define STREAM_READ_FLOAT_DECL(type)                              \
    INLINE r_##type stream_read_##type(stream * st) {             \
        assert(stream_is_valid(st));                              \
        check_prep(r_##type);                                     \
        if (unlikely(stream_remaining(st) < sizeof(type))) {      \
            return err(e_malformed, "stream: not enough data"); \
        }                                                         \
        type val = mem_read_##type(st->p);                        \
        st->p += sizeof(type);                                    \
        return ok(val);                                           \
    }
STREAM_READ_FLOAT_DECL(f32)
STREAM_READ_FLOAT_DECL(f64)

#define LEB_MAX_LEN(type) ((sizeof(type) * 8 - 1) / 7 + 1)
#define LEB_LAST_BYTE_MASK(type) (~((1 << (sizeof(type) * 8) % 7) - 1))

#define STREAM_READ_LEB_UNSIGNED_DECL(type)                                            \
    INLINE r_##type stream_read_v##type(stream * st) {                                 \
        check_prep(r_##type);                                                          \
        assert(stream_is_valid(st));                                                   \
        type result = 0;                                                               \
        size_t offset = 0;                                                             \
        u16 shift = 0;                                                                 \
        while ((offset < LEB_MAX_LEN(type)) && (offset < stream_remaining(st))) {      \
            u8 b = st->p[offset];                                                      \
            result |= (type)(b & 0x7f) << shift;                                       \
            shift += 7;                                                                \
            offset++;                                                                  \
            if (!(b & 0x80)) {                                                         \
                if ((offset == LEB_MAX_LEN(type)) && (b & LEB_LAST_BYTE_MASK(type))) { \
                    break;                                                             \
                }                                                                      \
                st->p += offset;                                                       \
                return ok(result);                                                     \
            }                                                                          \
        }                                                                              \
        return err(e_malformed, "stream: invalid LEB123 encoding");                  \
    }
// varint
STREAM_READ_LEB_UNSIGNED_DECL(u32)
STREAM_READ_LEB_UNSIGNED_DECL(u64)

#define STREAM_READ_LEB_SIGNED_DECL(type, utype)                                  \
    INLINE r_##type stream_read_v##type(stream * st) {                            \
        assert(stream_is_valid(st));                                              \
        check_prep(r_##type)                                                      \
            type result                                                           \
            = 0;                                                                  \
        size_t offset = 0;                                                        \
        u16 shift = 0;                                                            \
        while ((offset < LEB_MAX_LEN(type)) && (offset < stream_remaining(st))) { \
            u8 b = st->p[offset];                                                 \
            result |= (utype)(b & 0x7f) << shift;                                 \
            shift += 7;                                                           \
            offset++;                                                             \
            if (!(b & 0x80)) {                                                    \
                if (offset == LEB_MAX_LEN(type)) {                                \
                    if (!(b & 0x40)) {                                            \
                        if (b & (LEB_LAST_BYTE_MASK(type) >> 1))                  \
                            break;                                                \
                    } else {                                                      \
                        if ((u8)(~(b | 0x80)) & (LEB_LAST_BYTE_MASK(type) >> 1))  \
                            break;                                                \
                    }                                                             \
                }                                                                 \
                st->p += offset;                                                  \
                if ((shift < (sizeof(type) * 8)) && (b & 0x40)) {                 \
                    result = (utype)(result) | (((utype)(-1)) << shift);          \
                }                                                                 \
                return ok(result);                                                \
            }                                                                     \
        }                                                                         \
        return err(e_malformed, "stream: invalid LEB123 encoding");             \
    }

STREAM_READ_LEB_SIGNED_DECL(i32, u64)
STREAM_READ_LEB_SIGNED_DECL(i64, u64)

// The unchecked functions doesn't really belong here since they don't take a stream but rather
// part of a stream - stream.p
#define stream_peek_u8_unchecked(p) (*p)
#define stream_read_u8_unchecked(p) (*(p++))
#define stream_seek_unchecked(p, len) (p += len)
#define stream_read_vi7_unchecked(target, p)                            \
    do {                                                                \
        u8 _val_ = (*(p++));                                            \
        target = ((i8)((_val_ & 0x40) ? ((i8)(_val_ | 0x80)) : _val_)); \
    } while (0)

#define stream_read_f32_unchecked(target, p) \
    do {                                     \
        target = mem_read_f32(p);            \
        p += sizeof(f32);                    \
    } while (0)

#define stream_read_f64_unchecked(target, p) \
    {                                        \
        target = mem_read_f64(p);            \
        p += sizeof(f64);                    \
    }                                        \
    while (0)

// clang-format off
INLINE u32 __stream_read_vu32_unchecked(const u8 ** p) {
    u32 b = 0;
    u32 result = 0;
    const u8 * ptr = *p;
    b = *(ptr++); result += b ; if (!(b & 0x80)) goto done;
    result -= 0x80;
    b = *(ptr++); result += b <<  7; if (!(b & 0x80)) goto done;
    result -= 0x80 << 7;
    b = *(ptr++); result += b << 14; if (!(b & 0x80)) goto done;
    result -= 0x80 << 14;
    b = *(ptr++); result += b << 21; if (!(b & 0x80)) goto done;
    result -= 0x80 << 21;
    b = *(ptr++); result += b << 28; if (!(b & 0x80)) goto done;
done:;
    *p = ptr;
    return result;
}

INLINE i32 __stream_read_vi32_unchecked(const u8 ** p) {
    u32 b = 0;
    u32 result = 0;
    u32 shift = 0;
    const u8 * ptr = *p;
    b = *(ptr++); result += b ; if (!(b & 0x80)) {shift = 7; goto done;}
    result -= 0x80;
    b = *(ptr++); result += b <<  7; if (!(b & 0x80)) {shift = 14; goto done;}
    result -= 0x80 << 7;
    b = *(ptr++); result += b << 14; if (!(b & 0x80)) {shift = 21; goto done;}
    result -= 0x80 << 14;
    b = *(ptr++); result += b << 21; if (!(b & 0x80)) {shift = 28; goto done;}
    result -= 0x80 << 21;
    b = *(ptr++); result += b << 28; if (!(b & 0x80)) {goto done_last_byte;}
done:;
    if (b & 0x40) {
        result |= ((u32)-1) << shift;
    }
done_last_byte:;
    *p = ptr;
    return (i32)result;
}

INLINE u64 __stream_read_vu64_unchecked(const u8 ** p) {
    u64 b = 0;
    u64 result = 0;
    const u8 * ptr = *p;
    b = *(ptr++); result += b ; if (!(b & 0x80)) goto done;
    result -= 0x80ull;
    b = *(ptr++); result += b <<  7; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 7;
    b = *(ptr++); result += b << 14; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 14;
    b = *(ptr++); result += b << 21; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 21;
    b = *(ptr++); result += b << 28; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 28;
    b = *(ptr++); result += b << 35; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 35;
    b = *(ptr++); result += b << 42; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 42;
    b = *(ptr++); result += b << 49; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 49;
    b = *(ptr++); result += b << 56; if (!(b & 0x80)) goto done;
    result -= 0x80ull << 56;
    b = *(ptr++); result += b << 63; if (!(b & 0x80)) goto done;
done:;
    *p = ptr;
    return result;
}

INLINE i64 __stream_read_vi64_unchecked(const u8 ** p) {
    u64 b = 0;
    u64 result = 0;
    u64 shift = 0;
    const u8 * ptr = *p;
    b = *(ptr++); result += b ; if (!(b & 0x80)) {shift = 7; goto done;}
    result -= 0x80ull;
    b = *(ptr++); result += b <<  7; if (!(b & 0x80)) {shift = 14; goto done;}
    result -= 0x80ull << 7;
    b = *(ptr++); result += b << 14; if (!(b & 0x80)) {shift = 21; goto done;}
    result -= 0x80ull << 14;
    b = *(ptr++); result += b << 21; if (!(b & 0x80)) {shift = 28; goto done;}
    result -= 0x80ull << 21;
    b = *(ptr++); result += b << 28; if (!(b & 0x80)) {shift = 35; goto done;}
    result -= 0x80ull << 28;
    b = *(ptr++); result += b << 35; if (!(b & 0x80)) {shift = 42; goto done;}
    result -= 0x80ull << 35;
    b = *(ptr++); result += b << 42; if (!(b & 0x80)) {shift = 49; goto done;}
    result -= 0x80ull << 42;
    b = *(ptr++); result += b << 49; if (!(b & 0x80)) {shift = 56; goto done;}
    result -= 0x80ull << 49;
    b = *(ptr++); result += b << 56; if (!(b & 0x80)) {shift = 63; goto done;}
    result -= 0x80ull << 56;
    b = *(ptr++); result += b << 63; if (!(b & 0x80)) {goto done_last_byte;}
done:;
    if (b & 0x40) {
        result |= ((u64)-1) << shift;
    }
done_last_byte:;
    *p = ptr;
    return (i64)result;
}
// clang-format on

#define stream_read_vu32_unchecked(target, p)       \
    do {                                            \
        const u8 * _p = p;                          \
        target = __stream_read_vu32_unchecked(&_p); \
        p = _p;                                     \
    } while (0)

#define stream_read_vi32_unchecked(target, p)       \
    do {                                            \
        const u8 * _p = p;                          \
        target = __stream_read_vi32_unchecked(&_p); \
        p = _p;                                     \
    } while (0)

#define stream_read_vu64_unchecked(target, p)       \
    do {                                            \
        const u8 * _p = p;                          \
        target = __stream_read_vu64_unchecked(&_p); \
        p = _p;                                     \
    } while (0)

#define stream_read_vi64_unchecked(target, p)       \
    do {                                            \
        const u8 * _p = p;                          \
        target = __stream_read_vi64_unchecked(&_p); \
        p = _p;                                     \
    } while (0)

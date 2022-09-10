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
#include "types.h"

#define MEM_READ_1B(type)                       \
    INLINE type mem_read_##type(const u8 * p) { \
        return (type)(*p);                      \
    }
MEM_READ_1B(i8)
MEM_READ_1B(u8)

#define MEM_READ_2B(type)                       \
    INLINE type mem_read_##type(const u8 * p) { \
        if (unlikely((uintptr_t)(p)&1)) {       \
            u16 v = 0;                          \
            v |= (u16)p[0];                     \
            v |= ((u16)p[1]) << 8;              \
            return *(type *)(&v);               \
        } else {                                \
            return *((type *)(p));              \
        }                                       \
    }
MEM_READ_2B(i16)
MEM_READ_2B(u16)

#define MEM_READ_4B(type)                       \
    INLINE type mem_read_##type(const u8 * p) { \
        if (unlikely((uintptr_t)(p)&3)) {       \
            u32 v = 0;                          \
            v |= (u32)p[0];                     \
            v |= ((u32)p[1]) << 8;              \
            v |= ((u32)p[2]) << 16;             \
            v |= ((u32)p[3]) << 24;             \
            return *(type *)(&v);               \
        } else {                                \
            return *((type *)(p));              \
        }                                       \
    }
MEM_READ_4B(i32)
MEM_READ_4B(u32)
MEM_READ_4B(f32)

#define MEM_READ_8B(type)                       \
    INLINE type mem_read_##type(const u8 * p) { \
        if (unlikely((uintptr_t)(p)&7)) {       \
            u64 v = 0;                          \
            v |= (u64)p[0];                     \
            v |= ((u64)p[1]) << 8;              \
            v |= ((u64)p[2]) << 16;             \
            v |= ((u64)p[3]) << 24;             \
            v |= ((u64)p[4]) << 32;             \
            v |= ((u64)p[5]) << 40;             \
            v |= ((u64)p[6]) << 48;             \
            v |= ((u64)p[7]) << 56;             \
            return *((type *)(&v));             \
        } else {                                \
            return *((type *)(p));              \
        }                                       \
    }
MEM_READ_8B(i64)
MEM_READ_8B(u64)
MEM_READ_8B(f64)

// integer truncation is not part of the c standard but all compilers
// behave the same way (afaik).
#define MEM_WRITE_DEFINE_MB(type)                            \
    INLINE void mem_write_##type(u8 * p, type val) {         \
        u32 type_size = sizeof(type);                        \
        assert(type_size > 1);                               \
        if (unlikely(((uintptr_t)(p)) & ~(type_size - 1))) { \
            while (type_size--) {                            \
                *p = ((u8)(val));                            \
                val >>= 8;                                   \
                p++;                                         \
            }                                                \
        } else {                                             \
            *((type *)(p)) = val;                            \
        }                                                    \
    }

MEM_WRITE_DEFINE_MB(u16)
MEM_WRITE_DEFINE_MB(u32)
MEM_WRITE_DEFINE_MB(u64)

INLINE void mem_write_u8(u8 * p, u8 val) {
    *p = (u8)(val);
}

INLINE void mem_write_f32(u8 * p, f32 val) {
    mem_write_u32(p, *((u32 *)(&val)));
}

INLINE void mem_write_f64(u8 * p, f64 val) {
    mem_write_u64(p, *((u64 *)(&val)));
}

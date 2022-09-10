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

#include "types.h"
#include "wasm_format.h"

// name, opcode(prefix), u32, description
#define FOR_EACH_WASM_OPCODE(macro) \
macro(unreachable                      , 0x00 , _    , unreachable                   )\
macro(nop                              , 0x01 , _    , nop                           )\
macro(block                            , 0x02 , _    , block                         )\
macro(loop                             , 0x03 , _    , loop                          )\
macro(if                               , 0x04 , _    , if                            )\
macro(else                             , 0x05 , _    , else                          )\
/* unused 0x06 - 0x0a */                                                              \
macro(end                              , 0x0b , _    , end                           )\
macro(br                               , 0x0c , _    , br                            )\
macro(br_if                            , 0x0d , _    , br_if                         )\
macro(br_table                         , 0x0e , _    , br_table                      )\
macro(return                           , 0x0f , _    , return                        )\
macro(call                             , 0x10 , _    , call                          )\
macro(call_indirect                    , 0x11 , _    , call_indirect                 )\
/* unused 0x12 - 0x19 */                                                              \
macro(drop                             , 0x1a , _    , drop                          )\
macro(select                           , 0x1b , _    , select                        )\
macro(select_t                         , 0x1c , _    , select_t                      )\
/* unused 0x1d - 0x1f */                                                              \
macro(local_get                        , 0x20 , _    , local.get                     )\
macro(local_set                        , 0x21 , _    , local.set                     )\
macro(local_tee                        , 0x22 , _    , local.tee                     )\
macro(global_get                       , 0x23 , _    , global.get                    )\
macro(global_set                       , 0x24 , _    , global.set                    )\
macro(table_get                        , 0x25 , _    , table.get                     )\
macro(table_set                        , 0x26 , _    , table.set                     )\
/* unused 0x27 */                                                              \
macro(i32_load                         , 0x28 , _    , i32.load                      )\
macro(i64_load                         , 0x29 , _    , i64.load                      )\
macro(f32_load                         , 0x2a , _    , f32.load                      )\
macro(f64_load                         , 0x2b , _    , f64.load                      )\
macro(i32_load8_s                      , 0x2c , _    , i32.load8_s                   )\
macro(i32_load8_u                      , 0x2d , _    , i32.load8_u                   )\
macro(i32_load16_s                     , 0x2e , _    , i32.load16_s                  )\
macro(i32_load16_u                     , 0x2f , _    , i32.load16_u                  )\
macro(i64_load8_s                      , 0x30 , _    , i64.load8_s                   )\
macro(i64_load8_u                      , 0x31 , _    , i64.load8_u                   )\
macro(i64_load16_s                     , 0x32 , _    , i64.load16_s                  )\
macro(i64_load16_u                     , 0x33 , _    , i64.load16_u                  )\
macro(i64_load32_s                     , 0x34 , _    , i64.load32_s                  )\
macro(i64_load32_u                     , 0x35 , _    , i64.load32_u                  )\
macro(i32_store                        , 0x36 , _    , i32.store                     )\
macro(i64_store                        , 0x37 , _    , i64.store                     )\
macro(f32_store                        , 0x38 , _    , f32.store                     )\
macro(f64_store                        , 0x39 , _    , f64.store                     )\
macro(i32_store8                       , 0x3a , _    , i32.store8                    )\
macro(i32_store16                      , 0x3b , _    , i32.store16                   )\
macro(i64_store8                       , 0x3c , _    , i64.store8                    )\
macro(i64_store16                      , 0x3d , _    , i64.store16                   )\
macro(i64_store32                      , 0x3e , _    , i64.store32                   )\
macro(memory_size                      , 0x3f , _    , memory.size                   )\
macro(memory_grow                      , 0x40 , _    , memory.grow                   )\
macro(i32_const                        , 0x41 , _    , i32.const                     )\
macro(i64_const                        , 0x42 , _    , i64.const                     )\
macro(f32_const                        , 0x43 , _    , f32.const                     )\
macro(f64_const                        , 0x44 , _    , f64.const                     )\
macro(i32_eqz                          , 0x45 , _    , i32.eqz                       )\
macro(i32_eq                           , 0x46 , _    , i32.eq                        )\
macro(i32_ne                           , 0x47 , _    , i32.ne                        )\
macro(i32_lt_s                         , 0x48 , _    , i32.lt_s                      )\
macro(i32_lt_u                         , 0x49 , _    , i32.lt_u                      )\
macro(i32_gt_s                         , 0x4a , _    , i32.gt_s                      )\
macro(i32_gt_u                         , 0x4b , _    , i32.gt_u                      )\
macro(i32_le_s                         , 0x4c , _    , i32.le_s                      )\
macro(i32_le_u                         , 0x4d , _    , i32.le_u                      )\
macro(i32_ge_s                         , 0x4e , _    , i32.ge_s                      )\
macro(i32_ge_u                         , 0x4f , _    , i32.ge_u                      )\
macro(i64_eqz                          , 0x50 , _    , i64.eqz                       )\
macro(i64_eq                           , 0x51 , _    , i64.eq                        )\
macro(i64_ne                           , 0x52 , _    , i64.ne                        )\
macro(i64_lt_s                         , 0x53 , _    , i64.lt_s                      )\
macro(i64_lt_u                         , 0x54 , _    , i64.lt_u                      )\
macro(i64_gt_s                         , 0x55 , _    , i64.gt_s                      )\
macro(i64_gt_u                         , 0x56 , _    , i64.gt_u                      )\
macro(i64_le_s                         , 0x57 , _    , i64.le_s                      )\
macro(i64_le_u                         , 0x58 , _    , i64.le_u                      )\
macro(i64_ge_s                         , 0x59 , _    , i64.ge_s                      )\
macro(i64_ge_u                         , 0x5a , _    , i64.ge_u                      )\
macro(f32_eq                           , 0x5b , _    , f32.eq                        )\
macro(f32_ne                           , 0x5c , _    , f32.ne                        )\
macro(f32_lt                           , 0x5d , _    , f32.lt                        )\
macro(f32_gt                           , 0x5e , _    , f32.gt                        )\
macro(f32_le                           , 0x5f , _    , f32.le                        )\
macro(f32_ge                           , 0x60 , _    , f32.ge                        )\
macro(f64_eq                           , 0x61 , _    , f64.eq                        )\
macro(f64_ne                           , 0x62 , _    , f64.ne                        )\
macro(f64_lt                           , 0x63 , _    , f64.lt                        )\
macro(f64_gt                           , 0x64 , _    , f64.gt                        )\
macro(f64_le                           , 0x65 , _    , f64.le                        )\
macro(f64_ge                           , 0x66 , _    , f64.ge                        )\
macro(i32_clz                          , 0x67 , _    , i32.clz                       )\
macro(i32_ctz                          , 0x68 , _    , i32.ctz                       )\
macro(i32_popcnt                       , 0x69 , _    , i32.popcnt                    )\
macro(i32_add                          , 0x6a , _    , i32.add                       )\
macro(i32_sub                          , 0x6b , _    , i32.sub                       )\
macro(i32_mul                          , 0x6c , _    , i32.mul                       )\
macro(i32_div_s                        , 0x6d , _    , i32.div_s                     )\
macro(i32_div_u                        , 0x6e , _    , i32.div_u                     )\
macro(i32_rem_s                        , 0x6f , _    , i32.rem_s                     )\
macro(i32_rem_u                        , 0x70 , _    , i32.rem_u                     )\
macro(i32_and                          , 0x71 , _    , i32.and                       )\
macro(i32_or                           , 0x72 , _    , i32.or                        )\
macro(i32_xor                          , 0x73 , _    , i32.xor                       )\
macro(i32_shl                          , 0x74 , _    , i32.shl                       )\
macro(i32_shr_s                        , 0x75 , _    , i32.shr_s                     )\
macro(i32_shr_u                        , 0x76 , _    , i32.shr_u                     )\
macro(i32_rotl                         , 0x77 , _    , i32.rotl                      )\
macro(i32_rotr                         , 0x78 , _    , i32.rotr                      )\
macro(i64_clz                          , 0x79 , _    , i64.clz                       )\
macro(i64_ctz                          , 0x7a , _    , i64.ctz                       )\
macro(i64_popcnt                       , 0x7b , _    , i64.popcnt                    )\
macro(i64_add                          , 0x7c , _    , i64.add                       )\
macro(i64_sub                          , 0x7d , _    , i64.sub                       )\
macro(i64_mul                          , 0x7e , _    , i64.mul                       )\
macro(i64_div_s                        , 0x7f , _    , i64.div_s                     )\
macro(i64_div_u                        , 0x80 , _    , i64.div_u                     )\
macro(i64_rem_s                        , 0x81 , _    , i64.rem_s                     )\
macro(i64_rem_u                        , 0x82 , _    , i64.rem_u                     )\
macro(i64_and                          , 0x83 , _    , i64.and                       )\
macro(i64_or                           , 0x84 , _    , i64.or                        )\
macro(i64_xor                          , 0x85 , _    , i64.xor                       )\
macro(i64_shl                          , 0x86 , _    , i64.shl                       )\
macro(i64_shr_s                        , 0x87 , _    , i64.shr_s                     )\
macro(i64_shr_u                        , 0x88 , _    , i64.shr_u                     )\
macro(i64_rotl                         , 0x89 , _    , i64.rotl                      )\
macro(i64_rotr                         , 0x8a , _    , i64.rotr                      )\
macro(f32_abs                          , 0x8b , _    , f32.abs                       )\
macro(f32_neg                          , 0x8c , _    , f32.neg                       )\
macro(f32_ceil                         , 0x8d , _    , f32.ceil                      )\
macro(f32_floor                        , 0x8e , _    , f32.floor                     )\
macro(f32_trunc                        , 0x8f , _    , f32.trunc                     )\
macro(f32_nearest                      , 0x90 , _    , f32.nearest                   )\
macro(f32_sqrt                         , 0x91 , _    , f32.sqrt                      )\
macro(f32_add                          , 0x92 , _    , f32.add                       )\
macro(f32_sub                          , 0x93 , _    , f32.sub                       )\
macro(f32_mul                          , 0x94 , _    , f32.mul                       )\
macro(f32_div                          , 0x95 , _    , f32.div                       )\
macro(f32_min                          , 0x96 , _    , f32.min                       )\
macro(f32_max                          , 0x97 , _    , f32.max                       )\
macro(f32_copysign                     , 0x98 , _    , f32.copysign                  )\
macro(f64_abs                          , 0x99 , _    , f64.abs                       )\
macro(f64_neg                          , 0x9a , _    , f64.neg                       )\
macro(f64_ceil                         , 0x9b , _    , f64.ceil                      )\
macro(f64_floor                        , 0x9c , _    , f64.floor                     )\
macro(f64_trunc                        , 0x9d , _    , f64.trunc                     )\
macro(f64_nearest                      , 0x9e , _    , f64.nearest                   )\
macro(f64_sqrt                         , 0x9f , _    , f64.sqrt                      )\
macro(f64_add                          , 0xa0 , _    , f64.add                       )\
macro(f64_sub                          , 0xa1 , _    , f64.sub                       )\
macro(f64_mul                          , 0xa2 , _    , f64.mul                       )\
macro(f64_div                          , 0xa3 , _    , f64.div                       )\
macro(f64_min                          , 0xa4 , _    , f64.min                       )\
macro(f64_max                          , 0xa5 , _    , f64.max                       )\
macro(f64_copysign                     , 0xa6 , _    , f64.copysign                  )\
macro(i32_wrap_i64                     , 0xa7 , _    , i32.wrap_i64                  )\
macro(i32_trunc_f32_s                  , 0xa8 , _    , i32.trunc_f32_s               )\
macro(i32_trunc_f32_u                  , 0xa9 , _    , i32.trunc_f32_u               )\
macro(i32_trunc_f64_s                  , 0xaa , _    , i32.trunc_f64_s               )\
macro(i32_trunc_f64_u                  , 0xab , _    , i32.trunc_f64_u               )\
macro(i64_extend_i32_s                 , 0xac , _    , i64.extend_i32_s              )\
macro(i64_extend_i32_u                 , 0xad , _    , i64.extend_i32_u              )\
macro(i64_trunc_f32_s                  , 0xae , _    , i64.trunc_f32_s               )\
macro(i64_trunc_f32_u                  , 0xaf , _    , i64.trunc_f32_u               )\
macro(i64_trunc_f64_s                  , 0xb0 , _    , i64.trunc_f64_s               )\
macro(i64_trunc_f64_u                  , 0xb1 , _    , i64.trunc_f64_u               )\
macro(f32_convert_i32_s                , 0xb2 , _    , f32.convert_i32_s             )\
macro(f32_convert_i32_u                , 0xb3 , _    , f32.convert_i32_u             )\
macro(f32_convert_i64_s                , 0xb4 , _    , f32.convert_i64_s             )\
macro(f32_convert_i64_u                , 0xb5 , _    , f32.convert_i64_u             )\
macro(f32_demote_f64                   , 0xb6 , _    , f32.demote_f64                )\
macro(f64_convert_i32_s                , 0xb7 , _    , f64.convert_i32_s             )\
macro(f64_convert_i32_u                , 0xb8 , _    , f64.convert_i32_u             )\
macro(f64_convert_i64_s                , 0xb9 , _    , f64.convert_i64_s             )\
macro(f64_convert_i64_u                , 0xba , _    , f64.convert_i64_u             )\
macro(f64_promote_f32                  , 0xbb , _    , f64.promote_f32               )\
macro(i32_reinterpret_f32              , 0xbc , _    , i32.reinterpret_f32           )\
macro(i64_reinterpret_f64              , 0xbd , _    , i64.reinterpret_f64           )\
macro(f32_reinterpret_i32              , 0xbe , _    , f32.reinterpret_i32           )\
macro(f64_reinterpret_i64              , 0xbf , _    , f64.reinterpret_i64           )\
macro(i32_extend8_s                    , 0xc0 , _    , i32.extend8_s                 )\
macro(i32_extend16_s                   , 0xc1 , _    , i32.extend16_s                )\
macro(i64_extend8_s                    , 0xc2 , _    , i64.extend8_s                 )\
macro(i64_extend16_s                   , 0xc3 , _    , i64.extend16_s                )\
macro(i64_extend32_s                   , 0xc4 , _    , i64.extend32_s                )\
/* unused 0xc5 - 0xcf */                                                              \
macro(ref_null                         , 0xd0 , _    , ref.null                      )\
macro(ref_is_null                      , 0xd1 , _    , ref.is_null                   )\
macro(ref_func                         , 0xd2 , _    , ref.func                      )\
/* unused 0xd3 - 0xfb */                                                              \
macro(prefix_fc                        , 0xfc , _    , prefix_fc                     )\
macro(prefix_fd                        , 0xfd , _    , prefix_fd                     )\
/* unused 0xfe - 0xff */

#define FOR_EACH_WASM_OPCODE_FC(macro) \
macro(i32_trunc_sat_f32_s              , 0xfc , 0x00 , i32.trunc_sat_f32_s           )\
macro(i32_trunc_sat_f32_u              , 0xfc , 0x01 , i32.trunc_sat_f32_u           )\
macro(i32_trunc_sat_f64_s              , 0xfc , 0x02 , i32.trunc_sat_f64_s           )\
macro(i32_trunc_sat_f64_u              , 0xfc , 0x03 , i32.trunc_sat_f64_u           )\
macro(i64_trunc_sat_f32_s              , 0xfc , 0x04 , i64.trunc_sat_f32_s           )\
macro(i64_trunc_sat_f32_u              , 0xfc , 0x05 , i64.trunc_sat_f32_u           )\
macro(i64_trunc_sat_f64_s              , 0xfc , 0x06 , i64.trunc_sat_f64_s           )\
macro(i64_trunc_sat_f64_u              , 0xfc , 0x07 , i64.trunc_sat_f64_u           )\
macro(memory_init                      , 0xfc , 0x08 , memory.init                   )\
macro(data_drop                        , 0xfc , 0x09 , data.drop                     )\
macro(memory_copy                      , 0xfc , 0x0a , memory.copy                   )\
macro(memory_fill                      , 0xfc , 0x0b , memory.fill                   )\
macro(table_init                       , 0xfc , 0x0c , table.init                    )\
macro(elem_drop                        , 0xfc , 0x0d , elem.drop                     )\
macro(table_copy                       , 0xfc , 0x0e , table.copy                    )\
macro(table_grow                       , 0xfc , 0x0f , table.grow                    )\
macro(table_size                       , 0xfc , 0x10 , table.size                    )\
macro(table_fill                       , 0xfc , 0x11 , table.fill                    )

#define FOR_EACH_WASM_OPCODE_FD(macro) \
macro(v128_load                        , 0xfd , 0x00 ,  v128.load                    )\
macro(v128_load8x8_s                   , 0xfd , 0x01 , v128.load8x8_s                )\
macro(v128_load8x8_u                   , 0xfd , 0x02 , v128.load8x8_u                )\
macro(v128_load16x4_s                  , 0xfd , 0x03 , v128.load16x4_s               )\
macro(v128_load16x4_u                  , 0xfd , 0x04 , v128.load16x4_u               )\
macro(v128_load32x2_s                  , 0xfd , 0x05 , v128.load32x2_s               )\
macro(v128_load32x2_u                  , 0xfd , 0x06 , v128.load32x2_u               )\
macro(v128_load8_splat                 , 0xfd , 0x07 , v128.load8_splat              )\
macro(v128_load16_splat                , 0xfd , 0x08 , v128.load16_splat             )\
macro(v128_load32_splat                , 0xfd , 0x09 , v128.load32_splat             )\
macro(v128_load64_splat                , 0xfd , 0x0a , v128.load64_splat             )\
macro(v128_store                       , 0xfd , 0x0b , v128.store                    )\
macro(v128_const                       , 0xfd , 0x0c , v128.const                    )\
macro(i8x16_shuffle                    , 0xfd , 0x0d , i8x16.shuffle                 )\
macro(i8x16_swizzle                    , 0xfd , 0x0e , i8x16.swizzle                 )\
macro(i8x16_splat                      , 0xfd , 0x0f , i8x16.splat                   )\
macro(i16x8_splat                      , 0xfd , 0x10 , i16x8.splat                   )\
macro(i32x4_splat                      , 0xfd , 0x11 , i32x4.splat                   )\
macro(i64x2_splat                      , 0xfd , 0x12 , i64x2.splat                   )\
macro(f32x4_splat                      , 0xfd , 0x13 , f32x4.splat                   )\
macro(f64x2_splat                      , 0xfd , 0x14 , f64x2.splat                   )\
macro(i8x16_extract_lane_s             , 0xfd , 0x15 , i8x16.extract_lane_s          )\
macro(i8x16_extract_lane_u             , 0xfd , 0x16 , i8x16.extract_lane_u          )\
macro(i8x16_replace_lane               , 0xfd , 0x17 , i8x16.replace_lane            )\
macro(i16x8_extract_lane_s             , 0xfd , 0x18 , i16x8.extract_lane_s          )\
macro(i16x8_extract_lane_u             , 0xfd , 0x19 , i16x8.extract_lane_u          )\
macro(i16x8_replace_lane               , 0xfd , 0x1a , i16x8.replace_lane            )\
macro(i32x4_extract_lane               , 0xfd , 0x1b , i32x4.extract_lane            )\
macro(i32x4_replace_lane               , 0xfd , 0x1c , i32x4.replace_lane            )\
macro(i64x2_extract_lane               , 0xfd , 0x1d , i64x2.extract_lane            )\
macro(i64x2_replace_lane               , 0xfd , 0x1e , i64x2.replace_lane            )\
macro(f32x4_extract_lane               , 0xfd , 0x1f , f32x4.extract_lane            )\
macro(f32x4_replace_lane               , 0xfd , 0x20 , f32x4.replace_lane            )\
macro(f64x2_extract_lane               , 0xfd , 0x21 , f64x2.extract_lane            )\
macro(f64x2_replace_lane               , 0xfd , 0x22 , f64x2.replace_lane            )\
macro(i8x16_eq                         , 0xfd , 0x23 , i8x16.eq                      )\
macro(i8x16_ne                         , 0xfd , 0x24 , i8x16.ne                      )\
macro(i8x16_lt_s                       , 0xfd , 0x25 , i8x16.lt_s                    )\
macro(i8x16_lt_u                       , 0xfd , 0x26 , i8x16.lt_u                    )\
macro(i8x16_gt_s                       , 0xfd , 0x27 , i8x16.gt_s                    )\
macro(i8x16_gt_u                       , 0xfd , 0x28 , i8x16.gt_u                    )\
macro(i8x16_le_s                       , 0xfd , 0x29 , i8x16.le_s                    )\
macro(i8x16_le_u                       , 0xfd , 0x2a , i8x16.le_u                    )\
macro(i8x16_ge_s                       , 0xfd , 0x2b , i8x16.ge_s                    )\
macro(i8x16_ge_u                       , 0xfd , 0x2c , i8x16.ge_u                    )\
macro(i16x8_eq                         , 0xfd , 0x2d , i16x8.eq                      )\
macro(i16x8_ne                         , 0xfd , 0x2e , i16x8.ne                      )\
macro(i16x8_lt_s                       , 0xfd , 0x2f , i16x8.lt_s                    )\
macro(i16x8_lt_u                       , 0xfd , 0x30 , i16x8.lt_u                    )\
macro(i16x8_gt_s                       , 0xfd , 0x31 , i16x8.gt_s                    )\
macro(i16x8_gt_u                       , 0xfd , 0x32 , i16x8.gt_u                    )\
macro(i16x8_le_s                       , 0xfd , 0x33 , i16x8.le_s                    )\
macro(i16x8_le_u                       , 0xfd , 0x34 , i16x8.le_u                    )\
macro(i16x8_ge_s                       , 0xfd , 0x35 , i16x8.ge_s                    )\
macro(i16x8_ge_u                       , 0xfd , 0x36 , i16x8.ge_u                    )\
macro(i32x4_eq                         , 0xfd , 0x37 , i32x4.eq                      )\
macro(i32x4_ne                         , 0xfd , 0x38 , i32x4.ne                      )\
macro(i32x4_lt_s                       , 0xfd , 0x39 , i32x4.lt_s                    )\
macro(i32x4_lt_u                       , 0xfd , 0x3a , i32x4.lt_u                    )\
macro(i32x4_gt_s                       , 0xfd , 0x3b , i32x4.gt_s                    )\
macro(i32x4_gt_u                       , 0xfd , 0x3c , i32x4.gt_u                    )\
macro(i32x4_le_s                       , 0xfd , 0x3d , i32x4.le_s                    )\
macro(i32x4_le_u                       , 0xfd , 0x3e , i32x4.le_u                    )\
macro(i32x4_ge_s                       , 0xfd , 0x3f , i32x4.ge_s                    )\
macro(i32x4_ge_u                       , 0xfd , 0x40 , i32x4.ge_u                    )\
macro(f32x4_eq                         , 0xfd , 0x41 , f32x4.eq                      )\
macro(f32x4_ne                         , 0xfd , 0x42 , f32x4.ne                      )\
macro(f32x4_lt                         , 0xfd , 0x43 , f32x4.lt                      )\
macro(f32x4_gt                         , 0xfd , 0x44 , f32x4.gt                      )\
macro(f32x4_le                         , 0xfd , 0x45 , f32x4.le                      )\
macro(f32x4_ge                         , 0xfd , 0x46 , f32x4.ge                      )\
macro(f64x2_eq                         , 0xfd , 0x47 , f64x2.eq                      )\
macro(f64x2_ne                         , 0xfd , 0x48 , f64x2.ne                      )\
macro(f64x2_lt                         , 0xfd , 0x49 , f64x2.lt                      )\
macro(f64x2_gt                         , 0xfd , 0x4a , f64x2.gt                      )\
macro(f64x2_le                         , 0xfd , 0x4b , f64x2.le                      )\
macro(f64x2_ge                         , 0xfd , 0x4c , f64x2.ge                      )\
macro(v128_not                         , 0xfd , 0x4d , v128.not                      )\
macro(v128_and                         , 0xfd , 0x4e , v128.and                      )\
macro(v128_andnot                      , 0xfd , 0x4f , v128.andnot                   )\
macro(v128_or                          , 0xfd , 0x50 , v128.or                       )\
macro(v128_xor                         , 0xfd , 0x51 , v128.xor                      )\
macro(v128_bitselect                   , 0xfd , 0x52 , v128.bitselect                )\
macro(v128_any_true                    , 0xfd , 0x53 , v128.any_true                 )\
macro(v128_load8_lane                  , 0xfd , 0x54 , v128.load8_lane               )\
macro(v128_load16_lane                 , 0xfd , 0x55 , v128.load16_lane              )\
macro(v128_load32_lane                 , 0xfd , 0x56 , v128.load32_lane              )\
macro(v128_load64_lane                 , 0xfd , 0x57 , v128.load64_lane              )\
macro(v128_store8_lane                 , 0xfd , 0x58 , v128.store8_lane              )\
macro(v128_store16_lane                , 0xfd , 0x59 , v128.store16_lane             )\
macro(v128_store32_lane                , 0xfd , 0x5a , v128.store32_lane             )\
macro(v128_store64_lane                , 0xfd , 0x5b , v128.store64_lane             )\
macro(v128_load32_zero                 , 0xfd , 0x5c , v128.load32_zero              )\
macro(v128_load64_zero                 , 0xfd , 0x5d , v128.load64_zero              )\
macro(f32x4_demote_f64x2_zero          , 0xfd , 0x5e , f32x4.demote_f64x2_zero       )\
macro(f64x2_promote_low_f32x4          , 0xfd , 0x5f , f64x2.promote_low_f32x4       )\
macro(i8x16_abs                        , 0xfd , 0x60 , i8x16.abs                     )\
macro(i8x16_neg                        , 0xfd , 0x61 , i8x16.neg                     )\
macro(i8x16_popcnt                     , 0xfd , 0x62 , i8x16.popcnt                  )\
macro(i8x16_all_true                   , 0xfd , 0x63 , i8x16.all_true                )\
macro(i8x16_bitmask                    , 0xfd , 0x64 , i8x16.bitmask                 )\
macro(i8x16_narrow_i16x8_s             , 0xfd , 0x65 , i8x16.narrow_i16x8_s          )\
macro(i8x16_narrow_i16x8_u             , 0xfd , 0x66 , i8x16.narrow_i16x8_u          )\
macro(i8x16_shl                        , 0xfd , 0x6b , i8x16.shl                     )\
macro(i8x16_shr_s                      , 0xfd , 0x6c , i8x16.shr_s                   )\
macro(i8x16_shr_u                      , 0xfd , 0x6d , i8x16.shr_u                   )\
macro(i8x16_add                        , 0xfd , 0x6e , i8x16.add                     )\
macro(i8x16_add_sat_s                  , 0xfd , 0x6f , i8x16.add_sat_s               )\
macro(i8x16_add_sat_u                  , 0xfd , 0x70 , i8x16.add_sat_u               )\
macro(i8x16_sub                        , 0xfd , 0x71 , i8x16.sub                     )\
macro(i8x16_sub_sat_s                  , 0xfd , 0x72 , i8x16.sub_sat_s               )\
macro(i8x16_sub_sat_u                  , 0xfd , 0x73 , i8x16.sub_sat_u               )\
macro(i8x16_min_s                      , 0xfd , 0x76 , i8x16.min_s                   )\
macro(i8x16_min_u                      , 0xfd , 0x77 , i8x16.min_u                   )\
macro(i8x16_max_s                      , 0xfd , 0x78 , i8x16.max_s                   )\
macro(i8x16_max_u                      , 0xfd , 0x79 , i8x16.max_u                   )\
macro(i8x16_avgr_u                     , 0xfd , 0x7b , i8x16.avgr_u                  )\
macro(i16x8_extadd_pairwise_i8x16_s    , 0xfd , 0x7c , i16x8.extadd_pairwise_i8x16_s )\
macro(i16x8_extadd_pairwise_i8x16_u    , 0xfd , 0x7d , i16x8.extadd_pairwise_i8x16_u )\
macro(i32x4_extadd_pairwise_i16x8_s    , 0xfd , 0x7e , i32x4.extadd_pairwise_i16x8_s )\
macro(i32x4_extadd_pairwise_i16x8_u    , 0xfd , 0x7f , i32x4.extadd_pairwise_i16x8_u )\
macro(i16x8_abs                        , 0xfd , 0x80 , i16x8.abs                     )\
macro(i16x8_neg                        , 0xfd , 0x81 , i16x8.neg                     )\
macro(i16x8_q15mulr_sat_s              , 0xfd , 0x82 , i16x8.q15mulr_sat_s           )\
macro(i16x8_all_true                   , 0xfd , 0x83 , i16x8.all_true                )\
macro(i16x8_bitmask                    , 0xfd , 0x84 , i16x8.bitmask                 )\
macro(i16x8_narrow_i32x4_s             , 0xfd , 0x85 , i16x8.narrow_i32x4_s          )\
macro(i16x8_narrow_i32x4_u             , 0xfd , 0x86 , i16x8.narrow_i32x4_u          )\
macro(i16x8_extend_low_i8x16_s         , 0xfd , 0x87 , i16x8.extend_low_i8x16_s      )\
macro(i16x8_extend_high_i8x16_s        , 0xfd , 0x88 , i16x8.extend_high_i8x16_s     )\
macro(i16x8_extend_low_i8x16_u         , 0xfd , 0x89 , i16x8.extend_low_i8x16_u      )\
macro(i16x8_extend_high_i8x16_u        , 0xfd , 0x8a , i16x8.extend_high_i8x16_u     )\
macro(i16x8_shl                        , 0xfd , 0x8b , i16x8.shl                     )\
macro(i16x8_shr_s                      , 0xfd , 0x8c , i16x8.shr_s                   )\
macro(i16x8_shr_u                      , 0xfd , 0x8d , i16x8.shr_u                   )\
macro(i16x8_add                        , 0xfd , 0x8e , i16x8.add                     )\
macro(i16x8_add_sat_s                  , 0xfd , 0x8f , i16x8.add_sat_s               )\
macro(i16x8_add_sat_u                  , 0xfd , 0x90 , i16x8.add_sat_u               )\
macro(i16x8_sub                        , 0xfd , 0x91 , i16x8.sub                     )\
macro(i16x8_sub_sat_s                  , 0xfd , 0x92 , i16x8.sub_sat_s               )\
macro(i16x8_sub_sat_u                  , 0xfd , 0x93 , i16x8.sub_sat_u               )\
macro(i16x8_mul                        , 0xfd , 0x95 , i16x8.mul                     )\
macro(i16x8_min_s                      , 0xfd , 0x96 , i16x8.min_s                   )\
macro(i16x8_min_u                      , 0xfd , 0x97 , i16x8.min_u                   )\
macro(i16x8_max_s                      , 0xfd , 0x98 , i16x8.max_s                   )\
macro(i16x8_max_u                      , 0xfd , 0x99 , i16x8.max_u                   )\
macro(i16x8_avgr_u                     , 0xfd , 0x9b , i16x8.avgr_u                  )\
macro(i16x8_extmul_low_i8x16_s         , 0xfd , 0x9c , i16x8.extmul_low_i8x16_s      )\
macro(i16x8_extmul_high_i8x16_s        , 0xfd , 0x9d , i16x8.extmul_high_i8x16_s     )\
macro(i16x8_extmul_low_i8x16_u         , 0xfd , 0x9e , i16x8.extmul_low_i8x16_u      )\
macro(i16x8_extmul_high_i8x16_u        , 0xfd , 0x9f , i16x8.extmul_high_i8x16_u     )\
macro(i32x4_abs                        , 0xfd , 0xa0 , i32x4.abs                     )\
macro(i32x4_neg                        , 0xfd , 0xa1 , i32x4.neg                     )\
macro(i32x4_all_true                   , 0xfd , 0xa3 , i32x4.all_true                )\
macro(i32x4_bitmask                    , 0xfd , 0xa4 , i32x4.bitmask                 )\
macro(i32x4_extend_low_i16x8_s         , 0xfd , 0xa7 , i32x4.extend_low_i16x8_s      )\
macro(i32x4_extend_high_i16x8_s        , 0xfd , 0xa8 , i32x4.extend_high_i16x8_s     )\
macro(i32x4_extend_low_i16x8_u         , 0xfd , 0xa9 , i32x4.extend_low_i16x8_u      )\
macro(i32x4_extend_high_i16x8_u        , 0xfd , 0xaa , i32x4.extend_high_i16x8_u     )\
macro(i32x4_shl                        , 0xfd , 0xab , i32x4.shl                     )\
macro(i32x4_shr_s                      , 0xfd , 0xac , i32x4.shr_s                   )\
macro(i32x4_shr_u                      , 0xfd , 0xad , i32x4.shr_u                   )\
macro(i32x4_add                        , 0xfd , 0xae , i32x4.add                     )\
macro(i32x4_sub                        , 0xfd , 0xb1 , i32x4.sub                     )\
macro(i32x4_mul                        , 0xfd , 0xb5 , i32x4.mul                     )\
macro(i32x4_min_s                      , 0xfd , 0xb6 , i32x4.min_s                   )\
macro(i32x4_min_u                      , 0xfd , 0xb7 , i32x4.min_u                   )\
macro(i32x4_max_s                      , 0xfd , 0xb8 , i32x4.max_s                   )\
macro(i32x4_max_u                      , 0xfd , 0xb9 , i32x4.max_u                   )\
macro(i32x4_dot_i16x8_s                , 0xfd , 0xba , i32x4.dot_i16x8_s             )\
macro(i32x4_extmul_low_i16x8_s         , 0xfd , 0xbc , i32x4.extmul_low_i16x8_s      )\
macro(i32x4_extmul_high_i16x8_s        , 0xfd , 0xbd , i32x4.extmul_high_i16x8_s     )\
macro(i32x4_extmul_low_i16x8_u         , 0xfd , 0xbe , i32x4.extmul_low_i16x8_u      )\
macro(i32x4_extmul_high_i16x8_u        , 0xfd , 0xbf , i32x4.extmul_high_i16x8_u     )\
macro(i64x2_abs                        , 0xfd , 0xc0 , i64x2.abs                     )\
macro(i64x2_neg                        , 0xfd , 0xc1 , i64x2.neg                     )\
macro(i64x2_all_true                   , 0xfd , 0xc3 , i64x2.all_true                )\
macro(i64x2_bitmask                    , 0xfd , 0xc4 , i64x2.bitmask                 )\
macro(i64x2_extend_low_i32x4_s         , 0xfd , 0xc7 , i64x2.extend_low_i32x4_s      )\
macro(i64x2_extend_high_i32x4_s        , 0xfd , 0xc8 , i64x2.extend_high_i32x4_s     )\
macro(i64x2_extend_low_i32x4_u         , 0xfd , 0xc9 , i64x2.extend_low_i32x4_u      )\
macro(i64x2_extend_high_i32x4_u        , 0xfd , 0xca , i64x2.extend_high_i32x4_u     )\
macro(i64x2_shl                        , 0xfd , 0xcb , i64x2.shl                     )\
macro(i64x2_shr_s                      , 0xfd , 0xcc , i64x2.shr_s                   )\
macro(i64x2_shr_u                      , 0xfd , 0xcd , i64x2.shr_u                   )\
macro(i64x2_add                        , 0xfd , 0xce , i64x2.add                     )\
macro(i64x2_sub                        , 0xfd , 0xd1 , i64x2.sub                     )\
macro(i64x2_mul                        , 0xfd , 0xd5 , i64x2.mul                     )\
macro(i64x2_eq                         , 0xfd , 0xd6 , i64x2.eq                      )\
macro(i64x2_ne                         , 0xfd , 0xd7 , i64x2.ne                      )\
macro(i64x2_lt_s                       , 0xfd , 0xd8 , i64x2.lt_s                    )\
macro(i64x2_gt_s                       , 0xfd , 0xd9 , i64x2.gt_s                    )\
macro(i64x2_le_s                       , 0xfd , 0xda , i64x2.le_s                    )\
macro(i64x2_ge_s                       , 0xfd , 0xdb , i64x2.ge_s                    )\
macro(i64x2_extmul_low_i32x4_s         , 0xfd , 0xdc , i64x2.extmul_low_i32x4_s      )\
macro(i64x2_extmul_high_i32x4_s        , 0xfd , 0xdd , i64x2.extmul_high_i32x4_s     )\
macro(i64x2_extmul_low_i32x4_u         , 0xfd , 0xde , i64x2.extmul_low_i32x4_u      )\
macro(i64x2_extmul_high_i32x4_u        , 0xfd , 0xdf , i64x2.extmul_high_i32x4_u     )\
macro(f32x4_ceil                       , 0xfd , 0x67 , f32x4.ceil                    )\
macro(f32x4_floor                      , 0xfd , 0x68 , f32x4.floor                   )\
macro(f32x4_trunc                      , 0xfd , 0x69 , f32x4.trunc                   )\
macro(f32x4_nearest                    , 0xfd , 0x6a , f32x4.nearest                 )\
macro(f64x2_ceil                       , 0xfd , 0x74 , f64x2.ceil                    )\
macro(f64x2_floor                      , 0xfd , 0x75 , f64x2.floor                   )\
macro(f64x2_trunc                      , 0xfd , 0x7a , f64x2.trunc                   )\
macro(f64x2_nearest                    , 0xfd , 0x94 , f64x2.nearest                 )\
macro(f32x4_abs                        , 0xfd , 0xe0 , f32x4.abs                     )\
macro(f32x4_neg                        , 0xfd , 0xe1 , f32x4.neg                     )\
macro(f32x4_sqrt                       , 0xfd , 0xe3 , f32x4.sqrt                    )\
macro(f32x4_add                        , 0xfd , 0xe4 , f32x4.add                     )\
macro(f32x4_sub                        , 0xfd , 0xe5 , f32x4.sub                     )\
macro(f32x4_mul                        , 0xfd , 0xe6 , f32x4.mul                     )\
macro(f32x4_div                        , 0xfd , 0xe7 , f32x4.div                     )\
macro(f32x4_min                        , 0xfd , 0xe8 , f32x4.min                     )\
macro(f32x4_max                        , 0xfd , 0xe9 , f32x4.max                     )\
macro(f32x4_pmin                       , 0xfd , 0xea , f32x4.pmin                    )\
macro(f32x4_pmax                       , 0xfd , 0xeb , f32x4.pmax                    )\
macro(f64x2_abs                        , 0xfd , 0xec , f64x2.abs                     )\
macro(f64x2_neg                        , 0xfd , 0xed , f64x2.neg                     )\
macro(f64x2_sqrt                       , 0xfd , 0xef , f64x2.sqrt                    )\
macro(f64x2_add                        , 0xfd , 0xf0 , f64x2.add                     )\
macro(f64x2_sub                        , 0xfd , 0xf1 , f64x2.sub                     )\
macro(f64x2_mul                        , 0xfd , 0xf2 , f64x2.mul                     )\
macro(f64x2_div                        , 0xfd , 0xf3 , f64x2.div                     )\
macro(f64x2_min                        , 0xfd , 0xf4 , f64x2.min                     )\
macro(f64x2_max                        , 0xfd , 0xf5 , f64x2.max                     )\
macro(f64x2_pmin                       , 0xfd , 0xf6 , f64x2.pmin                    )\
macro(f64x2_pmax                       , 0xfd , 0xf7 , f64x2.pmax                    )\
macro(i32x4_trunc_sat_f32x4_s          , 0xfd , 0xf8 , i32x4.trunc_sat_f32x4_s       )\
macro(i32x4_trunc_sat_f32x4_u          , 0xfd , 0xf9 , i32x4.trunc_sat_f32x4_u       )\
macro(f32x4_convert_i32x4_s            , 0xfd , 0xfa , f32x4.convert_i32x4_s         )\
macro(f32x4_convert_i32x4_u            , 0xfd , 0xfb , f32x4.convert_i32x4_u         )\
macro(i32x4_trunc_sat_f64x2_s_zero     , 0xfd , 0xfc , i32x4.trunc_sat_f64x2_s_zero  )\
macro(i32x4_trunc_sat_f64x2_u_zero     , 0xfd , 0xfd , i32x4.trunc_sat_f64x2_u_zero  )\
macro(f64x2_convert_low_i32x4_s        , 0xfd , 0xfe , f64x2.convert_low_i32x4_s     )\
macro(f64x2_convert_low_i32x4_u        , 0xfd , 0xff , f64x2.convert_low_i32x4_u     )

#define DEFINE_OPCODE(label, b1, b2, name) op_##label = (u8)(b1),
typedef enum wasm_opcode {
    FOR_EACH_WASM_OPCODE(DEFINE_OPCODE)
} wasm_opcode;

#define DEFINE_OPCODE_FC(label, b1, b2, name) op_##label = (u8)(b2),
typedef enum wasm_opcode_fc {
    FOR_EACH_WASM_OPCODE_FC(DEFINE_OPCODE_FC)
} wasm_opcode_fc;

#define DEFINE_OPCODE_FD(label, b1, b2, name) op_##label = (u32)(b2),
typedef enum wasm_opcode_fd {
    FOR_EACH_WASM_OPCODE_FD(DEFINE_OPCODE_FD)
} wasm_opcode_fd;

const char * get_op_name(wasm_opcode op);
const char * get_op_fc_name(wasm_opcode_fc op);
const char * get_op_fd_name(wasm_opcode_fd op);


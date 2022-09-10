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

#include "opcode.h"

#define OPCODE_NAME(label, b1, b2, name) [b1] = #name,
static const char * const op_names[256] = {
    FOR_EACH_WASM_OPCODE(OPCODE_NAME)
};

const char * get_op_name(wasm_opcode op) {
    return op_names[op];
}

#define OPCODE_FC_NAME(label, b1, b2, name) [b2] = #name,
static const char * const op_fc_names[256] = {
    FOR_EACH_WASM_OPCODE_FC(OPCODE_FC_NAME)
};

const char * get_op_fc_name(wasm_opcode_fc op) {
    return op_fc_names[op];
}

#define OPCODE_FD_NAME(label, b1, b2, name) [b2] = #name,
static const char * const op_fd_names[256] = {
    FOR_EACH_WASM_OPCODE_FD(OPCODE_FD_NAME)
};

const char * get_op_fd_name(wasm_opcode_fd op) {
    return op_fd_names[op];
}


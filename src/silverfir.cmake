#
# Copyright 2022 Bai Ming
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#     http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.


set(silverfir_src_dir ${PROJECT_SOURCE_DIR}/src)

list(APPEND silverfir_sources
    ${silverfir_src_dir}/common/op_decoder.c
    ${silverfir_src_dir}/common/opcode.c
    ${silverfir_src_dir}/common/wasm_format.c
    ${silverfir_src_dir}/host/host_modules.c
    ${silverfir_src_dir}/interpreter/const_expr_eval.c
    ${silverfir_src_dir}/interpreter/in_place_dt.c
    ${silverfir_src_dir}/interpreter/in_place_tco.c
    ${silverfir_src_dir}/interpreter/interpreter.c
    ${silverfir_src_dir}/jit/ir_builder.c
    ${silverfir_src_dir}/runtime/module.c
    ${silverfir_src_dir}/runtime/parser.c
    ${silverfir_src_dir}/runtime/runtime.c
    ${silverfir_src_dir}/runtime/validator.c
    ${silverfir_src_dir}/runtime/vm.c
    ${silverfir_src_dir}/utils/containers_impl.c
    ${silverfir_src_dir}/utils/logger.c
    ${silverfir_src_dir}/utils/sjson.c
    ${silverfir_src_dir}/utils/str.c
    ${silverfir_src_dir}/utils/utf8.c
    ${silverfir_src_dir}/utils/vstr.c
)

list(APPEND silverfir_private_includes
    ${silverfir_src_dir}/common
    ${silverfir_src_dir}/host
    ${silverfir_src_dir}/interpreter
    ${silverfir_src_dir}/jit
    ${silverfir_src_dir}/platform
    ${silverfir_src_dir}/runtime
    ${silverfir_src_dir}/utils
)

if (ENABLE_SPECTEST)
    list(APPEND silverfir_sources
        ${silverfir_src_dir}/host/spectest/spectest_gen.c
        ${silverfir_src_dir}/host/spectest/spectest_impl.c
    )
    list(APPEND silverfir_private_includes
        ${silverfir_src_dir}/host/spectest
    )
endif (ENABLE_SPECTEST)

if (ENABLE_WASI)
    list(APPEND silverfir_sources
        ${silverfir_src_dir}/host/wasi/wasi_gen.c
        ${silverfir_src_dir}/host/wasi/wasi_impl_uv.c
    )
    list(APPEND silverfir_private_includes
        ${silverfir_src_dir}/host/wasi
    )
endif (ENABLE_WASI)

if (CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    list(APPEND silverfir_private_includes
        ${silverfir_src_dir}/platform/msvc
    )
    list(APPEND silverfir_sources)
elseif (CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
# including clang for windows
# OR (CMAKE_C_SIMULATE_ID STREQUAL "MSVC"))
    list(APPEND silverfir_private_includes
        ${silverfir_src_dir}/platform/gcc_clang
    )
    list(APPEND silverfir_sources)
endif ()

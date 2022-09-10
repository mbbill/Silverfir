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

cmake_minimum_required(VERSION 3.15)

include(${PROJECT_SOURCE_DIR}/src/silverfir.cmake)

##############################################
# coverage
add_library(coverage coverage/summary.c)
target_include_directories(coverage PRIVATE ${silverfir_private_includes})

##############################################
# unittest
add_executable(unittest
    unit/hello_wasm.c
    unit/list_test.c
    unit/mem_test.c
    unit/option_test.c
    unit/parser_test.c
    unit/result_test.c
    unit/runtime_test.c
    unit/sjson_test.c
    unit/smath_test.c
    unit/stream_test.c
    unit/test_containers.c
    unit/unittest_main.c
    unit/vec_test.c
    unit/leb128_cases.c
)
# we have to duplicate the private include list so that the unit tests can call into the
# private functions.
target_include_directories(unittest PRIVATE ${silverfir_private_includes})
target_link_libraries(unittest PRIVATE build_flags silverfir coverage cmocka)

##############################################
# Spec test runner
add_executable(harness
    harness/harness_main.c
    harness/spec_test.c
)
target_include_directories(harness PRIVATE ${silverfir_private_includes})
target_link_libraries(harness PRIVATE build_flags silverfir)

##############################################
# Simple WASM test loader
add_executable(sf_loader
    sf_loader/loader_main.c
)
target_include_directories(sf_loader PRIVATE ${silverfir_private_includes})
target_link_libraries(sf_loader PRIVATE build_flags silverfir)

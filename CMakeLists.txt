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

project(Silverfir)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED True)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-checks=*")

if (CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    # Debug
    set(CMAKE_C_FLAGS_DEBUG "-g -O0")
    # Release
    set(CMAKE_C_FLAGS_RELEASE "-O3 -DNDEBUG")
    # RelWithDebInfo
    set(CMAKE_C_FLAGS_RELWITHDEBINFO "-O3 -g -DNDEBUG")
    # MinSizeRel
    set(CMAKE_C_FLAGS_MINSIZEREL "-Os -g -DNDEBUG")
endif()

option(ENABLE_SPECTEST "Enable spec test" ON)
option(ENABLE_WASI "Enable WASI" ON)
option(ENABLE_LOGGER "Enable logger" ON)

add_subdirectory(src)
add_subdirectory(test)
add_subdirectory(third_party/cmocka)
if (ENABLE_WASI)
    add_subdirectory(third_party/uvwasi)
endif(ENABLE_WASI)

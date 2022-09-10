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

add_library(cmocka src/cmocka.c)
target_include_directories(cmocka PUBLIC include)
target_compile_definitions(cmocka PUBLIC
    HAVE__SNPRINTF_S
    HAVE__VSNPRINTF_S
    HAVE_INTTYPES_H
)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(cmocka PRIVATE "/wd4996" "/wd4267" "/wd5105")
endif()

if ((CMAKE_CXX_COMPILER_ID STREQUAL "Clang") OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
    target_compile_options(cmocka PRIVATE -Wno-deprecated-declarations)
endif()
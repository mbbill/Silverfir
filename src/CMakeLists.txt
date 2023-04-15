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

include(Silverfir.cmake)

##############################################
# The interface library to set the flags
add_library(build_flags INTERFACE)

if(CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    target_compile_options(build_flags INTERFACE /W4 /WX /wd4100 /wd5105)
    target_compile_definitions(build_flags INTERFACE _CRT_SECURE_NO_WARNINGS=1)
endif()

if (CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang|GNU")
    target_compile_options(build_flags INTERFACE -Wall -Wextra -Wpedantic -Werror -Wno-unused-parameter -Wno-gnu-zero-variadic-macro-arguments -Wno-deprecated-declarations -Wno-gnu-label-as-value)

    # The debug build will not optimize the stack frame size, so each interpreter loop call will take roughly 5kb of space, so
    # the stack will overflow quickly. It's not an issue for the optimized build since most of the local variables can be combined.
    if (CMAKE_BUILD_TYPE MATCHES Debug)
        if (CMAKE_C_SIMULATE_ID STREQUAL "MSVC")
            target_link_options(build_flags INTERFACE -Wl,/stack:0x1000000)
        else()
            target_link_options(build_flags INTERFACE -Wl,-stack_size -Wl,0x1000000)
        endif()
    endif()

    # Enable stack sanitizer for RelWithDebInfo build
    if (CMAKE_BUILD_TYPE MATCHES Debug OR CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
        if (CMAKE_C_COMPILER_ID MATCHES "AppleClang|Clang")
            target_compile_options(build_flags INTERFACE -fsanitize=address -fno-omit-frame-pointer)
            target_link_options(build_flags INTERFACE -fsanitize=address)
        endif()
    endif()
endif()

# spectest
if (ENABLE_SPECTEST)
    list(APPEND spectest_defines SILVERFIR_ENABLE_SPECTEST)
    target_compile_definitions(build_flags INTERFACE ${spectest_defines})
endif(ENABLE_SPECTEST)

# uvwasi
if (ENABLE_WASI)
    list(APPEND wasi_defines SILVERFIR_ENABLE_WASI)
    target_compile_definitions(build_flags INTERFACE ${wasi_defines})
    target_link_libraries(build_flags INTERFACE uvwasi_a)
    target_include_directories(build_flags INTERFACE "${PROJECT_SOURCE_DIR}/third_party/uvwasi/include")
endif(ENABLE_WASI)

# logger
if (ENABLE_LOGGER)
    list(APPEND tracer_defines SILVERFIR_ENABLE_LOGGER)
    target_compile_definitions(build_flags INTERFACE ${tracer_defines})
endif(ENABLE_LOGGER)

##############################################
# the main library.
add_library(silverfir ${silverfir_sources})

target_link_libraries(silverfir PRIVATE build_flags)
target_include_directories(silverfir PRIVATE ${silverfir_private_includes})
target_include_directories(silverfir PUBLIC "${PROJECT_SOURCE_DIR}/include")

# TODO: unfinished yet.
option(BUILD_WITH_COVERAGE "Enable code coverage info" OFF)
if (BUILD_WITH_COVERAGE)
    if (CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
        message("Code coverage is enabled!")
        target_compile_options(silverfir PUBLIC "--coverage")
        target_link_libraries(silverfir PUBLIC "C:\\Program Files (x86)\\Microsoft Visual Studio\\2019\\Enterprise\\VC\\Tools\\Llvm\\x64\\lib\\clang\\12.0.0\\lib\\windows\\clang_rt.profile-x86_64.lib")
    else()
        message(WARNING, "BUILD_WITH_COVERAGE is not yet supported on ${CMAKE_CXX_COMPILER_ID}")
    endif()
endif(BUILD_WITH_COVERAGE)

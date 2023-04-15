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

// The maximum number of stack frames.
#if !defined(SILVERFIR_STACK_FRAME_LIMIT)
    #define SILVERFIR_STACK_FRAME_LIMIT (256)
#endif

#if !defined(SILVERFIR_STACK_SIZE_LIMIT)
    #define SILVERFIR_STACK_SIZE_LIMIT (100 * 1024)
#endif

// Maximum number of locals in one frame
#if !defined(SILVERFIR_LOCAL_COUNT_LIMIT)
    #define SILVERFIR_LOCAL_COUNT_LIMIT (1024)
#endif

// Enable the direct-threading or the traditional switch-case based in-place interpreter
// Disable this option to fallback to TCO interpreter.
#if !defined(SILVERFIR_INTERP_INPLACE_DT)
    #define SILVERFIR_INTERP_INPLACE_DT 1
#endif

// Enable the tail-call optimized in-place interpreter
#if !defined(SILVERFIR_INTERP_INPLACE_TCO)
    #define SILVERFIR_INTERP_INPLACE_TCO 1
#endif

#if !SILVERFIR_INTERP_INPLACE_DT && !SILVERFIR_INTERP_INPLACE_TCO
// TODO: in the future we may allow JIT only mode.
#error All interpreters are disabled.
#endif

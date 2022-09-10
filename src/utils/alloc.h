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

#include <stdlib.h>

#define array_realloc(type, p, new_size) realloc(p, new_size * sizeof(type))
#define array_free(p) free(p)
#define array_alloc(type, count) malloc(sizeof(type) * count)
#define array_calloc(type, count) calloc(count, sizeof(type))
#define array_alloca(type, count) alloca(sizeof(type) * count)

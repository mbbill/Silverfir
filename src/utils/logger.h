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

#ifdef SILVERFIR_ENABLE_LOGGER

#include <inttypes.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>

#define FOR_EACH_LOG_CHANNEL(macro) \
    macro(in_place_dt)              \
    macro(in_place_tco)             \
    macro(ir_builder)               \
    macro(parser)                   \
    macro(validator)                \
    macro(vm)                       \
    macro(test)

#define LOG_CHANNEL_ENUM_ITEM(name) log_channel_##name,
typedef enum log_channel {
    FOR_EACH_LOG_CHANNEL(LOG_CHANNEL_ENUM_ITEM)
        log_channel_count,
} log_channel;

typedef enum log_level {
    log_info,
    log_warning,
} log_level;

// DO NOT use this function directly. use LOG_INFO or LOG_WARNING instead.
void log_with_level(log_level level, log_channel channel, const char * fmt, ...);

void log_channel_set_enabled(log_level level, log_channel channel, bool enabled);

// LOG_INFO is disabled in the release build by default.
#if defined(NDEBUG) && !defined(SILVERFIR_ENABLE_LOG_INFO)
#define LOG_INFO(channel, fmt, ...)
#else
#define LOG_INFO_ENABLED
#define LOG_INFO(channel, fmt, ...) log_with_level(log_info, channel, fmt, __VA_ARGS__)
#endif

// Warning logs are always enabled.
#define LOG_WARNING(channel, fmt, ...) log_with_level(log_warning, channel, fmt, __VA_ARGS__)

// There is no LOG_ERROR because errors should be properly returned and handled.

#endif // SILVERFIR_ENABLE_LOGGER

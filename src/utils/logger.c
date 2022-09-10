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

#ifdef SILVERFIR_ENABLE_LOGGER

#include "logger.h"

#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#define LOG_CHANNEL_NAME(name) #name,
static const char * const channel_name[] = {FOR_EACH_LOG_CHANNEL(LOG_CHANNEL_NAME)};

static bool enabled_channels_info[log_channel_count] = {0};

// all channels of warning messages are enabled by default unless they're specifically suppressed
#define LOG_CHANNEL_DEFAULT_ENABLE(name) true,
static bool enabled_channels_warning[log_channel_count] = {
    FOR_EACH_LOG_CHANNEL(LOG_CHANNEL_DEFAULT_ENABLE)};

// TODO: move the stdio to the `platform` folder and then extract an interface
void log_with_level(log_level level, log_channel channel, const char * fmt, ...) {
    assert(channel < log_channel_count);
    assert(fmt);
    switch (level) {
        case log_info:
            if (!enabled_channels_info[channel]) {
                return;
            }
            printf("[%s] ", channel_name[channel]);
            break;
        case log_warning:
            if (!enabled_channels_warning[channel]) {
                return;
            }
            printf("[%s] WARNING: ", channel_name[channel]);
            break;
    }
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    printf("\n");
    va_end(args);
}

void log_channel_set_enabled(log_level level, log_channel channel, bool enabled) {
    assert(channel < log_channel_count);
    switch (level) {
        case log_info:
            enabled_channels_info[channel] = enabled;
            break;
        case log_warning:
            enabled_channels_warning[channel] = enabled;
            break;
    }
}

#endif // SILVERFIR_ENABLE_LOGGER

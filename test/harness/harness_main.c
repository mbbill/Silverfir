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

#include "alloc.h"
#include "host_modules.h"
#include "interpreter.h"
#include "logger.h"
#include "result.h"
#include "runtime.h"
#include "sjson.h"
#include "spec_test.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LOGI(fmt, ...) LOG_INFO(log_channel_test, fmt, __VA_ARGS__)
#define LOGW(fmt, ...) LOG_WARNING(log_channel_test, fmt, __VA_ARGS__)

int json_runner(const char * json_fname, const char * dir, bool verbose) {
    int ret = 0;
    assert(json_fname);

    log_channel_set_enabled(log_info, log_channel_test, true);
    log_channel_set_enabled(log_warning, log_channel_test, true);

    if (verbose) {
        log_channel_set_enabled(log_info, log_channel_validator, true);
        log_channel_set_enabled(log_info, log_channel_in_place_dt, true);
    }

    str module_dir;
    if (!dir) {
        LOGI("Spec test folder unspecified, using the working folder");
        module_dir = STR_NULL;
    } else {
        module_dir = s_p(dir);
    }

    FILE * fp = fopen(json_fname, "r");
    if (!fp) {
        LOGW("Cannot open %s", json_fname);
        ret = 1;
        return ret;
    }

    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    if (!fsize) {
        LOGW("File is empty!");
        ret = 1;
        fclose(fp);
        return ret;
    }
    rewind(fp);

    char * json_buf = malloc(fsize);
    if (!json_buf) {
        LOGW("Failed to allocate JSON file buffer");
        ret = 1;
        fclose(fp);
        return ret;
    }

    size_t read_size = fread(json_buf, 1, fsize, fp);
    if (read_size != fsize) {
        LOGW("Failed to read the JSON file");
        ret = 1;
        fclose(fp);
        free(json_buf);
        return ret;
    }

    // parse the spec test format based on
    // https://github.com/WebAssembly/wabt/blob/main/docs/wast2json.md
    r_json_value rj = json_parse(str_from((str_iter_t)json_buf, read_size));
    if (!is_ok(rj)) {
        LOGW("Failed to parse the JSON file");
        ret = 1;
        fclose(fp);
        free(json_buf);
        return ret;
    }

    json_value json = rj.value;
    r result = do_spec_test(json, module_dir, verbose);
    if (!is_ok(result)) {
        LOGW("Error msg: %s", result.msg);
        ret = 1;
    }

    fclose(fp);
    free(json_buf);
    json_free_value(&json);
    return ret;
}

int print_help(const char * exec_name) {
    assert(exec_name);
    const char * base_name = strrchr(exec_name, '/');
    if (!base_name) {
        base_name = strrchr(exec_name, '\\');
    }
    if (base_name) {
        base_name++;
    } else {
        base_name = exec_name;
    }
    printf("Usage:\n");
    printf(" %s -j <json> [-d folder] [-v]\n", base_name);
    printf(" %s -h\n\n", base_name);
    return 1;
}

int main(int argc, char * argv[]) {
#if defined(_MSC_VER) && !defined(NDEBUG)
    int flag = _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF; //_CRTDBG_CHECK_ALWAYS_DF | _CRTDBG_DELAY_FREE_MEM_DF; this is a bit too slow.
    flag = (flag & 0x0000FFFF) | _CRTDBG_CHECK_EVERY_16_DF;
    _CrtSetDbgFlag(flag);
    _CrtSetReportMode(_CRT_WARN, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_WARN, _CRTDBG_FILE_STDOUT);
    // _CrtSetReportMode(_CRT_ERROR, _CRTDBG_MODE_FILE);
    // _CrtSetReportFile(_CRT_ERROR, _CRTDBG_FILE_STDOUT);
    _CrtSetReportMode(_CRT_ASSERT, _CRTDBG_MODE_FILE);
    _CrtSetReportFile(_CRT_ASSERT, _CRTDBG_FILE_STDOUT);
#endif
    const char * folder_name = NULL;
    const char * json_name = NULL;
    bool verbose = false;

    if (argc == 1) {
        return print_help(argv[0]);
    }

    for (int i = 1; i < argc; i++) {
        if (argv[i][0] == '-' && strlen(argv[i]) >= 2) {
            switch (argv[i][1]) {
                case 'd':
                    if (i + 1 >= argc) {
                        return print_help(argv[0]);
                    }
                    folder_name = argv[++i];
                    break;
                case 'j':
                    if (i + 1 >= argc) {
                        return print_help(argv[0]);
                    }
                    json_name = argv[++i];
                    break;
                case 'v':
                    verbose = true;
                    break;
                case 'h':
                default:
                    return print_help(argv[0]);
            }
        } else {
            return print_help(argv[0]);
        }
    }

    if (!json_name) {
        return print_help(argv[0]);
    }

    return json_runner(json_name, folder_name, verbose);
}

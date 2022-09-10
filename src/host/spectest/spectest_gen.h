// Generated from spectest.wat do not modify!

#include "host_modules.h"
#include "types.h"
#include "vm.h"
#include <stddef.h>

extern const u8 spectest_wasm[];

extern const size_t spectest_wasm_len;

extern const host_func_info spectest_host_info[];

extern const size_t spectest_host_info_len;

// The following functions needs to be manually implemented
extern r spectest_print (tr_ctx);
extern r spectest_print_i32 (tr_ctx, i32);
extern r spectest_print_i64 (tr_ctx, i64);
extern r spectest_print_f32 (tr_ctx, f32);
extern r spectest_print_f64 (tr_ctx, f64);
extern r spectest_print_i32_f32 (tr_ctx, i32, f32);
extern r spectest_print_f64_f64 (tr_ctx, f64, f64);
// The above functions needs to be manually implemented

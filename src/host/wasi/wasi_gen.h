// Generated from wasi.wat do not modify!

#include "host_modules.h"
#include "types.h"
#include "vm.h"
#include <stddef.h>

extern const u8 wasi_wasm[];

extern const size_t wasi_wasm_len;

extern const host_func_info wasi_host_info[];

extern const size_t wasi_host_info_len;

// The following functions needs to be manually implemented
extern r wasi_args_get (tr_ctx, i32, i32, i32*);
extern r wasi_args_sizes_get (tr_ctx, i32, i32, i32*);
extern r wasi_clock_res_get (tr_ctx, i32, i32, i32*);
extern r wasi_clock_time_get (tr_ctx, i32, i64, i32, i32*);
extern r wasi_environ_get (tr_ctx, i32, i32, i32*);
extern r wasi_environ_sizes_get (tr_ctx, i32, i32, i32*);
extern r wasi_fd_advise (tr_ctx, i32, i64, i64, i32, i32*);
extern r wasi_fd_allocate (tr_ctx, i32, i64, i64, i32*);
extern r wasi_fd_close (tr_ctx, i32, i32*);
extern r wasi_fd_datasync (tr_ctx, i32, i32*);
extern r wasi_fd_fdstat_get (tr_ctx, i32, i32, i32*);
extern r wasi_fd_fdstat_set_flags (tr_ctx, i32, i32, i32*);
extern r wasi_fd_fdstat_set_rights (tr_ctx, i32, i64, i64, i32*);
extern r wasi_fd_filestat_get (tr_ctx, i32, i32, i32*);
extern r wasi_fd_filestat_set_size (tr_ctx, i32, i64, i32*);
extern r wasi_fd_filestat_set_times (tr_ctx, i32, i64, i64, i32, i32*);
extern r wasi_fd_pread (tr_ctx, i32, i32, i32, i64, i32, i32*);
extern r wasi_fd_prestat_dir_name (tr_ctx, i32, i32, i32, i32*);
extern r wasi_fd_prestat_get (tr_ctx, i32, i32, i32*);
extern r wasi_fd_pwrite (tr_ctx, i32, i32, i32, i64, i32, i32*);
extern r wasi_fd_read (tr_ctx, i32, i32, i32, i32, i32*);
extern r wasi_fd_readdir (tr_ctx, i32, i32, i32, i64, i32, i32*);
extern r wasi_fd_renumber (tr_ctx, i32, i32, i32*);
extern r wasi_fd_seek (tr_ctx, i32, i64, i32, i32, i32*);
extern r wasi_fd_sync (tr_ctx, i32, i32*);
extern r wasi_fd_tell (tr_ctx, i32, i32, i32*);
extern r wasi_fd_write (tr_ctx, i32, i32, i32, i32, i32*);
extern r wasi_path_create_directory (tr_ctx, i32, i32, i32, i32*);
extern r wasi_path_filestat_get (tr_ctx, i32, i32, i32, i32, i32, i32*);
extern r wasi_path_filestat_set_times (tr_ctx, i32, i32, i32, i32, i64, i64, i32, i32*);
extern r wasi_path_link (tr_ctx, i32, i32, i32, i32, i32, i32, i32, i32*);
extern r wasi_path_open (tr_ctx, i32, i32, i32, i32, i32, i64, i64, i32, i32, i32*);
extern r wasi_path_readlink (tr_ctx, i32, i32, i32, i32, i32, i32, i32*);
extern r wasi_path_remove_directory (tr_ctx, i32, i32, i32, i32*);
extern r wasi_path_rename (tr_ctx, i32, i32, i32, i32, i32, i32, i32*);
extern r wasi_path_symlink (tr_ctx, i32, i32, i32, i32, i32, i32*);
extern r wasi_path_unlink_file (tr_ctx, i32, i32, i32, i32*);
extern r wasi_poll_oneoff (tr_ctx, i32, i32, i32, i32, i32*);
extern r wasi_proc_exit (tr_ctx, i32);
extern r wasi_proc_raise (tr_ctx, i32, i32*);
extern r wasi_random_get (tr_ctx, i32, i32, i32*);
extern r wasi_sched_yield (tr_ctx, i32*);
extern r wasi_sock_accept (tr_ctx, i32, i32, i32, i32*);
extern r wasi_sock_recv (tr_ctx, i32, i32, i32, i32, i32, i32, i32*);
extern r wasi_sock_send (tr_ctx, i32, i32, i32, i32, i32, i32*);
extern r wasi_sock_shutdown (tr_ctx, i32, i32, i32*);
// The above functions needs to be manually implemented

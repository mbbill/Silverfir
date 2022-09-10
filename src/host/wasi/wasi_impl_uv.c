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

#include "wasi_impl_uv.h"

#include "alloc.h"
#include "compiler.h"
#include "host_modules.h"
#include "types.h"
#include "uvwasi.h"
#include "vec.h"
#include "wasi_gen.h"

#include <stdio.h>

#define WASI_ERRNO_OK 0

static int g_argc;
static const char ** g_argv;

#define to_wasi(ctx) (&((wasi_ctx *)(ctx.f_addr->mod->resource_payload))->uvwasi)

#define check_err(e, ...)       \
    if (e != UVWASI_ESUCCESS) { \
        *result = err;          \
        __VA_ARGS__;            \
        return ok_r;            \
    }

// pointer to the beginning of mem0
#define pmem0 ((char *)(ctx.mem0->mdata._data))
#define pmem0_size (ctx.mem0->mdata._size)

void wasi_set_args(int argc, char ** argv) {
    g_argc = argc;
    g_argv = (const char **)argv;
}

static uvwasi_preopen_t g_preopens = {
    .mapped_path = ".",
    .real_path = ".",
};

wasi_ctx * wasi_ctx_get() {
    wasi_ctx * ctx = array_calloc(wasi_ctx, 1);
    uvwasi_options_t init_options;
    uvwasi_options_init(&init_options);
    init_options.argc = g_argc;
    init_options.argv = g_argv;
    init_options.preopenc = 1;
    init_options.preopens = &g_preopens;
    uvwasi_errno_t err = uvwasi_init(&ctx->uvwasi, &init_options);
    if (err != UVWASI_ESUCCESS) {
        return NULL;
    }
    return ctx;
}

void wasi_ctx_drop(void * payload) {
    assert(payload);
    wasi_ctx * ctx = (wasi_ctx *)payload;
    // array_free(ctx->uvwasi.)
    uvwasi_destroy(&ctx->uvwasi);
    array_free(ctx);
}

r wasi_args_get(tr_ctx ctx, i32 argv, i32 argv_buf, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t argc;
    uvwasi_size_t buf_size;
    err = uvwasi_args_sizes_get(to_wasi(ctx), &argc, &buf_size);
    check_err(err);
    if (!argc) {
        *result = WASI_ERRNO_OK;
        return ok_r;
    }
    vec_u8p v_argv = {0};
    check(vec_resize_u8p(&v_argv, argc));
    err = uvwasi_args_get(to_wasi(ctx), (char **)v_argv._data, pmem0 + argv_buf);
    check_err(err, vec_clear_u8p(&v_argv));
    for (size_t i = 0; i < argc; i++) {
        uvwasi_serdes_write_uint32_t(pmem0, argv + i * 4, (uint32_t)((char *)*vec_at_u8p(&v_argv, i) - pmem0));
    }
    vec_clear_u8p(&v_argv);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_args_sizes_get(tr_ctx ctx, i32 argc, i32 bufsize, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t _argc;
    uvwasi_size_t _bufsize;
    err = uvwasi_args_sizes_get(to_wasi(ctx), &_argc, &_bufsize);
    check_err(err);
    uvwasi_serdes_write_size_t(pmem0, argc, _argc);
    uvwasi_serdes_write_size_t(pmem0, bufsize, _bufsize);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_clock_res_get(tr_ctx ctx, i32 clock_id, i32 resolution, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_timestamp_t _resolution;
    err = uvwasi_clock_res_get(to_wasi(ctx), (u32)clock_id, &_resolution);
    check_err(err);
    uvwasi_serdes_write_timestamp_t(pmem0, resolution, _resolution);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_clock_time_get(tr_ctx ctx, i32 clock_id, i64 precision, i32 time, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_timestamp_t _time;
    err = uvwasi_clock_time_get(to_wasi(ctx), (u32)clock_id, (u64)precision, &_time);
    check_err(err);
    uvwasi_serdes_write_timestamp_t(pmem0, time, _time);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_environ_get(tr_ctx ctx, i32 environ_, i32 environ_buf, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t environ_count;
    uvwasi_size_t environ_buf_size;
    err = uvwasi_environ_sizes_get(to_wasi(ctx), &environ_count, &environ_buf_size);
    check_err(err);
    if (!environ_count) {
        *result = WASI_ERRNO_OK;
        return ok_r;
    }
    vec_u8p v_env = {0};
    check(vec_resize_u8p(&v_env, environ_count));
    err = uvwasi_environ_get(to_wasi(ctx), (char **)v_env._data, pmem0 + environ_buf);
    check_err(err, vec_clear_u8p(&v_env));
    for (size_t i = 0; i < environ_count; i++) {
        uvwasi_serdes_write_uint32_t(pmem0, environ_ + i * 4, (uint32_t)((char *)*vec_at_u8p(&v_env, i) - pmem0));
    }
    vec_clear_u8p(&v_env);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_environ_sizes_get(tr_ctx ctx, i32 environ_count, i32 environ_buf_size, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t _environ_count;
    uvwasi_size_t _environ_buf_size;
    err = uvwasi_environ_sizes_get(to_wasi(ctx), &_environ_count, &_environ_buf_size);
    check_err(err);
    uvwasi_serdes_write_size_t(pmem0, environ_count, _environ_count);
    uvwasi_serdes_write_size_t(pmem0, environ_buf_size, _environ_buf_size);
    return ok_r;
}

r wasi_fd_advise(tr_ctx ctx, i32 fd, i64 offset, i64 len, i32 advice, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_advise(to_wasi(ctx), fd, offset, len, (uvwasi_advice_t)advice);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_allocate(tr_ctx ctx, i32 fd, i64 offset, i64 len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_allocate(to_wasi(ctx), fd, offset, len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_close(tr_ctx ctx, i32 fd, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_close(to_wasi(ctx), fd);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_datasync(tr_ctx ctx, i32 fd, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_datasync(to_wasi(ctx), fd);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_fdstat_get(tr_ctx ctx, i32 fd, i32 fdstat, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_fdstat_t stat;
    err = uvwasi_fd_fdstat_get(to_wasi(ctx), fd, &stat);
    check_err(err);
    uvwasi_serdes_write_fdstat_t(pmem0, fdstat, &stat);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_fdstat_set_flags(tr_ctx ctx, i32 fd, i32 flags, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_fdstat_set_flags(to_wasi(ctx), fd, (uvwasi_fdflags_t)flags);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_fdstat_set_rights(tr_ctx ctx, i32 fd, i64 r_base, i64 r_inheriting, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_fdstat_set_rights(to_wasi(ctx), fd, (uvwasi_rights_t)r_base, (uvwasi_rights_t)r_inheriting);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_filestat_get(tr_ctx ctx, i32 fd, i32 filestat, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_filestat_t stat;
    err = uvwasi_fd_filestat_get(to_wasi(ctx), fd, &stat);
    check_err(err);
    uvwasi_serdes_write_filestat_t(pmem0, filestat, &stat);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_filestat_set_size(tr_ctx ctx, i32 fd, i64 size, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_filestat_set_size(to_wasi(ctx), fd, (uvwasi_filesize_t)size);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_filestat_set_times(tr_ctx ctx, i32 fd, i64 atim, i64 mtim, i32 flags, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_filestat_set_times(to_wasi(ctx), fd, (uvwasi_timestamp_t)atim, (uvwasi_timestamp_t)mtim, (uvwasi_fstflags_t)flags);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_pread(tr_ctx ctx, i32 fd, i32 iovs, i32 iovs_len, i64 offset, i32 nread, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_iovec_t * p_iovec = array_alloc(uvwasi_iovec_t, iovs_len);
    if (!p_iovec) {
        return err(e_general, "OOM");
    }
    uvwasi_size_t _nread;
    err = uvwasi_serdes_readv_iovec_t(pmem0, pmem0_size, iovs, p_iovec, iovs_len);
    check_err(err, free(p_iovec));
    err = uvwasi_fd_pread(to_wasi(ctx), fd, p_iovec, iovs_len, offset, &_nread);
    check_err(err, free(p_iovec));
    uvwasi_serdes_write_size_t(pmem0, nread, _nread);
    free(p_iovec);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_prestat_get(tr_ctx ctx, i32 fd, i32 prestat, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_prestat_t stat;
    err = uvwasi_fd_prestat_get(to_wasi(ctx), fd, &stat);
    check_err(err);
    uvwasi_serdes_write_prestat_t(pmem0, prestat, &stat);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_prestat_dir_name(tr_ctx ctx, i32 fd, i32 path, i32 path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_prestat_dir_name(to_wasi(ctx), fd, pmem0 + path, path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_pwrite(tr_ctx ctx, i32 fd, i32 iovs, i32 iovs_len, i64 offset, i32 nwritten, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_ciovec_t * p_iovec = array_alloc(uvwasi_ciovec_t, iovs_len);
    if (!p_iovec) {
        return err(e_general, "OOM");
    }
    uvwasi_size_t _nwritten;
    err = uvwasi_serdes_readv_ciovec_t(pmem0, pmem0_size, iovs, p_iovec, iovs_len);
    check_err(err, free(p_iovec));
    err = uvwasi_fd_pwrite(to_wasi(ctx), fd, p_iovec, iovs_len, offset, &_nwritten);
    check_err(err, free(p_iovec));
    uvwasi_serdes_write_size_t(pmem0, nwritten, _nwritten);
    free(p_iovec);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_read(tr_ctx ctx, i32 fd, i32 iovs, i32 iovs_len, i32 nread, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_iovec_t * p_iovec = array_alloc(uvwasi_iovec_t, iovs_len);
    if (!p_iovec) {
        return err(e_general, "OOM");
    }
    uvwasi_size_t _nread;
    err = uvwasi_serdes_readv_iovec_t(pmem0, pmem0_size, iovs, p_iovec, iovs_len);
    check_err(err, free(p_iovec));
    err = uvwasi_fd_read(to_wasi(ctx), fd, p_iovec, iovs_len, &_nread);
    check_err(err, free(p_iovec));
    uvwasi_serdes_write_size_t(pmem0, nread, _nread);
    free(p_iovec);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_readdir(tr_ctx ctx, i32 fd, i32 buf, i32 buf_len, i64 cookie, i32 bufused, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t _bufused;
    err = uvwasi_fd_readdir(to_wasi(ctx), fd, pmem0 + buf, buf_len, cookie, &_bufused);
    check_err(err);
    uvwasi_serdes_write_size_t(pmem0, bufused, _bufused);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_renumber(tr_ctx ctx, i32 from, i32 to, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_fd_renumber(to_wasi(ctx), from, to);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_seek(tr_ctx ctx, i32 fd, i64 offset, i32 whence, i32 newoffset, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_filesize_t _newoffset;
    err = uvwasi_fd_seek(to_wasi(ctx), fd, offset, (uvwasi_whence_t)whence, &_newoffset);
    check_err(err);
    uvwasi_serdes_write_filesize_t(pmem0, newoffset, _newoffset);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_sync(tr_ctx ctx, i32 fd, i32 * result) {
    uvwasi_errno_t err = uvwasi_fd_sync(to_wasi(ctx), fd);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_tell(tr_ctx ctx, i32 fd, i32 offset, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_filesize_t _offset;
    err = uvwasi_fd_tell(to_wasi(ctx), fd, &_offset);
    check_err(err);
    uvwasi_serdes_write_filesize_t(pmem0, offset, _offset);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_fd_write(tr_ctx ctx, i32 fd, i32 iovs, i32 iovs_len, i32 nwritten, i32 * result) {
    check_prep(r);
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_ciovec_t * p_iovec = array_alloc(uvwasi_ciovec_t, iovs_len);
    if (!p_iovec) {
        return err(e_general, "OOM");
    }
    uvwasi_size_t _nwritten;
    err = uvwasi_serdes_readv_ciovec_t(pmem0, pmem0_size, iovs, p_iovec, iovs_len);
    check_err(err, free(p_iovec));
    err = uvwasi_fd_write(to_wasi(ctx), fd, p_iovec, iovs_len, &_nwritten);
    check_err(err, free(p_iovec));
    uvwasi_serdes_write_size_t(pmem0, nwritten, _nwritten);
    free(p_iovec);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_create_directory(tr_ctx ctx, i32 fd, i32 path, i32 path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_create_directory(to_wasi(ctx), fd, pmem0 + path, path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_filestat_get(tr_ctx ctx, i32 fd, i32 flags, i32 path, i32 path_len, i32 buf, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_filestat_t filestat;
    err = uvwasi_path_filestat_get(to_wasi(ctx), fd, flags, pmem0 + path, path_len, &filestat);
    check_err(err);
    uvwasi_serdes_write_filestat_t(pmem0, buf, &filestat);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_filestat_set_times(tr_ctx ctx, i32 fd, i32 flags, i32 path, i32 path_len, i64 st_atim, i64 st_mtim, i32 fst_flags, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_filestat_set_times(to_wasi(ctx), fd, flags, pmem0 + path, path_len, st_atim, st_mtim, (uvwasi_fstflags_t)fst_flags);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_link(tr_ctx ctx, i32 old_fd, i32 old_flags, i32 old_path, i32 old_path_len, i32 new_fd, i32 new_path, i32 new_path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_link(to_wasi(ctx), old_fd, old_flags, pmem0 + old_path, old_path_len, new_fd, pmem0 + new_path, new_path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_open(tr_ctx ctx, i32 dirfd, i32 dirflags, i32 path, i32 path_len, i32 o_flags, i64 fs_rights_base, i64 fs_rights_inheriting, i32 fs_flags, i32 fd, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_fd_t out_fd;
    uvwasi_path_open(to_wasi(ctx), dirfd, dirflags, pmem0 + path, path_len, (uvwasi_oflags_t)o_flags, fs_rights_base, fs_rights_inheriting, (uvwasi_fdflags_t)fs_flags, &out_fd);
    check_err(err);
    uvwasi_serdes_write_fd_t(pmem0, fd, out_fd);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_readlink(tr_ctx ctx, i32 fd, i32 path, i32 path_len, i32 buf_len, i32 buf, i32 buf_used, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t _buf_used;
    err = uvwasi_path_readlink(to_wasi(ctx), fd, pmem0 + path, path_len, pmem0 + buf, buf_len, &_buf_used);
    check_err(err);
    uvwasi_serdes_write_size_t(pmem0, buf_used, _buf_used);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_remove_directory(tr_ctx ctx, i32 fd, i32 path, i32 path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_remove_directory(to_wasi(ctx), fd, pmem0 + path, path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_rename(tr_ctx ctx, i32 old_fd, i32 old_path, i32 old_path_len, i32 new_fd, i32 new_path, i32 new_path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_rename(to_wasi(ctx), old_fd, pmem0 + old_path, old_path_len, new_fd, pmem0 + new_path, new_path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_symlink(tr_ctx ctx, i32 old_path, i32 old_path_len, i32 fd, i32 new_path, i32 new_path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_symlink(to_wasi(ctx), pmem0 + old_path, old_path_len, fd, pmem0 + new_path, new_path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_path_unlink_file(tr_ctx ctx, i32 fd, i32 path, i32 path_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_path_unlink_file(to_wasi(ctx), fd, pmem0 + path, path_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_poll_oneoff(tr_ctx ctx, i32 in, i32 out, i32 nsubscriptions, i32 nevents, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    uvwasi_size_t _nevents;
    uvwasi_subscription_t _in;
    uvwasi_event_t _out;
    uvwasi_serdes_read_subscription_t(pmem0, in, &_in);
    uvwasi_serdes_read_event_t(pmem0, out, &_out);
    err = uvwasi_poll_oneoff(to_wasi(ctx), &_in, &_out, nsubscriptions, &_nevents);
    check_err(err);
    uvwasi_serdes_write_size_t(pmem0, nevents, _nevents);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_proc_exit(tr_ctx ctx, i32 exit_code) {
    check_prep(r);
    // TODO: pass the exit code
    //uvwasi_proc_exit(to_wasi(ctx), (uvwasi_exitcode_t)(u32)exit_code);
    return err(e_exit, "Application called exit()");
}

r wasi_proc_raise(tr_ctx ctx, i32 sig, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_proc_raise(to_wasi(ctx), (uvwasi_signal_t)sig);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_random_get(tr_ctx ctx, i32 buf, i32 buf_len, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_random_get(to_wasi(ctx), pmem0 + buf, buf_len);
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_sched_yield(tr_ctx ctx, i32 * result) {
    uvwasi_errno_t err = UVWASI_ESUCCESS;
    err = uvwasi_sched_yield(to_wasi(ctx));
    check_err(err);
    *result = WASI_ERRNO_OK;
    return ok_r;
}

r wasi_sock_accept(tr_ctx ctx, i32 a, i32 b, i32 c, i32 * result) {
    *result = UVWASI_EINVAL;
    return ok_r;
}

r wasi_sock_recv(tr_ctx ctx, i32 sock, i32 ri_data, i32 ri_data_len, i32 ri_flags, i32 ro_datalen, i32 ro_flags, i32 * result) {
    *result = UVWASI_EINVAL;
    return ok_r;
}

r wasi_sock_send(tr_ctx ctx, i32 sock, i32 si_data, i32 si_data_len, i32 si_flags, i32 so_datalen, i32 * result) {
    *result = UVWASI_EINVAL;
    return ok_r;
}

r wasi_sock_shutdown(tr_ctx ctx, i32 sock, i32 how, i32 * result) {
    *result = UVWASI_EINVAL;
    return ok_r;
}

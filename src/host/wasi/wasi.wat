;;
;; Copyright 2022 Bai Ming
;;
;; Licensed under the Apache License, Version 2.0 (the "License");
;; you may not use this file except in compliance with the License.
;; You may obtain a copy of the License at
;;
;;     http://www.apache.org/licenses/LICENSE-2.0
;;
;; Unless required by applicable law or agreed to in writing, software
;; distributed under the License is distributed on an "AS IS" BASIS,
;; WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
;; See the License for the specific language governing permissions and
;; limitations under the License.

;; "wasi_snapshot_preview1" interface module
;; The python script does not recognize s-expr but use a regexp to match each line, so
;; please keep every function definition in just one line

(module
    (func (export "args_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "args_sizes_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "clock_res_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "clock_time_get") (param i32 i64 i32) (result i32) i32.const 1)
    (func (export "environ_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "environ_sizes_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_advise") (param i32 i64 i64 i32) (result i32) i32.const 1)
    (func (export "fd_allocate") (param i32 i64 i64) (result i32) i32.const 1)
    (func (export "fd_close") (param i32) (result i32) i32.const 1)
    (func (export "fd_datasync") (param i32) (result i32) i32.const 1)
    (func (export "fd_fdstat_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_fdstat_set_flags") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_fdstat_set_rights") (param i32 i64 i64) (result i32) i32.const 1)
    (func (export "fd_filestat_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_filestat_set_size") (param i32 i64) (result i32) i32.const 1)
    (func (export "fd_filestat_set_times") (param i32 i64 i64 i32) (result i32) i32.const 1)
    (func (export "fd_pread") (param i32 i32 i32 i64 i32) (result i32) i32.const 1)
    (func (export "fd_prestat_dir_name") (param i32 i32 i32) (result i32) i32.const 1)
    (func (export "fd_prestat_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_pwrite") (param i32 i32 i32 i64 i32) (result i32) i32.const 1)
    (func (export "fd_read") (param i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "fd_readdir") (param i32 i32 i32 i64 i32) (result i32) i32.const 1)
    (func (export "fd_renumber") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_seek") (param i32 i64 i32 i32) (result i32) i32.const 1)
    (func (export "fd_sync") (param i32) (result i32) i32.const 1)
    (func (export "fd_tell") (param i32 i32) (result i32) i32.const 1)
    (func (export "fd_write") (param i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_create_directory") (param i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_filestat_get") (param i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_filestat_set_times") (param i32 i32 i32 i32 i64 i64 i32) (result i32) i32.const 1)
    (func (export "path_link") (param i32 i32 i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_open") (param i32 i32 i32 i32 i32 i64 i64 i32 i32) (result i32) i32.const 1)
    (func (export "path_readlink") (param i32 i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_remove_directory") (param i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_rename") (param i32 i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_symlink") (param i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "path_unlink_file") (param i32 i32 i32) (result i32) i32.const 1)
    (func (export "poll_oneoff") (param i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "proc_exit") (param i32))
    (func (export "proc_raise") (param i32) (result i32) i32.const 1)
    (func (export "random_get") (param i32 i32) (result i32) i32.const 1)
    (func (export "sched_yield") (result i32) i32.const 1)
    (func (export "sock_accept") (param i32 i32 i32 ) (result i32) i32.const 1)
    (func (export "sock_recv") (param i32 i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "sock_send") (param i32 i32 i32 i32 i32) (result i32) i32.const 1)
    (func (export "sock_shutdown") (param i32 i32) (result i32) i32.const 1)
)

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

;; "spectest" interface module
;; The python script does not recognize s-expr but use a regexp to match each line, so
;; please keep every function definition in just one line

(module
    (func (export "print"))
    (func (export "print_i32") (param i32))
    (func (export "print_i64") (param i64))
    (func (export "print_f32") (param f32))
    (func (export "print_f64") (param f64))
    (func (export "print_i32_f32") (param i32 f32))
    (func (export "print_f64_f64") (param f64 f64))
    (table (export "table") 10 20 funcref)
    (memory (export "memory") 1 2)
    (global (export "global_i32") i32 (i32.const 666))
    (global (export "global_i64") i64 (i64.const 666))
    (global (export "global_f32") f32 (f32.const 666))
    (global (export "global_f64") f64 (f64.const 666))
)

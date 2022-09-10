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

#include "runtime.h"

#include "parser.h"
#include "vm.h"

#include <assert.h>

r runtime_drop(runtime * rt) {
    check_prep(r);
    assert(rt);

    LIST_FOR_EACH(&(rt->vms), vm, iter) {
        vm_drop(iter);
    }

    LIST_FOR_EACH(&rt->modules, module, iter) {
        assert(!iter->ref_count);
        check(module_drop(iter));
    }

    list_clear_vm(&rt->vms);
    list_clear_module(&rt->modules);
    return ok_r;
}

r runtime_module_add_mod(runtime * rt, module mod) {
    assert(rt);
    assert(!mod.ref_count);
    check_prep(r);

    assert(vec_size_vstr(&mod.names));
    assert(!vstr_is_null(mod.wasm_bin));

    // check if the name has conflicts.
    LIST_FOR_EACH(&rt->modules, module, m) {
        VEC_FOR_EACH(&m->names, vstr, n1) {
            VEC_FOR_EACH(&mod.names, vstr, n2) {
                if (str_eq(n1->s, n2->s)) {
                    return err(e_general, "Module name conflict");
                }
            }
        }
    }

    // at this time, the ref_count is still zero, until it gets instantiated.
    return list_push_module(&rt->modules, mod);
}

r runtime_module_drop(runtime * rt, module * mod) {
    assert(rt);
    assert(mod);
    check_prep(r);

    LIST_FOR_EACH(&rt->modules, module, m) {
        if (m == mod) {
            check(module_drop(m));
            list_erase_module(&rt->modules, m);
            return ok_r;
        }
    }
    return err(e_general, "The module isn't owned by the runtime");
}

r runtime_module_add(runtime * rt, vstr bin, vstr name) {
    check_prep(r);

    module m = {0};
    check(module_init(&m, bin, name), {
        r ret = module_drop(&m);
        UNUSED(ret);
        assert(is_ok(ret));
    });
    check(runtime_module_add_mod(rt, m), {
        r ret = module_drop(&m);
        UNUSED(ret);
        assert(is_ok(ret));
    });
    return ok_r;
}

module * runtime_module_find(runtime * rt, str name) {
    assert(rt);

    LIST_FOR_EACH(&rt->modules, module, mod) {
        VEC_FOR_EACH(&mod->names, vstr, n) {
            if (str_eq(n->s, name)) {
                return mod;
            }
        }
    }
    return NULL;
}

r_vm_ptr runtime_vm_new(runtime * rt) {
    assert(rt);
    check_prep(r_vm_ptr);

    vm v = {.rt = rt};
    check(list_push_vm(&rt->vms, v));

    vm * pv = list_back_vm(&rt->vms);
    assert(pv);
    return ok(pv);
}

r runtime_vm_drop(runtime * rt, vm * v) {
    assert(rt);
    assert(v);
    check_prep(r);

    LIST_FOR_EACH(&rt->vms, vm, iter) {
        if (iter == v) {
            vm_drop(iter);
            list_erase_vm(&rt->vms, iter);
            return ok_r;
        }
    }
    return err(e_general, "The vm isn't owned by the runtime");
}

#if 0
r_vm_ptr runtime_vm_new(runtime * rt, module * mod) {
    check_prep(r_vm_ptr);
    assert(rt);
    assert(mod);

    // make sure the module is registered in the runtime.
    bool found = false;
    LIST_FOR_EACH(&rt->modules, module, iter) {
        if (iter == mod) {
            found = true;
            break;
        }
    }
    if (!found) {
        return err(E_RUNTIME_MODULE_NOT_FOUND);
    }
    vm v = {0};
    list_voidptr deps = {0};
    // push the root module, then recursively instantiate all dependencies.
    list_push_voidptr(&deps, mod);

    while (list_size_voidptr(&deps)) {
        module * m = (module *)(*list_back_voidptr(&deps));
        assert(m);
        list_pop_voidptr(&deps);

        // first, let's find out if it is already instantiated.
        // Notice that by doing this we can allow circular dependencies.
        if (vm_find_mod_inst(&v, m->name)) {
            continue;
        }

        // not found, so let's create a new one.
        check(vm_instantiate_module(&v, m), {
            vm_drop(&v);
            list_clear_voidptr(&deps);
        });

        // find all deps and insert into the deps list.
        VEC_FOR_EACH(&m->imports, import, iter) {
            // TODO: let's handle native modules later
            if (str_eq(iter->module, s("wasi_snapshot_preview1"))) {
                continue;
            }
            module * dep = runtime_module_find(rt, iter->module);
            if (!dep) {
                vm_drop(&v);
                list_clear_voidptr(&deps);
                return err(E_RUNTIME_MODULE_NOT_FOUND);
            }
            list_push_voidptr(&deps, dep);
        }
    }
    list_clear_voidptr(&deps);

    check(list_push_vm(&rt->vms, v), {
        vm_drop(&v);
    });

    vm * pv = list_back_vm(&rt->vms);
    assert(pv);

    // link all instances inside the vm.
    // The instances will be pinned after linking, so we have to link the
    // instances in-place (after inserting into the list)
    check(vm_link(pv), {
        vm_drop(pv);
        list_pop_vm(&rt->vms);
    });

    return ok(pv);
}
#endif

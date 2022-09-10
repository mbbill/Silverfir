#!/usr/bin/env python3
# Copyright 2022 Bai Ming

import argparse
import os
import re
import shutil
import subprocess
import sys

def normalize(text):
    for ch in ['\\','`','*','{','}','[',']','(',')','>','#','+','-','.','!','$','\'']:
        text = text.replace(ch,"_")
    return text

parser = argparse.ArgumentParser()
parser.add_argument("file", help="Interface WAT file", type=str)
parser.add_argument("out_dir", help="Output folder", type=str)
args = parser.parse_args()

wasm_file = args.file

def to_hex(data):
    counter = 0
    hexdata = []
    for b in data:
        hexdata.append("0x{0:02x},".format(b))
        counter = counter + 1
        if counter == 16:
            hexdata.append("\n");
            counter = 0
    hexdata[-1] = hexdata[-1].rstrip(',')
    return hexdata

# I know I know this code is ugly as hell but whatever

def write_wasm_binary_info(fout_h, fout_c, wasm_file):
    wasm_data = wasm_file.read() # it's fine to read everything into memory since we're not like converting huge binary file into headers.
    var_name = normalize(os.path.basename(wasm_file.name))
    module_name = os.path.splitext(os.path.basename(wasm_file.name))[0]
    fout_c.write("// Generated from " + module_name + ".wat do not modify!\n")
    fout_c.write("\n#include \"types.h\"")
    fout_c.write("\n#include \"trampoline.h\"")
    fout_c.write("\n#include \"module.h\"")
    fout_c.write("\n#include \"vm.h\"")
    fout_c.write("\n#include <stddef.h>")
    fout_c.write("\n#include \"{}_gen.h\"".format(module_name))
    fout_c.write("\n")
    fout_c.write("\nconst size_t {}_len = {};\n".format(var_name, len(wasm_data)))
    fout_c.write("\nconst u8 {}[] = {{\n".format(var_name))
    fout_c.write("".join(to_hex(wasm_data)))
    fout_c.write("\n};\n\n")
    fout_h.write("// Generated from " + module_name + ".wat do not modify!\n")
    fout_h.write("\n#include \"host_modules.h\"")
    fout_h.write("\n#include \"types.h\"")
    fout_h.write("\n#include \"vm.h\"")
    fout_h.write("\n#include <stddef.h>")
    fout_h.write("\n")
    fout_h.write("\nextern const u8 {}[];\n".format(var_name))
    fout_h.write("\nextern const size_t {}_len;\n".format(var_name))
    fout_h.write("\nextern const host_func_info {}_host_info[];\n".format(module_name))
    fout_h.write("\nextern const size_t {}_host_info_len;\n".format(module_name))
    return 0

def process_wat(wat_file, out_dir):
    if shutil.which("wat2wasm") is None:
        sys.stderr.write("wat2wasm not found. Please make sure it's in the PATH")
        return None
    wasm_file = os.path.splitext(os.path.basename(wat_file))[0] +".wasm"
    cmd = ["wat2wasm", wat_file, "-o", os.path.join(out_dir, wasm_file)]
    return_code = subprocess.call(cmd)
    if return_code != 0:
        return None
    return os.path.join(out_dir, wasm_file)

# It's not necessary to fully parse the s-expr. A simple regexp should just work but you have to keep everything in
# one line
def get_function_signatures(fout_h, fout_c, wat_file_name):
    wat_file = open(wat_file_name, "r")
    lines = wat_file.readlines()
    sigs = []
    re_func = re.compile(r'^.*func.*export\s*"([a-zA-Z0-9_]*)"\)')
    re_param = re.compile(r'\(param\s*(.+?)\)')
    re_result = re.compile(r'\(result\s*(.+?)\)')
    for line in lines:
        fsig = {}
        fn = re_func.search(line)
        if fn == None:
            continue;
        fsig["name"] = fn.group(1)
        pa = re_param.search(line)
        if pa != None:
            fsig["param"] = pa.group(1).split()
        res = re_result.search(line)
        if res != None:
            fsig["result"] = res.group(1).split()
        sigs.append(fsig)
    return sigs

def write_func_decl(fout_h, sigs, prefix):
    fout_h.write("\n// The following functions needs to be manually implemented")
    for sig in sigs:
        fname = ""
        param = "tr_ctx"
        result = ""
        fname = prefix + "_" + sig["name"]
        if "param" in sig:
            param += ", " + ", ".join(sig["param"])
        if "result" in sig:
            result += ", {}*".format(sig["result"][0])
        func_decl = "extern r {} ({}{});".format(fname, param, result)
        fout_h.write("\n")
        fout_h.write(func_decl)
    fout_h.write("\n// The above functions needs to be manually implemented\n")

def to_tr_name(param, result):
    return "__tr_"+("_".join(param))+"_result_"+("_".join(result))+"__"

def write_trampoline_func(fout_c, name, param, result):
    fout_c.write("typedef r (*_p{})(tr_ctx".format(name))
    if (param[0] != "void") :
        fout_c.write(", {}".format(", ".join(param)))
    if (result[0] != "void") :
        fout_c.write(", {}*".format(", ".join(result)))
    fout_c.write(");\n")
    fout_c.write("static r {} (tr_ctx ctx, void * f) {{\n".format(name))
    fout_c.write("    check_prep(r);\n")
    if (result[0] != "void") :
        fout_c.write("    {} result = 0;\n".format(result[0]))
    fout_c.write("    check(((_p{})f)(ctx".format(name))
    counter=0
    if (param[0] != "void") :
        for p in param:
            fout_c.write(", ctx.args[{}].u_{}".format(counter, p))
            counter+=1
    if (result[0] != "void") :
        fout_c.write(", &result")
    fout_c.write("));\n")
    if (result[0] != "void") :
        fout_c.write("    ctx.args[0] = (value_u){.u_i32 = result};\n")
    fout_c.write("    return ok_r;\n")
    fout_c.write("}\n\n")

def write_trampoline(fout_c, sigs):
    # to avoid duplicated trampoline functions
    added = {}
    for sig in sigs:
        param = ["void"]
        result = ["void"]
        fname = sig["name"]
        if "param" in sig:
            param = sig["param"]
        if "result" in sig:
            result = sig["result"]
        tr_name = to_tr_name(param, result)
        if tr_name in added:
            continue
        added[tr_name] = 1
        write_trampoline_func(fout_c, tr_name, param, result)
    return 0

def write_host_info(fout_c, sigs, prefix):
    fout_c.write("\nconst host_func_info {}_host_info[] = {{\n".format(prefix));
    for sig in sigs:
        param = ["void"]
        result = ["void"]
        fname = sig["name"]
        if "param" in sig:
            param = sig["param"]
        if "result" in sig:
            result = sig["result"]
        tr_name = to_tr_name(param, result)
        fout_c.write('    {{"{}", {}, (void*){}}},\n'.format(fname, tr_name, prefix + "_" + fname))
    fout_c.write("};\n");
    fout_c.write("\nconst size_t {}_host_info_len = array_len({}_host_info);\n".format(prefix, prefix))

def do_convert(args):
    wat_file_name = args.file
    out_dir_name = args.out_dir
    if not os.path.isfile(wat_file_name):
        sys.stderr.write("Cannot read " + wat_file_name)
        return 1

    if not os.path.isdir(out_dir_name):
        sys.stderr.write("Target dir " + out_dir_name + " does not exist")
        return 1

    wasm_file_name = process_wat(wat_file_name, out_dir_name)
    if wasm_file_name == None:
        return 1

    # strip the wat part and use its basename for the .c and .h
    name = os.path.splitext(os.path.basename(wasm_file_name))[0]
    out_c = os.path.join(args.out_dir, name + "_gen.c")
    out_h = os.path.join(args.out_dir, name + "_gen.h")
    fout_c = open(out_c, 'w')
    fout_h = open(out_h, 'w')
    fwasm = open(wasm_file_name, 'rb')

    if write_wasm_binary_info(fout_h, fout_c, fwasm) != 0:
        return 1

    sigs = get_function_signatures(fout_h, fout_c, wat_file_name)
    if sigs == None:
        return 1

    write_func_decl(fout_h, sigs, name)
    write_trampoline(fout_c, sigs)
    write_host_info(fout_c, sigs, name)

    fout_c.close()
    fout_h.close()
    fwasm.close()
    os.remove(wasm_file_name)

exit(do_convert(args))

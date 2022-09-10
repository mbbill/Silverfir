#!/usr/bin/env python
# Bai Ming

import argparse
import json
import sys, os, glob
from shutil import copyfile
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument("json_folder", help="The folder where the generated spec test JSON files go", type=str)
parser.add_argument("harness", help="The harness binary to run the tests", type=str)
args = parser.parse_args()

def run_spec_test(json_folder, harness):
    if not os.path.exists(json_folder):
        sys.stderr.write("Invalid JSON folder")
        return 1
    if not os.path.exists(harness):
        sys.stderr.write("Invalid harness path")
        return 1
    for file in glob.glob(os.path.join(json_folder, "*.json")):
        cmd = [ harness, "-j", file, "-d", json_folder ]
        return_code = subprocess.call(cmd)
        if return_code != 0:
            return return_code
    return 0

ret = run_spec_test(args.json_folder, args.harness)
exit(ret)
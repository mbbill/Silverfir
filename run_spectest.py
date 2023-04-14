#!/usr/bin/env python
# Bai Ming

import os
import sys
import glob
import subprocess
import argparse


def find_harness_files(current_dir):
    build_types = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]
    harness_files = []

    for build_type in build_types:
        harness_path = os.path.join(current_dir, "build", "clang", build_type, "bin")
        if os.name == 'nt':  # Windows
            harness_file = os.path.join(harness_path, "harness.exe")
        else:  # Other operating systems
            harness_file = os.path.join(harness_path, "harness")

        if os.path.exists(harness_file):
            harness_files.append(harness_file)

    return harness_files


def parse_arguments():
    current_dir = os.path.dirname(os.path.realpath(__file__))
    default_json_folder = os.path.join(current_dir, "build", "testsuite")
    harness_files = find_harness_files(current_dir)

    if not harness_files:
        print("Error: No harness binaries found.")
        sys.exit(1)

    print("Found harness binaries:")
    for idx, harness_file in enumerate(harness_files, start=1):
        print("{}) {}".format(idx, harness_file))

    selected_idx = int(input("Select a harness binary by entering the corresponding number: "))
    selected_harness = harness_files[selected_idx - 1]

    parser = argparse.ArgumentParser(description="Run spec tests using a provided harness.")
    parser.add_argument("--json_folder", help="Path to the folder containing generated spec test JSON files. Default: {}".format(default_json_folder), type=str, default=default_json_folder)
    parser.add_argument("--harness", help="Path to the harness binary used to run the tests. Default: {}".format(selected_harness), type=str, default=selected_harness)
    return parser.parse_args()

def run_spec_test(json_folder, harness):
    if not os.path.exists(json_folder):
        sys.stderr.write("Error: The provided JSON folder does not exist: {}\n".format(json_folder))
        return 1
    if not os.path.exists(harness):
        sys.stderr.write("Error: The provided harness path does not exist: {}\n".format(harness))
        return 1

    # Blacklist for tests that should not run
    blacklist_prefix = ["simd"]

    for file in glob.glob(os.path.join(json_folder, "*.json")):
        # Check if the file is in the blacklist
        if any(file.startswith(os.path.join(json_folder, prefix)) for prefix in blacklist_prefix):
            print("Skipping blacklisted test: {}".format(file))
            continue

        cmd = [harness, "-j", file, "-d", json_folder]
        return_code = subprocess.call(cmd)
        if return_code != 0:
            sys.stderr.write("Error: Test failed for JSON file: {}\n".format(file))
            return return_code
    print("All non-blacklisted spec tests passed successfully.")
    return 0

def main():
    args = parse_arguments()
    ret = run_spec_test(args.json_folder, args.harness)
    sys.exit(ret)

if __name__ == "__main__":
    main()

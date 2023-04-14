#!/usr/bin/env python3

import argparse
import os
import re
import subprocess
import shutil

SUPPORTED_BUILD_TYPES = ["Debug", "Release", "RelWithDebInfo", "MinSizeRel"]

def check_required_tools():
    required_tools = ["cmake", "ninja"]
    missing_tools = []

    for tool in required_tools:
        if shutil.which(tool) is None:
            missing_tools.append(tool)

    if missing_tools:
        print(f"Error: The following required tools are missing: {', '.join(missing_tools)}")
        print("Please install them and try again.")
        exit(1)

def main():
    check_required_tools()

    parser = argparse.ArgumentParser(description="A script to run CMake and Ninja with the specified build type.")
    parser.add_argument("build_type", nargs='?', default=None, help="Build type to pass to cmake as the CMAKE_BUILD_TYPE parameter.")

    args = parser.parse_args()
    build_type = args.build_type

    if build_type is None:
        print("Please select a build type from the list below:")
        for i, bt in enumerate(SUPPORTED_BUILD_TYPES):
            print(f"{i + 1}. {bt}")
        selected = int(input("Enter the number corresponding to your choice: ")) - 1
        build_type = SUPPORTED_BUILD_TYPES[selected]

    if build_type not in SUPPORTED_BUILD_TYPES:
        print(f"Invalid build type: {build_type}. Supported build types are: {', '.join(SUPPORTED_BUILD_TYPES)}")
        exit(1)

    build_target_folder = f"build/clang/{build_type}"
    print(f"The build target folder is: {build_target_folder}")

    cmake_cmd = f"cmake -B {build_target_folder} -GNinja -DCMAKE_BUILD_TYPE={build_type}"
    ninja_cmd = f"ninja -C {build_target_folder}"

    print(f"Running cmake command: {cmake_cmd}")
    os.system(cmake_cmd)

    print(f"Running ninja command: {ninja_cmd}")
    os.system(ninja_cmd)

if __name__ == "__main__":
    main()